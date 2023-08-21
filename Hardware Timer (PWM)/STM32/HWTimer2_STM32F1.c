/***************************************************************************//**
 * @brief Hardware Timer 2 Implementation (STM32F1)
 * 
 * @file HWTimer2_STM32F1.c
 * 
 * @author Matthew Spinks <https://github.com/mspinksosu>
 * 
 * @date 9/18/22  Original creation
 *
 * @details
 *      // TODO
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

#include "HWTimer2_STM32F1.h"

/* Include processor specific header files here */
#include "stm32f10x_map.h"

// ***** Defines ***************************************************************

#define HW_TIM2_NUM_COMP_CHANNELS   4
#define TIMx    TIM2

// ***** Global Variables ******************************************************

/*  The sub class must implement the functions provided in the interface. In 
    this case we are declaring an interface struct and initializing its members 
    (which are function pointers) the our local functions. Typecasting is 
    necessary. When a new sub class object is created, we will set its interface
    member equal to this table. */
HWTimerInterface HWTimer2_FunctionTable = {
    .HWTimer_ComputePeriodUs = (void (*)(void *, uint32_t, uint32_t, uint16_t *))HWTimer2_STM32_ComputePeriodUs,
    .HWTimer_Init = (void (*)(void *))HWTimer2_STM32_Init,
};

static bool useOverflowInterrupt = false, useCompareMatchInterrupts = false;

// local function pointers
static void (*OverflowCallback)(void);
static void (*CompareMatchCallback)(uint8_t compChan);

// ***** Static Function Prototypes ********************************************

/* Put static function prototypes here */
static uint32_t compChanToAddress(uint8_t chan);

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// ***** Non-Interface Functions *********************************************//
//                                                                            //
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// ***** Interface Functions with Inheritance  *******************************//
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

void HWTimer2_ComputePeriodUs(HWTimerInitType_STM32 *retParams, uint32_t desiredPeriodUs, 
    uint32_t clkInHz, uint16_t *retDiffInTicks)
{
    uint32_t prescaleCounter = clkInHz / desiredPeriodUs;
    if(prescaleCounter > 0x0000FFFF)
        prescaleCounter = 0x0000FFFF;
    retParams->super->prescaleCounterValue = (uint16_t)prescaleCounter;
    retParams->super->prescaleSelect = HWTIM_PRESCALE_USES_COUNTER;
}

// *****************************************************************************

void HWTimer2_STM32_Init(HWTimerInitType_STM32 *params)
{
    /* Reset counter. Count up continously, edge aligned mode. Disable. */
    TIMx->CR1 = 0;
    TIMx->ARR = 0xFFFF; // reset auto-reload value
    TIMx->CCER = 0; // disable all compare channels
    TIMx->CCR1 = 0;
    TIMx->CCR2 = 0;
    TIMx->CCR3 = 0;
    TIMx->CCR4 = 0;
    TIMx->CCR5 = 0;
    TIMx->CCR6 = 0;
    TIMx->PSC = params->super->prescaleCounterValue;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// ***** Interface Functions *************************************************//
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

HWTimerPrescaleOptions HWTimer2_GetPrescaleOptions(void)
{
    HWTimerPrescaleOptions retVal = {.options.usesCounter = 1,
                                     .counterNumBits = 16 };
    return retVal;
}

// *****************************************************************************

HWTimerSize HWTimer2_GetSize(void)
{
    return HWTIM_16_BIT;
}

// *****************************************************************************

void HWTimer2_Start(void)
{
    TIMx->CR1 |= TIM_CR1_CEN;
}

// *****************************************************************************

void HWTimer2_Stop(void)
{
    TIMx->CR1 &= ~TIM_CR1_CEN;
}

// *****************************************************************************

void HWTimer2_Reset(void)
{
    TIMx->CNT = 0;
}

// *****************************************************************************

bool HWTimer2_IsRunning(void)
{
    if(TIMx->CR1 & TIM_CR1_CEN)
        return true;
    else
        return false;
}

// *****************************************************************************

void HWTimer2_SetCount(uint16_t count)
{
    TIMx->CNT = count;
}

// *****************************************************************************

uint16_t HWTimer2_GetCount(void)
{
    return TIMx->CNT;
}

// *****************************************************************************

void HWTimer2_AddToCount(uint16_t addToCount)
{
    TIMx->CNT += addToCount;
}

// *****************************************************************************

uint8_t HWTimer2_GetNumCompareChannels(void)
{
    return HW_TIM1_NUM_COMP_CHANNELS;
}

// *****************************************************************************

void HWTimer2_SetCompare16Bit(uint8_t compChan, uint16_t compValue)
{
    if(compChan >= HW_TIM1_NUM_COMP_CHANNELS)
        return;
    
    uint32_t *CCRx = compChanToAddress(compChan);
    *CCRx = compValue;
}

// *****************************************************************************

uint16_t HWTimer2_GetCompare16Bit(uint8_t compChan)
{
    if(compChan >= HW_TIM1_NUM_COMP_CHANNELS)
        return;
    
    uint32_t *CCRx = compChanToAddress(compChan);
    return *CCRx;
}

// *****************************************************************************

void HWTimer2_SetComparePercent(uint8_t compChan, uint8_t percent)
{
    if(compChan >= HW_TIM1_NUM_COMP_CHANNELS)
        return;
    
    if(percent > 100)
        percent = 100;

    uint32_t *CCRx = compChanToAddress(compChan);
    uint32_t compValue = percent * 65535 / 100;
    *CCRx = compValue;
}

// *****************************************************************************

uint8_t HWTimer2_GetComparePercent(uint8_t compChan)
{
    if(compChan >= HW_TIM1_NUM_COMP_CHANNELS)
        return 0;

    uint32_t *CCRx = compChanToAddress(compChan);
    return *CCRx * 100 / 65535;
}

// *****************************************************************************

void HWTimer2_EnableCompare(uint8_t compChan, bool useInterrupt)
{
    if(compChan >= HW_TIM1_NUM_COMP_CHANNELS)
        return;
    
    TIMx->CCER |= (1 << (compChan * 4));

    /* Only CC channels 0 - 3 (1 to 4) have interrupts */
    if(compChan < 4)
    {
        if(useInterrupt)
            TIMx->DIER |= (1 << (compChan + 1)); // bits [5:1]
        else
            TIMx->DIER &= ~(1 << (compChan + 1)); // bits [5:1]
    }
}

// *****************************************************************************

void HWTimer2_DisableCompare(uint8_t compChan)
{
    if(compChan >= HW_TIM1_NUM_COMP_CHANNELS)
        return;
    
    TIMx->CCER &= ~(1 << (compChan * 4));
}

// *****************************************************************************

bool HWTimer2_GetOverflow(void)
{
    return (TIMx->SR & TIM_SR_UIF) ? 1 : 0;
}

// *****************************************************************************

bool HWTimer2_GetCompareMatch(uint8_t compChan)
{
    if(compChan < 4)
    {
        return (TIMx->SR & (1 << (compChan + 1))) ? 1 : 0; // bits [5:1]
    }
    else if(compChan < 6)
    {
        return (TIMx->SR & (1 << (compChan + 12))) ? 1 : 0; // bits [17:16]
    }
    else
    {
        return false;
    }
}

// *****************************************************************************

void HWTimer2_ClearOverflowFlag(void)
{
    TIMx->SR &= ~TIM_SR_UIF;
}

// *****************************************************************************

void HWTimer2_ClearCompareMatchFlag(uint8_t compChan)
{
    if(compChan < 4)
    {
        TIMx->SR &= ~(1 << (compChan + 1)); // bits [5:1]
    }
    else if(compChan < 6)
    {
        TIMx->SR &= ~(1 << (compChan + 12)); // bits [17:16]
    }
}

// *****************************************************************************

void HWTimer2_OverflowEvent(void)
{
    TIMx->SR &= ~TIM_SR_UIF;

    if(OverflowCallback)
        OverflowCallback();
}

// *****************************************************************************

void HWTimer2_CompareMatchEvent(void)
{
    if(TIMx->SR & TIM_SR_CC1IF)
    {
        TIMx->SR &= ~TIM_SR_CC1IF;
        if(CompareMatchCallback)
            CompareMatchCallback(0);
    }
    if(TIMx->SR & TIM_SR_CC2IF)
    {
        TIMx->SR &= ~TIM_SR_CC2IF;
        if(CompareMatchCallback)
            CompareMatchCallback(1);
    }
    if(TIMx->SR & TIM_SR_CC3IF)
    {
        TIMx->SR &= ~TIM_SR_CC3IF;
        if(CompareMatchCallback)
            CompareMatchCallback(2);
    }
    if(TIMx->SR & TIM_SR_CC4IF)
    {
        TIMx->SR &= ~TIM_SR_CC4IF;
        if(CompareMatchCallback)
            CompareMatchCallback(3);
    }
    if(TIMx->SR & TIM_SR_CC5IF)
    {
        TIMx->SR &= ~TIM_SR_CC5IF;
        if(CompareMatchCallback)
            CompareMatchCallback(4);
    }
    if(TIMx->SR & TIM_SR_CC6IF)
    {
        TIMx->SR &= ~TIM_SR_CC6IF;
        if(CompareMatchCallback)
            CompareMatchCallback(5);
    }
}

// *****************************************************************************

void HWTimer2_SetOverflowCallback(void (*Function)(void))

{
    OverflowCallback = Function;
}

// *****************************************************************************

void HWTimer2_SetCompareMatchCallback(void (*Function)(uint8_t compChan))

{
    CompareMatchCallback = Function;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// ***** Local Functions *****************************************************//
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

static uint32_t compChanToAddress(uint8_t channel)
{
    uint32_t address = &(TIMx->CCR1); // address offset 0x34

    // channel 0 - 3 equals CCR1 to CCR4
    if(channel < 4)
    {
        // address offset 0x34 through 0x40
        address += channel * 4;
    }
    else if(channel < 6)
    {
        // address offset 0x58 and 0x5C
        address = address + 20 + channel * 4;
    }
}

/*
 End of File
 */