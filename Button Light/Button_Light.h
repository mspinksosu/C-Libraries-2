/***************************************************************************//**
 * @brief Button Library (Light) Header File
 * 
 * @author  Matthew Spinks
 * 
 * @date 6/28/22     Original creation
 * 
 * @file Button_Light.h
 * 
 * @details
 *      A stripped down version of my original button library with less
 * features. In order to try and save a little bit of memory, buttons are 
 * grouped in eight. There are no analog button options, no long press feature,
 * and the debounce length is used for both the press and release. This library 
 * can also be used for things like inserting connectors or switches. Anything 
 * that requires debouncing. 
 * 
 ******************************************************************************/

#ifndef BUTTON_LIGHT_H
#define	BUTTON_LIGHT_H

// ***** Includes **************************************************************

#include <stdint.h>
#include <stdbool.h>

// ***** Defines ***************************************************************


// ***** Global Variables ******************************************************


typedef struct ButtonGroupTag
{
    uint16_t debouncePeriod;
    //uint16_t debounceCounter;
    
    uint16_t integrator[8];
    // int16_t integrator1;
    // int16_t integrator2;
    // int16_t integrator3;
    // int16_t integrator4;
    // int16_t integrator5;
    // int16_t integrator6;
    // int16_t integrator7;
    // int16_t integrator8;

    uint8_t input;
    uint8_t output;
    uint8_t previousOutput;
    uint8_t pressed;
    uint8_t released;
} ButtonGroup;

/** 
 * Description of struct members: TODO
 * 
 */

// ***** Function Prototypes ***************************************************

void ButtonGroup_Init(ButtonGroup *self, uint16_t debounceMs, uint16_t tickMs);

// -----------------------------------------------------------------------------

void BG_Tick(ButtonGroup *self);

void BG_SetButtonValue(ButtonGroup *self, uint8_t index, bool isPressed);

bool BG_GetPress(ButtonGroup *self, uint8_t index);

void BG_ClearPressFlag(ButtonGroup *self, uint8_t index);

uint8_t BG_GetButtonOutput(ButtonGroup *self, uint8_t index);

uint8_t BG_GetButtonGroupOutput(ButtonGroup *self);

// TODO Button Released

#endif	/* BUTTON_LIGHT_H */