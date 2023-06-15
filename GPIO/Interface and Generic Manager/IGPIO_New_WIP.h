/***************************************************************************//**
 * @brief GPIO Interface Header File
 * 
 * @file IGPIO.h
 * 
 * @author Matthew Spinks <https://github.com/mspinksosu>
 * 
 * @date 2/2/22    Original creation
 * 
 * @details
 *      An interface for a GPIO library to be used with different processors.
 * There are two types of objects. One that holds the properties needed for the
 * pin, called GPIO and the other that holds the parameters needed to
 * initialize the pin called GPIOInitType.
 * 
 * To make a GPIO object, you will need to make a base class GPIO object, and
 * a subclass GPIO object. You will also need a base class GPIOInitType object
 * and possibly a subclass GPIOInitType object. The base class will usually 
 * contain at minimum, a pointer to the interface and a void pointer called 
 * "instance". But in this particular case, the MCU will never have more than 
 * one GPIO interface, so I've omitted the pointer to the interface. Sub class 
 * objects need their own variables which are not in the base class. This is 
 * why the void pointer is used. After the base and sub classes are linked, the 
 * void pointer will be set to point to the sub class.
 * 
 * The GPIOInterface or function table tells the interface what functions to 
 * call. Declare your GPIOInterface object as extern in your GPIO 
 * implementation's header file. Then in your implementation's .c file, declare
 * and initialize your GPIOInterface object. Set each of its members to the
 * functions in your implementation. Typecasting will be required. The function
 * pointers in the table will be using void pointers in place of the base class
 * types. In your implementation, your functions will be using your own sub 
 * class types.
 * 
 * A sub class will contain at minimum, a pointer to the base class named
 * "super". After creating a sub class, the base class will be connected to 
 * it by using the base class's Create function. These functions will set the 
 * void pointers for you. By using a void pointer to point to an instance 
 * of the subclass object, we remove the need for the user to have to do some 
 * ugly typecasting on every single function call. You will also need to set
 * your pointer to the base class ("super") to point to whatever base class
 * object you created.
 * 
 * I think the best way to do the previous step is to call the Create function 
 * from a sub class create function. Make your own implementation of the Create 
 * function that uses your sub class type and the base class type as arguments. 
 * This function will not be listed in the function table. From within that 
 * function, set your pointer to the base class ("super"), and then call the 
 * base class create function to finish setting the void pointer. The reason I 
 * prefer this method is it makes the process a little more type-safe by having 
 * the base class and sub class as arguments in the function signature.
 * 
 * If you need access to the pins from anywhere besides where the pins were 
 * declared, you can make the base class external. The function table will call 
 * the appropriate sub class function and give it the sub class object. All you 
 * would need is the base class GPIO variable and IGPIO.h. Doing it this way 
 * hides the sub class away and removes the need to include those files, which 
 * would otherwise create a processor specific dependency.
 * 
 * In the example below, there are two init type objects. These do not have to
 * be kept in memory. They can be declared as local variables and used for each
 * pin's initialization.
 * 
 * @section example_code Example Code
 *      GPIO_DriverSetInterface(&MCU1_GPIOInterface);
 *      GPIO led1;
 *      GPIO_MCU1 myMcuLED1; // extends GPIO pin
 *      GPIOInitType init;
 *      GPIOInitType_MCU1 myMcuInit // extends GPIO init type
 *      init.type = GPIO_TYPE_DIGITAL_OUTPUT;
 *      init.pull = GPIO_PULL_NONE;
 *      myMcuInit.extendedClassMember = 1;
 *      GPIO_MCU1_CreateInitType(&init, &myMcuInit); // connect sub and base
 *      GPIO_MCU1_Create(&led1, &myMcuLED1); // connect sub class and base
 *      GPIO_InitPin(&led1, &init);
 *      GPIO_Set(&led1); // set output high
 *      GPIO_SetType(&led1, GPIO_TYPE_ANALOG); // ready pin for sleep
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

#ifndef IGPIO_H
#define IGPIO_H

#include <stdint.h>
#include <stdbool.h>

// ***** Defines ***************************************************************


// ***** Global Variables ******************************************************

typedef enum GPIOPortTag
{
    GPIO_PORTA = 0,
    GPIO_PORTB,
    GPIO_PORTC,
    GPIO_PORTD,
    GPIO_PORTE,
    GPIO_PORTF,
    GPIO_PORTG,
    GPIO_PORTH,
    GPIO_PORTI,
    GPIO_PORTJ,
    GPIO_PORTK,
    GPIO_PORTL,
    GPIO_PORTM,
    GPIO_PORTN,
} GPIOPort;

/* Port and pin name combined into a single byte.
Upper nibble is the port. Lower nibble is the pin.
I've listed them at the bottom due to the size. */
typedef enum GPIOPinTag GPIOPin;

typedef enum GPIOTypeTag
{
    GPIO_TYPE_ANALOG,
    GPIO_TYPE_DIGITAL_INPUT,
    GPIO_TYPE_DIGITAL_OUTPUT,
    GPIO_TYPE_OPEN_DRAIN,
} GPIOType;

typedef enum GPIOPullTag
{
    GPIO_PULL_NONE,
    GPIO_PULL_UP,
    GPIO_PULL_DOWN,
} GPIOPull;

typedef struct GPIOInterfaceTag
{
    /* These are the functions that will be called. You will create your own
    interface object for your class that will have these function signatures 
    References to the base class are replaced with void. You will set each of 
    your functions equal to one of these pointers. Typecasting will be needed */
    void (*GPIO_InitPin)(GPIOPin pin, void *params);
    // void (*GPIO_SetPin)(GPIOPin pin);
    // void (*GPIO_ClearPin)(GPIOPin pin);
    // void (*GPIO_InvertPin)(GPIOPin pin);
    // void (*GPIO_WritePin)(GPIOPin pin, bool setPinHigh);
    // bool (*GPIO_ReadPin)(GPIOPin pin);
    // void (*GPIO_SetType)(GPIOPin pin, GPIOType);
    // GPIOType (*GPIO_GetType)(GPIOPin pin);
    // void (*GPIO_SetPull)(GPIOPin pin, GPIOPull);
    // GPIOPull (*GPIO_GetPull)(GPIOPin pin);
    void (*GPIO_WritePort)(GPIOPort port); // TODO
    uint16_t (*GPIO_ReadPort)(GPIOPort port); // TODO
} GPIOInterface;

typedef struct GPIOInitTypeTag
{
    void *instance;
    GPIOType type;
    GPIOPull pull;
} GPIOInitType;

/**
 * Description of struct members:
 * 
 * GPIOInterface  The table of functions that need to be implemented
 * 
 * GPIOInitType  The base class init object
 * 
 * GPIOPin  A value that refers to a port and a pin. Processor dependent. I've 
 *          made a list of port plus pin names. How you choose to use them is 
 *          up to you. The port is the upper nibble and the pin is the lower 
 *          nibble. The pin number refers to the number inside the port. It is 
 *          not the number written on the schematic, nor is it a mask. Some 
 *          processors will have only 8 pins per port. Others will have 16.
 * 
 * instance  A pointer to the instance of the sub class being used. This will 
 *           be set by means of the base class create function
 * 
 * type  The type of pin, analog input, digital input, etc.
 * 
 * pull  Internal pull-up / pull-down resistors. Default is none
 * 
 */

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// ***** Non-Interface Functions *********************************************//
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

/***************************************************************************//**
 * @brief Set the interface for this GPIO driver
 * 
 * Because there is usually only one GPIO driver per processor, there doesn't
 * need to be a pointer to the interface in each GPIO object. For this library
 * the interface will be static.
 * 
 * @param thisInterface  pointer to the function table you want to use
 */
void GPIO_DriverSetInterface(GPIOInterface *thisInterface);

/***************************************************************************//**
 * @brief Combine the base class and sub class
 * 
 * Links the instance pointer in the base class to the sub class. Because of 
 * the void pointer, my preferred method is to call this function from a sub
 * class constructor. I created a sub class constructor that needs an instance 
 * of the sub class and base class. This makes the create function more type
 * safe.
 * 
 * @param params  pointer to the GPIOInit type you are using
 * 
 * @param instanceOfSubClass  the child object that inherits from GPIOInitType
 */
void GPIO_CreateInitType(GPIOInitType *params, void *instanceOfSubClass);

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// ***** Interface Functions with Inheritance ********************************//
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

/***************************************************************************//**
 * @brief Initialize a GPIO object
 * 
 * Set the necessary registers for your MCU. When you call this function, you
 * give it a reference to your subclass of GPIOInitType.
 * 
 * @param pin  a combined port/pin identifier type
 * 
 * @param params  pointer to the GPIOInitType that provides the init parameters
 */
void GPIO_InitPin(GPIOPin pin, GPIOInitType *params);

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// ***** Simple Interface Functions ******************************************//
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

/* These functions should be implemented for your processor exactly as they are
written. I reason I do this is to increase speed a little bit. There is usually
only one GPIO peripheral on an MCU. Therefore, we shouldn't need multiple
instances of a GPIO object, so the functions can be called directly. */

/***************************************************************************//**
 * @brief Set a pin high
 * 
 * @param pin  a combined port/pin identifier type
 */
void GPIO_SetPin(GPIOPin pin);

/***************************************************************************//**
 * @brief Clear a pin
 * 
 * @param pin  a combined port/pin identifier type
 */
void GPIO_ClearPin(GPIOPin pin);

/***************************************************************************//**
 * @brief Invert a pin's value
 * 
 * @param pin  a combined port/pin identifier type
 */
void GPIO_InvertPin(GPIOPin pin);

/***************************************************************************//**
 * @brief Write a value to a pin
 * 
 * @param pin  a combined port/pin identifier type
 * 
 * @param setPinHigh  true = high, false = low
 */
void GPIO_WritePin(GPIOPin pin, bool setPinHigh);

/***************************************************************************//**
 * @brief Read the value of a pin
 * 
 * @param pin  a combined port/pin identifier type
 * 
 * @return bool  if analog return 0, if digital 0 = low, 1 = high
 */
bool GPIO_ReadPin(GPIOPin pin);

/***************************************************************************//**
 * @brief Set the pin type
 * 
 * Set the pin's associated registers
 * 
 * @param pin  a combined port/pin identifier type
 * 
 * @param type  GPIO_TYPE_ANALOG, GPIO_TYPE_DIGITAL_INPUT, 
 *              GPIO_TYPE_DIGITAL_OUTPUT, GPIO_TYPE_OPEN_DRAIN
 */
void GPIO_SetType(GPIOPin pin, GPIOType type);

/***************************************************************************//**
 * @brief Get the type of pin
 * 
 * Read the pin's associated registers and determine the pin type
 * 
 * @param pin  a combined port/pin identifier type
 * 
 * @return GPIOType  GPIO_TYPE_ANALOG, GPIO_TYPE_DIGITAL_INPUT, 
 *                   GPIO_TYPE_DIGITAL_OUTPUT, GPIO_TYPE_OPEN_DRAIN
 */
GPIOType GPIO_GetType(GPIOPin pin);

/***************************************************************************//**
 * @brief Set the pull up resistors
 * 
 * Set the pin's associated registers
 * 
 * @param pin  a combined port/pin identifier type
 * 
 * @param pullType  GPIO_PULL_UP, GPIO_PULL_DOWN, or GPIO_PULL_NONE
 */
void GPIO_SetPull(GPIOPin pin, GPIOPull pullType);

/***************************************************************************//**
 * @brief Get the pull up resistors if any
 * 
 * Read the pin's associated registers and determine the pull up type
 * 
 * @param pin  a combined port/pin identifier type
 * 
 * @return GPIOPull  GPIO_PULL_UP, GPIO_PULL_DOWN, or GPIO_PULL_NONE
 */
GPIOPull GPIO_GetPull(GPIOPin pin);


// ----- Pin Type Names --------------------------------------------------------

typedef enum GPIOPinTag
{
    GPIO_NONE = 0xE0, // a default value you can use that doesn't exist
    GPIO_PORTA_PIN0 = 0,
    GPIO_PORTA_PIN1,
    GPIO_PORTA_PIN2,
    GPIO_PORTA_PIN3,
    GPIO_PORTA_PIN4,
    GPIO_PORTA_PIN5,
    GPIO_PORTA_PIN6,
    GPIO_PORTA_PIN7,
    GPIO_PORTA_PIN8,
    GPIO_PORTA_PIN9,
    GPIO_PORTA_PIN10,
    GPIO_PORTA_PIN11,
    GPIO_PORTA_PIN12,
    GPIO_PORTA_PIN13,
    GPIO_PORTA_PIN14,
    GPIO_PORTA_PIN15,
    GPIO_PORTB_PIN0,
    GPIO_PORTB_PIN1,
    GPIO_PORTB_PIN2,
    GPIO_PORTB_PIN3,
    GPIO_PORTB_PIN4,
    GPIO_PORTB_PIN5,
    GPIO_PORTB_PIN6,
    GPIO_PORTB_PIN7,
    GPIO_PORTB_PIN8,
    GPIO_PORTB_PIN9,
    GPIO_PORTB_PIN10,
    GPIO_PORTB_PIN11,
    GPIO_PORTB_PIN12,
    GPIO_PORTB_PIN13,
    GPIO_PORTB_PIN14,
    GPIO_PORTB_PIN15,
    GPIO_PORTC_PIN0,
    GPIO_PORTC_PIN1,
    GPIO_PORTC_PIN2,
    GPIO_PORTC_PIN3,
    GPIO_PORTC_PIN4,
    GPIO_PORTC_PIN5,
    GPIO_PORTC_PIN6,
    GPIO_PORTC_PIN7,
    GPIO_PORTC_PIN8,
    GPIO_PORTC_PIN9,
    GPIO_PORTC_PIN10,
    GPIO_PORTC_PIN11,
    GPIO_PORTC_PIN12,
    GPIO_PORTC_PIN13,
    GPIO_PORTC_PIN14,
    GPIO_PORTC_PIN15,
    GPIO_PORTD_PIN0,
    GPIO_PORTD_PIN1,
    GPIO_PORTD_PIN2,
    GPIO_PORTD_PIN3,
    GPIO_PORTD_PIN4,
    GPIO_PORTD_PIN5,
    GPIO_PORTD_PIN6,
    GPIO_PORTD_PIN7,
    GPIO_PORTD_PIN8,
    GPIO_PORTD_PIN9,
    GPIO_PORTD_PIN10,
    GPIO_PORTD_PIN11,
    GPIO_PORTD_PIN12,
    GPIO_PORTD_PIN13,
    GPIO_PORTD_PIN14,
    GPIO_PORTD_PIN15,
    GPIO_PORTE_PIN0,
    GPIO_PORTE_PIN1,
    GPIO_PORTE_PIN2,
    GPIO_PORTE_PIN3,
    GPIO_PORTE_PIN4,
    GPIO_PORTE_PIN5,
    GPIO_PORTE_PIN6,
    GPIO_PORTE_PIN7,
    GPIO_PORTE_PIN8,
    GPIO_PORTE_PIN9,
    GPIO_PORTE_PIN10,
    GPIO_PORTE_PIN11,
    GPIO_PORTE_PIN12,
    GPIO_PORTE_PIN13,
    GPIO_PORTE_PIN14,
    GPIO_PORTE_PIN15,
    GPIO_PORTF_PIN0,
    GPIO_PORTF_PIN1,
    GPIO_PORTF_PIN2,
    GPIO_PORTF_PIN3,
    GPIO_PORTF_PIN4,
    GPIO_PORTF_PIN5,
    GPIO_PORTF_PIN6,
    GPIO_PORTF_PIN7,
    GPIO_PORTF_PIN8,
    GPIO_PORTF_PIN9,
    GPIO_PORTF_PIN10,
    GPIO_PORTF_PIN11,
    GPIO_PORTF_PIN12,
    GPIO_PORTF_PIN13,
    GPIO_PORTF_PIN14,
    GPIO_PORTF_PIN15,
    GPIO_PORTG_PIN0,
    GPIO_PORTG_PIN1,
    GPIO_PORTG_PIN2,
    GPIO_PORTG_PIN3,
    GPIO_PORTG_PIN4,
    GPIO_PORTG_PIN5,
    GPIO_PORTG_PIN6,
    GPIO_PORTG_PIN7,
    GPIO_PORTG_PIN8,
    GPIO_PORTG_PIN9,
    GPIO_PORTG_PIN10,
    GPIO_PORTG_PIN11,
    GPIO_PORTG_PIN12,
    GPIO_PORTG_PIN13,
    GPIO_PORTG_PIN14,
    GPIO_PORTG_PIN15,
    GPIO_PORTH_PIN0,
    GPIO_PORTH_PIN1,
    GPIO_PORTH_PIN2,
    GPIO_PORTH_PIN3,
    GPIO_PORTH_PIN4,
    GPIO_PORTH_PIN5,
    GPIO_PORTH_PIN6,
    GPIO_PORTH_PIN7,
    GPIO_PORTH_PIN8,
    GPIO_PORTH_PIN9,
    GPIO_PORTH_PIN10,
    GPIO_PORTH_PIN11,
    GPIO_PORTH_PIN12,
    GPIO_PORTH_PIN13,
    GPIO_PORTH_PIN14,
    GPIO_PORTH_PIN15,
    GPIO_PORTI_PIN0,
    GPIO_PORTI_PIN1,
    GPIO_PORTI_PIN2,
    GPIO_PORTI_PIN3,
    GPIO_PORTI_PIN4,
    GPIO_PORTI_PIN5,
    GPIO_PORTI_PIN6,
    GPIO_PORTI_PIN7,
    GPIO_PORTI_PIN8,
    GPIO_PORTI_PIN9,
    GPIO_PORTI_PIN10,
    GPIO_PORTI_PIN11,
    GPIO_PORTI_PIN12,
    GPIO_PORTI_PIN13,
    GPIO_PORTI_PIN14,
    GPIO_PORTI_PIN15,
    GPIO_PORTJ_PIN0,
    GPIO_PORTJ_PIN1,
    GPIO_PORTJ_PIN2,
    GPIO_PORTJ_PIN3,
    GPIO_PORTJ_PIN4,
    GPIO_PORTJ_PIN5,
    GPIO_PORTJ_PIN6,
    GPIO_PORTJ_PIN7,
    GPIO_PORTJ_PIN8,
    GPIO_PORTJ_PIN9,
    GPIO_PORTJ_PIN10,
    GPIO_PORTJ_PIN11,
    GPIO_PORTJ_PIN12,
    GPIO_PORTJ_PIN13,
    GPIO_PORTJ_PIN14,
    GPIO_PORTJ_PIN15,
    GPIO_PORTK_PIN0,
    GPIO_PORTK_PIN1,
    GPIO_PORTK_PIN2,
    GPIO_PORTK_PIN3,
    GPIO_PORTK_PIN4,
    GPIO_PORTK_PIN5,
    GPIO_PORTK_PIN6,
    GPIO_PORTK_PIN7,
    GPIO_PORTK_PIN8,
    GPIO_PORTK_PIN9,
    GPIO_PORTK_PIN10,
    GPIO_PORTK_PIN11,
    GPIO_PORTK_PIN12,
    GPIO_PORTK_PIN13,
    GPIO_PORTK_PIN14,
    GPIO_PORTK_PIN15,
    GPIO_PORTL_PIN0,
    GPIO_PORTL_PIN1,
    GPIO_PORTL_PIN2,
    GPIO_PORTL_PIN3,
    GPIO_PORTL_PIN4,
    GPIO_PORTL_PIN5,
    GPIO_PORTL_PIN6,
    GPIO_PORTL_PIN7,
    GPIO_PORTL_PIN8,
    GPIO_PORTL_PIN9,
    GPIO_PORTL_PIN10,
    GPIO_PORTL_PIN11,
    GPIO_PORTL_PIN12,
    GPIO_PORTL_PIN13,
    GPIO_PORTL_PIN14,
    GPIO_PORTL_PIN15,
    GPIO_PORTM_PIN0,
    GPIO_PORTM_PIN1,
    GPIO_PORTM_PIN2,
    GPIO_PORTM_PIN3,
    GPIO_PORTM_PIN4,
    GPIO_PORTM_PIN5,
    GPIO_PORTM_PIN6,
    GPIO_PORTM_PIN7,
    GPIO_PORTM_PIN8,
    GPIO_PORTM_PIN9,
    GPIO_PORTM_PIN10,
    GPIO_PORTM_PIN11,
    GPIO_PORTM_PIN12,
    GPIO_PORTM_PIN13,
    GPIO_PORTM_PIN14,
    GPIO_PORTM_PIN15,
    GPIO_PORTN_PIN0,
    GPIO_PORTN_PIN1,
    GPIO_PORTN_PIN2,
    GPIO_PORTN_PIN3,
    GPIO_PORTN_PIN4,
    GPIO_PORTN_PIN5,
    GPIO_PORTN_PIN6,
    GPIO_PORTN_PIN7,
    GPIO_PORTN_PIN8,
    GPIO_PORTN_PIN9,
    GPIO_PORTN_PIN10,
    GPIO_PORTN_PIN11,
    GPIO_PORTN_PIN12,
    GPIO_PORTN_PIN13,
    GPIO_PORTN_PIN14,
    GPIO_PORTN_PIN15,
} GPIOPin;

#endif  /* IGPIO_H */
