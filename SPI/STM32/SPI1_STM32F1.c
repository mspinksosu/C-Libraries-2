/***************************************************************************//**
 * @brief SPI Library Implementation (STM32F1)
 * 
 * @author Matthew Spinks
 * 
 * @date 5/6/23    Original creation
 * 
 * @file SPI1_STM32F1.c
 * 
 * @details
 *      // TODO details
 * 
 ******************************************************************************/

#include "SPI1.h"
#include <stddef.h> // needed for NULL

/* Include processor specific header files here */
#include "stm32f10x_map.h"

// ***** Defines ***************************************************************

enum {
    SPI1_PRE_DIV_2 = 0,
    SPI1_PRE_DIV_4,
    SPI1_PRE_DIV_8,
    SPI1_PRE_DIV_16,
    SPI1_PRE_DIV_32,
    SPI1_PRE_DIV_64,
    SPI1_PRE_DIV_128,
    SPI1_PRE_DIV_256};

/* Prescale selection. Source = Pclk. Choose from the enum */
#define SPI_PRESCALE    SPI1_PRE_DIV_2

/* Peripheral addresses and registers */
#define SPI_ADDR        SPI1
#define SPI_CLK_REG     RCC->APB2ENR
#define SPI_CLK_EN_MSK  RCC_APB2ENR_SPI1EN

// ***** Global Variables ******************************************************

/* Assign functions to the interface */
SPIInterface SPI1_FunctionTable = {
    .SPI_Init = SPI1_Init,
    .SPI_Enable = SPI1_Enable,
    .SPI_Disable = SPI1_Disable,
    .SPI_ReceivedDataEvent = SPI1_ReceivedDataEvent,
    .SPI_GetReceivedByte = SPI1_GetReceivedByte,
    .SPI_IsReceiveRegisterFull = SPI1_IsReceiveRegisterFull,
    .SPI_TransmitRegisterEmptyEvent = SPI1_TransmitRegisterEmptyEvent,
    .SPI_TransmitByte = SPI1_TransmitByte,
    .SPI_IsTransmitRegisterEmpty = SPI1_IsTransmitRegisterEmpty,
    .SPI_IsTransmitFinished = SPI1_IsTransmitFinished,
    .SPI_GetStatus = SPI1_GetStatus,
    .SPI_PendingEventHandler = SPI1_PendingEventHandler,
    .SPI_SetTransmitRegisterEmptyCallback = SPI1_SetTransmitRegisterEmptyCallback,
    .SPI_SetReceivedDataCallback = SPI1_SetReceivedDataCallback,
    .SPI_SetSSPinFunc = SPI1_SetSSPinFunc,
};

static bool useRxInterrupt = false, useTxInterrupt = false;
static SPIRole role = SPI_ROLE_MASTER;
static SPIMode mode = SPI_MODE_0;
static SPISSControl ssControl = SPI_SS_NONE;
static bool lockTxFinishedEvent = false, txFinishedEventPending = false,
    lockRxReceivedEvent = false;

// local function pointers
static void (*TransmitRegisterEmptyCallback)(void);
static void (*ReceivedDataCallback)(uint8_t (*CallToGetData)(void));
static void (*SetSSPin)(bool setHigh);

// ***** Static Function Prototypes ********************************************

/* Put static function prototypes here */


////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// ***** Interface Functions *************************************************//
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

void SPI1_Init(SPIInitType *params)
{
    role = params->role;
    mode = params->mode;
    ssControl = params->ssControl;

    /* Peripheral clock must be enabled before you can write any registers */
    SPI_CLK_REG |= SPI_CLK_EN_MSK;

    /* Turn off module before making changes */
    SPI_ADDR->CR1 &= ~SPI_CR1_SPE;

    /* Turn off tx/rx interrupts */
    SPI_ADDR->CR2 &= ~(SPI_CR2_RXNEIE | SPI_CR2_TXEIE);

    /* 8-bit data register width (default) */
    SPI_ADDR->CR1 &= ~SPI_CR1_DFF;

    /* Set the baud rate. Pclk / prescale */
    SPI_ADDR->CR1 &= SPI_CR1_BR;
    SPI_ADDR->CR1 |= (SPI_PRESCALE << 3);

    if(role == SPI_ROLE_MASTER)
    {
        SPI_ADDR->CR1 |= SPI_CR1_MSTR;
    }
    else
    {
        SPI_ADDR->CR1 &= ~SPI_CR1_MSTR;
    }

    switch(mode)
    {
        case SPI_MODE_1:
            SPI_ADDR->CR1 &= ~(SPI_CR1_CPOL);
            SPI_ADDR->CR1 |= SPI_CR1_CPHA;
            break;
        case SPI_MODE_2:
            SPI_ADDR->CR1 |= SPI_CR1_CPOL;
            SPI_ADDR->CR1 &= ~(SPI_CR1_CPHA);
            break;
        case SPI_MODE_3:
            SPI_ADDR->CR1 |= SPI_CR1_CPOL;
            SPI_ADDR->CR1 |= SPI_CR1_CPHA;
            break;
        default:
            SPI_ADDR->CR1 &= ~(SPI_CR1_CPOL | SPI_CR1_CPHA);
            break;
    }

    /* In NSS software mode, set SSM and SSI. If used as input in hardware 
    mode, NSS pin must pulled high. If using NSS as output in hardware mode,
    set SSOE only. Ref man 25.3.3 (page 707) */
    if(ssControl == SPI_SS_HARDWARE)
    {
        SPI_ADDR->CR1 &= ~SPI_CR1_SSI;
        SPI_ADDR->CR1 &= ~SPI_CR1_SSM;
        SPI_ADDR->CR2 |= SPI_CR2_SSOE;
    }
    else
    {
        SPI_ADDR->CR1 |= SPI_CR1_SSI;
        SPI_ADDR->CR1 |= SPI_CR1_SSM;
        SPI_ADDR->CR2 &= ~SPI_CR2_SSOE;
    }

    /* If you turn on the transmit interrupt during initialization, it could
    fire off repeatedly. It's best to turn it on after placing data in the 
    transmit register */

    if(useRxInterrupt)
        SPI_ADDR->CR2 |= SPI_CR2_RXNEIE;

    SPI_ADDR->CR1 |= SPI_CR1_SPE; // enable SPI
}

// *****************************************************************************

void SPI1_Enable(void)
{
    SPI_ADDR->CR1 |= SPI_CR1_SPE;

    if(useRxInterrupt)
        SPI_ADDR->CR2 |= SPI_CR2_RXNEIE;

    /* Set slave select high */
    if(ssControl == SPI_SS_CALLBACKS && SetSSPin != NULL)
    {
        SetSSPin(true);
    }
}

// *****************************************************************************

void SPI1_Disable(void)
{
    /* Great care must be taken when using the BSY flag with the SPI. Simply
    checking the BSY flag alone is not enough to reliably detect it. The 
    reference manual gives very specific instructions for what to do. 
    Ref man page 718, master or slave full-duplex mode: */
    while(!(SPI_ADDR->SR & SPI_SR_RXNE));
    while(!(SPI_ADDR->SR & SPI_SR_TXE));
    while(SPI_ADDR->SR & SPI_SR_BSY);
    SPI_ADDR->CR1 &= ~SPI_CR1_SPE;

    /* Set slave select high */
    if(ssControl == SPI_SS_CALLBACKS && SetSSPin != NULL)
    {
        SetSSPin(true);
    }
}

// *****************************************************************************

void SPI1_ReceivedDataEvent(void)
{
    if(lockRxReceivedEvent == true)
    {
        /* Prevent the possibility of another interrupt from somehow calling us 
        while we're in a callback. This won't happen in SPI master mode because
        we control the clock. */
        return;
    }
    lockRxReceivedEvent = true;

    if(ReceivedDataCallback)
    {
        ReceivedDataCallback(SPI1_GetReceivedByte);
    }
    lockRxReceivedEvent = false;
}

// *****************************************************************************

uint8_t SPI1_GetReceivedByte(void)
{
    /* data is right aligned */
    uint8_t data = (uint8_t)(SPI_ADDR->DR << 8);

    return data;
}

// *****************************************************************************

bool SPI1_IsReceiveRegisterFull(void)
{
    /* The RX register not empty flag is set when the receive data register has 
    a character placed in it. It is cleared by reading the character from the
    receive data register. */
    if(SPI_ADDR->SR & SPI_SR_RXNE)
        return true;
    else
        return false;
}

// *****************************************************************************

void SPI1_TransmitRegisterEmptyEvent(void)
{
    /* This will prevent recursive calls if we call transmit byte function from
    within the transmit interrupt callback. This requires the pending event
    handler function to be called to catch the txFinishedEventPending flag. */
    if(lockTxFinishedEvent == true)
    {
        txFinishedEventPending = true;
        return;
    }
    lockTxFinishedEvent = true;

    /* Disable transmit interrupt here */
    SPI_ADDR->CR2 &= ~SPI_CR2_TXEIE;

    if(TransmitRegisterEmptyCallback)
    {
        TransmitRegisterEmptyCallback();
    }
}

// *****************************************************************************

void SPI1_TransmitByte(uint8_t data)
{
    SPI_ADDR->DR = data;

    /* Enable transmit interrupt here if needed */
    if(useTxInterrupt)
        SPI_ADDR->CR2 |= SPI_CR2_TXEIE;
}

// *****************************************************************************

bool SPI1_IsTransmitRegisterEmpty(void)
{
    /* The transmit register empty flag is set when the contents of the
    transmit data register are emptied. It is cleared when the transmit data
    register is written to */
    if(SPI_ADDR->SR & SPI_SR_TXE)
        return true;
    else
        return false;
}

// *****************************************************************************

bool SPI1_IsTransmitFinished(void)
{
    /* Do not use the busy flag to handle data transmission. You should use the
    TXE flag instead. The busy flag is low in master mode during reception.
    so if the TXE flag is high (tx empty) and BSY is low, it "should" mean that
    we've fully finished shifting bytes out. */
    if((SPI_ADDR->SR & SPI_SR_TXE) && !(SPI_ADDR->SR & SPI_SR_BSY))
        return true;
    else
        return false;
}

// *****************************************************************************

SPIStatusBits SPI1_GetStatus(void)
{
    /* TODO My first I2C state machine had a lot decisions to make and relied
    on status bits. Whereas the UART could just throw bytes into a buffer. 
    I decided to make some for SPI also. But now I'm thinking I may not need 
    these after all. Decide if I want to keep these or not after testing. */
    SPIStatusBits status = {0};

    if(SPI_ADDR->SR & SPI_SR_BSY)
        status.BSY = 1;
    if(SPI_ADDR->SR & SPI_SR_TXE)
        status.TXE = 1;
    if(SPI_ADDR->SR & SPI_SR_RXNE)
        status.RXNE = 1;
    if(SPI_ADDR->SR & SPI_SR_MODF)
        status.FAULT = 1;
    if(SPI_ADDR->SR & SPI_SR_OVR)
        status.OVF = 1;

    return status;
}

// *****************************************************************************

void SPI1_PendingEventHandler(void)
{
    if(txFinishedEventPending && !lockTxFinishedEvent)
    {
        txFinishedEventPending = false;
        SPI1_TransmitRegisterEmptyEvent();
    }
}

// *****************************************************************************

void SPI1_SetTransmitRegisterEmptyCallback(void (*Function)(void))
{
    TransmitRegisterEmptyCallback = Function;
}

// *****************************************************************************

void SPI1_SetReceivedDataCallback(void (*Function)(uint8_t (*CallToGetData)(void)))
{
    ReceivedDataCallback = Function;
}

// *****************************************************************************

void SPI1_SetSSPinFunc(void (*Function)(bool setPinHigh))
{
    SetSSPin = Function;
}

/*
 End of File
 */