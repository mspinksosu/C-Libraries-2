/***************************************************************************//**
 * @brief ADC Manager (STM32G0 DMA)
 * 
 * @file ADC_Manager_STM32G0_DMA.c
 * 
 * @author Matthew Spinks <https://github.com/mspinksosu>
 * 
 * @date 2/19/22  Original creation
 * @date 2/26/22  Added function pointers for enable and disable
 * 
 * @details
 *      This is a special version of the ADC_Manager that incorporates the DMA
 * controller of the G0 processor to store results in the DMAArray variable.
 * This file replaces ADC_Manager.c.
 * 
 * This library  makes use of the PeripheralEnable and PeripheralDisable 
 * callback functions. These will, in turn, stop or reset and start the DMA
 * controller. This makes it so that the ADC library no longer needs to care
 * about the DMA controller or include any DMA controller related header files.
 * This also allows the user to call the ADC_TakeSample function at any time
 * and automatically pause the DMA sequence.
 * 
 * The use of an array with all the parts combined will allow the user to 
 * declare and initialize everything here in this file without needing to call 
 * the init function individually for every object. I just did as an example of 
 * different way to initialize all the channels as an array. I did this because
 * the DMA array must have a known size before initialization. The size is 
 * determined by the number of channels times the samples. Since the number of
 * channels must be known, I decided to put the channels in array.
 * 
 * One important difference is that I'm using an external pointer to point to
 * the ADC channel rather than declaring the ADC channels as extern. This is 
 * because the ADC channels are in an array, and if I make the array extern, I
 * have to have some way to decode the index. This leads to having an enum
 * somewhere that always has to match the order of this array exactly. By using
 * the pointer and setting it during the declaration of the array's members, 
 * everything can be done here and the declarations can be done in any order.
 * The downside of this method is that it takes up slightly more memory due to
 * the pointer. Remember that because we are using a pointer, the function 
 * calls don't need to use the reference operator:
 * ADC_Get16Bit(tempInternal) vs ADC_Get16Bit(&tempInternal) <-not needed
 * 
 * The number of channels is given by a define statement near the top of this
 * file. This value must be altered to match the number of channels declared in
 * the array. The DMA controller requires that the channels be sorted by the 
 * channel number. During the initialization process, the list is sorted
 * everytime a new channel is added.
 * 
 * More channel objects can still be added to the list from outside this file,
 * though the appropriate init functions still have to be called. If more 
 * channel objects are added after initialization, they will still operate and 
 * you can still read their values. These channels will be added to the back of 
 * the list and won't have a matching entry in the DMA array.
 * 
 * @section license License
 * SPDX-FileCopyrightText: © 2022 Matthew Spinks
 * SPDX-License-Identifier: Zlib
 * 
 * This software is released under the Zlib license. You are free alter and
 * redistribute it, but you must not misrepresent the origin of the software.
 * This notice may not be removed. <http://www.zlib.net/zlib_license.html>
 * 
 ******************************************************************************/

#include "ADC_Manager.h"
#include <stddef.h> // needed for NULL

/* Include processor specific header files here */
#include "stm32g071xx.h"
#include "stm32g0xx_ll_adc.h"

// ***** Defines ***************************************************************

#define ADC_MANAGE_SAMPLE_MS            5 // time to wait for sample to finish
#define ADC_MANAGE_TICK_MS              1
#define ADC_MANAGE_NUM_CHANNELS         3
#define ADC_MANAGE_SAMPLES_PER_CHANNEL  8

// ***** Global Variables ******************************************************

static uint8_t numChannelsInList, loopCount = 0;
static ADCChannelEntry *ptrToLast = NULL;      // linked list
static ADCChannelEntry *currentChannel = NULL; // index for linked list
static uint32_t dmaChannelSelection = 0;    // for the channel select register
static uint16_t DMAArray[ADC_MANAGE_NUM_CHANNELS * ADC_MANAGE_SAMPLES_PER_CHANNEL];
static bool adcManagerEnabled = true;

typedef struct ADCChannelFullTag
{
    ADCChannel **ptrToChannel;
    ADCChannel adcChannel;
    ADCChannelEntry entry;
} ADCChannelFull;

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// ***** Initialize ADC Channels *********************************************//
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

/* Declare pointers for adc channels starting here. Each variable should have a 
matching extern declaration in a header file. Use a memorable name. This is the 
pointer you will use for library function calls. */
//------------------------------------------------------------------------------

ADCChannel *vrefInternal, *tempInternal, *vbatInternal;

// -----------------------------------------------------------------------------

/* Initialize the channel array here. Set each channel's pointer member to the 
external pointer that you wish to use for it. Be sure to match the array size
with the number of channels you have */
static ADCChannelFull ChannelArray[ADC_MANAGE_NUM_CHANNELS] = {
{   .ptrToChannel = &vrefInternal,
    .adcChannel.channelNumber = 12},
     
{   .ptrToChannel = &tempInternal,
    .adcChannel.channelNumber = 13},
    
{   .ptrToChannel = &vbatInternal,
    .adcChannel.channelNumber = 14}};
                           
// ***** Static Function Prototypes ********************************************

/* Put static function prototypes here */
static void ADC_Manager_ChannelPush(ADCChannelEntry *self, ADCChannel *newChannel);
static void ADC_Manager_InsertChannelAfter(ADCChannelEntry *entryToInsert, ADCChannelEntry *prev, ADCChannel *newChannel);
static uint32_t ADC_Manager_AverageDMAArrayValues(uint8_t channel);

// *****************************************************************************

void ADC_Manager_Init(uint16_t sampleTimeMs, uint16_t tickRateMs)
{
    /* Initialize the ADC peripheral */
    ADC_InitPeripheralNonBlocking(sampleTimeMs, tickRateMs);

    /* Disable the ADC and reconfigure for DMA, trigger on timer */
    ADC_Disable();
    LL_ADC_REG_SetTriggerSource(ADC1, LL_ADC_REG_TRIG_EXT_TIM1_TRGO2);
    LL_ADC_REG_SetSequencerScanDirection(ADC1, LL_ADC_REG_SEQ_SCAN_DIR_FORWARD);

    /* Configure the DMA transfer */
    LL_DMA_ConfigAddresses(DMA1,LL_DMA_CHANNEL_1, LL_ADC_DMA_GetRegAddr(ADC1, LL_ADC_DMA_REG_REGULAR_DATA), &DMAArray, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_1, LL_DMAMUX_REQ_ADC1);
    LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_1, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MODE_CIRCULAR);
    LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PDATAALIGN_HALFWORD);
    LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MDATAALIGN_HALFWORD);

    /* Setup all the channels at once */
    for(uint8_t i = 0; i < ADC_MANAGE_NUM_CHANNELS; i++)
    {
        if(ChannelArray[i].ptrToChannel != NULL)
        {
            *(ChannelArray[i].ptrToChannel) = &(ChannelArray[i].adcChannel);
            ADC_Manager_AddChannel(&(ChannelArray[i].entry), &(ChannelArray[i].adcChannel));

            /* Add to DMA sequencer. Order of channels is fixed by channel 
            hardware number. The first entries added to my list must be sorted 
            by channel number. */
            dmaChannelSelection |= (1 << ChannelArray[i].adcChannel.channelNumber);
        }
    }
    ADC1->CHSELR = dmaChannelSelection;

    ADC_Manager_Enable(); /* Turn on the DMA */

    /* Set function callbacks. This needs to be done last. From now on, when 
    the ADC needs to start or stop it will call these functions which will 
    start and stop the DMA for us automatically. This way, the ADC doesn't need 
    to know or care about the DMA or the DMA channel number */
    ADC_SetPeripheralEnabledCallbackFunc(ADC_Manager_Enable);
    ADC_SetPeripheralDisabledCallbackFunc(ADC_Manager_Disable);
}

// *****************************************************************************

void ADC_Manager_AddChannel(ADCChannelEntry *self, ADCChannel *newChannel)
{
    if(ptrToLast == NULL)
    {
        /* Begin with empty list */
        self->channel = newChannel;

        /* Update the pointer to point to the last entry, which is itself */
        ptrToLast = self;

        /* Since the list only contains one entry, the "next" pointer will
        also point to itself */
        self->next = ptrToLast;
    }
    else if(numChannelsInList < ADC_MANAGE_NUM_CHANNELS)
    {
        /* The STM32 DMA controller requires you to have an array of a known
        size declared to use it. It also requires that the list of channels be 
        sorted by channel number. */

        /* tempHead = beginning of list */
        ADCChannelEntry *tempHead = ptrToLast->next;

        if(newChannel->channelNumber <= tempHead->channel->channelNumber)
        {
            ADC_Manager_ChannelPush(self, newChannel);
        }
        else
        {
            ADCChannelEntry *peekNext;
            
            /* Look for where to insert new data */
            while(tempHead != ptrToLast)
            {
                peekNext = tempHead->next;
                if(newChannel->channelNumber < peekNext->channel->channelNumber)
                    break;
                tempHead = tempHead->next;
            }
            ADC_Manager_InsertChannelAfter(self, tempHead, newChannel);
        }
    }
    else
    {
        /* In order to maintain the interface, I will allow a user to add 
        another channel to the list if necessary. But there will be no entry
        in the DMA array for it. Add it to the back of the list */
        ADC_Manager_InsertChannelAfter(self, ptrToLast, newChannel);
    }

    numChannelsInList++;

    /* Set index to the beginning and reset the count for the DMA array */
    currentChannel = ptrToLast->next;
    loopCount = 0;

    /* Initialize the new channel */
    ADC_InitChannel(newChannel, newChannel->channelNumber);
}

// *****************************************************************************

void ADC_Manager_Tick(void)
{
    ADC_Tick();

    /* Go round-robin through the list */
    if(adcManagerEnabled && currentChannel != NULL)
    {
        /* Start by getting the entries from the DMA array. The list begins in 
        order and matches the DMA array */
        if(loopCount < ADC_MANAGE_NUM_CHANNELS)
        {
            currentChannel->channel->adcValue = (uint16_t)ADC_Manager_AverageDMAArrayValues(loopCount);
            loopCount++;
            currentChannel = currentChannel->next;
        }
        else if(loopCount < numChannelsInList)
        {
            if(!ADC_IsBusy())
            {
                /* When the take sample function is called, the DMA is paused
                automatically through the enable and disable ADC callback
                functions. The ADC will restore its channel select settings
                when it finishes. */
                ADC_TakeSample(currentChannel);
                currentChannel = currentChannel->next;
                loopCount++;
            }
        }
        else
        {
            /* TODO check DMA sequencer gets reset after TakeSample */
            loopCount = 0;
        }
    }
}

// *****************************************************************************

void ADC_Manager_Enable(void)
{
    /* Re-enable DMA channel */
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, (TOTAL_ADC_CHANNELS* ADC_SAMPLES_PER_CHANNEL));
    LL_ADC_REG_SetDMATransfer(ADC1, LL_ADC_REG_DMA_TRANSFER_UNLIMITED);
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);

    if(!ADC_IsEnabled())
        ADC_Enable();
    
    adcManagerEnabled = true;
}

// *****************************************************************************

void ADC_Manager_Disable(void)
{
    /* Stop the DMA sequence */
    LL_ADC_REG_SetDMATransfer(ADC1, LL_ADC_REG_DMA_TRANSFER_NONE);
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);
    while(LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_1)){}
    
    adcManagerEnabled = false;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// ***** Local Functions *****************************************************//
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

/***************************************************************************//**
 * @brief 
 * 
 * @param self 
 * @param newChannel 
 */
static void ADC_Manager_ChannelPush(ADCChannelEntry *self, ADCChannel *newChannel)
{
    /* Store the new data */
    self->channel = newChannel;

    /* Add the new entry to the beginning of the list. The last entry's "next" 
    pointer will always point to the beginning of the list */
    self->next = ptrToLast->next;

    /* Update the beginning entry to point to the new beginning */
    ptrToLast->next = self;
}

/***************************************************************************//**
 * @brief 
 * 
 * @param entryToInsert 
 * @param prev 
 * @param newChannel 
 */
static void ADC_Manager_InsertChannelAfter(ADCChannelEntry *entryToInsert, ADCChannelEntry *prev, ADCChannel *newChannel)
{
    if(prev == NULL || entryToInsert == NULL)
    {
        return;
    }
    // Store the new data
    entryToInsert->channel = newChannel;
    // Insert the new entry
    entryToInsert->next = prev->next;
    // Change the previous entry to point to our new entry
    prev->next = entryToInsert;
}

/***************************************************************************//**
 * @brief 
 * 
 * @param channelNumber 
 * @return uint32_t 
 */
static uint32_t ADC_Manager_AverageDMAArrayValues(uint8_t channelNumber)
{
    uint32_t average;

    for(uint16_t i = channelNumber; i < sizeof(ChannelArray); i+=ADC_MANAGE_NUM_CHANNELS)
    {
        average += DMAArray[i];
    }
    average = average / ADC_MANAGE_SAMPLES_PER_CHANNEL;

    return average;
}

/*
 End of File
 */