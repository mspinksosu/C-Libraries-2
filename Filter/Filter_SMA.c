/***************************************************************************//**
 * @brief Filter Library Implementation (SMA Filter)
 * 
 * @author Matthew Spinks
 * 
 * @date 12/2/14   Original creation
 * @date 2/4/22    Modified
 * 
 * @file Filter_SMA.c
 * 
 * @details
 *      TODO
 * 
 ******************************************************************************/

#include "Filter_SMA.h"

// ***** Defines ***************************************************************


// ***** Global Variables ******************************************************

/*  Declare an interface struct and initialize its members the our local 
    functions. Typecasting is necessary. When a new sub class object is 
    created, we will set its interface member equal to this table. */
FilterInterface FilterFunctionTable = {
    .Filter_ComputeU16 = (uint16_t (*)(void *, uint16_t))Filter_SMA_ComputeU16,
};

// ***** Static Function Prototypes ********************************************

/* Put static function prototypes here */


////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// ***** Non-Interface Functions *********************************************//
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

void Filter_SMA_Create(Filter_SMA *self, Filter *base)
{
    self->super = base;

    /*  Call the base class constructor */
    Foo_Create(base, self, &FilterFunctionTable);
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// ***** Interface Functions *************************************************//
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

// TODO
#define AVERAGE_LENGTH  8
uint16_t Filter_SMA_ComputeU16(Filter *self, uint16_t input)
{
    /* @note This is a type of filter called a simple moving average filter.
    It makes a buffer of samples, and averages the samples. There is one clever
    trick. There are only two values in the sum that change. Instead of summing
    the buffer each loop, we subtract from the sum the current value in the 
    buffer we are at. Then add the new input to the sum. - MS */
    static uint8_t smaIndex = 0;
    static uint32_t sum = 0;
    static uint32_t buffer[AVERAGE_LENGTH];
    uint16_t output;

    sum -= buffer[smaIndex];
    sum += input;
    buffer[smaIndex] = input;
    output = sum / AVERAGE_LENGTH;

    smaIndex++;
    if(smaIndex == AVERAGE_LENGTH)
        smaIndex = 0;
}

/*
 End of File
 */