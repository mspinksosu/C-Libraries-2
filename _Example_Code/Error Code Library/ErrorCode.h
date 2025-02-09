/***************************************************************************//**
 * @brief Error Code Library Header File
 * 
 * @file ErrorCode.h
 * 
 * @author Matthew Spinks
 * 
 * @date 11/20/22  Original creation
 * 
 * @details
 *      Manages up to 32 error codes and flashes an LED for you. The LED can be 
 * updated by calling ErrorCode_GetBlinkOutput(). When you initialize this
 * library, you must give it a tick rate in milliseconds. This is the rate at 
 * which you plan to call the Tick() function. Every tick, the library will
 * go through the list of error codes and set the LED output accordingly.
 * 
 *      The error codes begin at 1 and go to 32. An error code of zero is
 * equivalent to having no errors. By default, the library will show each error 
 * code that is set, one by one. Once it finishes blinking an error code, it 
 * will insert a brief pause, then begin the next one. Calling the 
 * GetCurrentErrorCode function will tell you which one is being flashed at
 * that moment.
 * 
 *      Calling GetActiveMask will give a 32-bit word of all the errors that 
 * are currently set. The error code state machine is normally always running, 
 * even if there are no error codes. Do not use the IsRunning function to see 
 * if there are any error codes. Rather, get the active mask and check if it is 
 * zero instead. The reason I have the Start, Stop, and IsRunning functions is 
 * so that you can pause the state machine briefly if you need to take control 
 * of the LED pin. When restarting the error code, it will start over on the 
 * current error code instead of in the middle of an error code.
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

#ifndef ERROR_CODE_H
#define ERROR_CODE_H

#include <stdint.h>
#include <stdbool.h>

// ***** Defines ***************************************************************

#define DEFAULT_EC_PRIORITY_LEVEL   64

// ***** Global Variables ******************************************************


////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// ***** Function Prototypes *************************************************//
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

/***************************************************************************//**
 * @brief Initialize the library
 * 
 * Sets the expected tick rate and the LED pattern. The tick rate you specify
 * should preferrably be between 1 and 25 ms for the most accurate results. It
 * can be higher though if needed. The LED times will be truncated a little.
 * 
 * @param tickMs  how often you plan to call the ErrorCode Tick function
 */
void ErrorCode_InitMs(uint16_t tickMs);

/***************************************************************************//**
 * @brief Set an error code
 * 
 * @param code  error code 1 to 32
 */
void ErrorCode_Set(uint8_t code);

/***************************************************************************//**
 * @brief Clear an error code
 * 
 * @param code  error code 1 to 32
 */
void ErrorCode_Clear(uint8_t code);

/***************************************************************************//**
 * @brief Clear all error codes
 * 
 */
void ErrorCode_ClearAll(void);

/***************************************************************************//**
 * @brief Check if an error code has been set
 * 
 * @param code  error code 1 to 32
 * 
 * @return true  error code set
 */
bool ErrorCode_IsSet(uint8_t code);

/***************************************************************************//**
 * @brief Updates the state machine and LED output
 * 
 * The LED will flash to show the number of the error code it is currently on. 
 * When it is finished, it will insert a small pause to make it easier for the
 * customer to see where one error code stops and the next begins. Then it will
 * look for the next active error code and begin flashing the LED for that one.
 */
void ErrorCode_Tick(void);

/***************************************************************************//**
 * @brief Get the error code currently being flashed
 * 
 * @return uint8_t  the current error code (1 to 32)
 */
uint8_t ErrorCode_GetCurrentCode(void);

/***************************************************************************//**
 * @brief Set the output of an LED
 * 
 * @return true  set the output of your LED on. Else, set it off
 */
bool ErrorCode_GetBlinkOutput(void);

/***************************************************************************//**
 * @brief Stop the output
 * 
 * Does not change the current error code or clear any error codes. Behavior 
 * can be changed to stop immediately, stop on next off state, or stop when
 * finished.
 */
void ErrorCode_Stop(void);

/***************************************************************************//**
 * @brief Start the output
 * 
 * Restarts at the beginning of the current error code
 */
void ErrorCode_Start(void);

/***************************************************************************//**
 * @brief Check if the state machine is running
 * 
 * @return true  if running
 */
bool ErrorCode_IsRunning(void);

/***************************************************************************//**
 * @brief Get error code mask
 * 
 * Returns all the error codes in a 64-bit word. Remember that the bits are
 * numbered 0 through 63, but the error codes are 1 through 64.
 * 
 * @return uint64_t  a bit mask. 1 = error code set, 0 = error code cleared
 */
uint64_t ErrorCode_GetActiveMask(void);

/***************************************************************************//**
 * @brief Get a range of error codes as a mask
 * 
 * Returns the range of error codes you specify as a 32-bit word. Remember that 
 * the bits are numbered 0 through 63, but the error codes are 1 through 64.
 * 
 * @param errorCodeEnd  the error code of the end of the range
 * 
 * @param errorCodeStart  the error code of the start of the range
 * 
 * @return uint32_t  result (truncated if larger than 32)
 */
uint32_t ErrorCode_GetActiveMaskRange(uint8_t errorCodeEnd, uint8_t errorCodeStart);

/***************************************************************************//**
 * @brief Change behavior for flashing the top n number of codes
 * 
 * The default behavior is to go through every error code and flash each one
 * with a pause in between them. By setting this value less than the maximum, 
 * you can control how many error codes are flashed. The priority of error 
 * codes is controlled by the SetPriorityLevel function.
 * 
 * For example, if you only want to blink one error code to the customer set 
 * this value to 1. If error codes 3 and 5 are both active, then only the one 
 * with the highest priority (lowest number) gets displayed.
 * 
 * A value of 0 is not allowed. If you set it to zero, it will be set to 1.
 * A value greater than the maximum will enable all.
 * 
 * @see ErrorCode_SetPriorityLevel
 * 
 * @param displayNumErrorCodes  flash up to 1 to 32 codes. (32 is default) 
 */
void ErrorCode_SetDisplayTopNumOfCodes(uint8_t displayNumErrorCodes);

/***************************************************************************//**
 * @brief Change the priority of an error code
 * 
 * If multiple error codes are to be displayed, the ones with lower priority 
 * numbers get flashed first. The default priority number is 64. 
 * 
 * For example, change error code 1 to be priority 65 and set 
 * SetDisplayTopNumOfCodes to 1. If error code 1 is the only error code, it 
 * will be displayed. If error codes 1 and 3 are active, only error code 3 gets 
 * displayed. Increasing the number of error codes to display will allow both 
 * to be displayed. First error 3, then error code 1.
 * 
 * If you change two error codes to the same priority number, they will be 
 * displayed in the order in which you changed the priority level. For example, 
 * if you set error codes 5 to priority 1 and then error code 4 to priority 1, 
 * error code 5 gets displayed first, then 4.
 * 
 * @param code  error code 1 to 32
 * 
 * @param priority  priority number (64 is default)
 */
void ErrorCode_SetPriorityLevel(uint8_t code, uint8_t priority);

#endif /* ERROR_CODE_H */