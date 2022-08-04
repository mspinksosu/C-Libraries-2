/***************************************************************************//**
 * @brief GPIO Interface
 * 
 * @author Matthew Spinks
 * 
 * @date February 6, 2022  Original creation
 * 
 * @file IGPIO.c
 * 
 * @details
 *      The counterpart for the GPIO interface library. Functions here are
 * called using the base class object. When you use the create function, you
 * are linking a base class and a subclass together. This file will use the
 * interface and your base class object to determine which function
 * implementation to call and which subclass object to give to it.
 * 
 ******************************************************************************/

#include "IGPIO.h"

// ***** Defines ***************************************************************


// ***** Function Prototypes ***************************************************


// ***** Global Variables ******************************************************

static GPIOInterface *interface;

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// ***** Non-Interface Functions *********************************************//
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

void GPIO_DriverSetInterface(GPIOInterface *thisInterface)
{
    interface = thisInterface;
}

void GPIO_Create(GPIO *self, void *instanceOfSubClass)
{
    self->instance = instanceOfSubClass;
}

void GPIO_CreateInitType(GPIOInitType *params, void *instanceOfSubClass)
{
    params->instance = instanceOfSubClass;
}

void GPIO_SetPointerToAnalogValue(GPIO *self, uint16_t *ptr)
{
    self->ptrToAnalogValue = ptr;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// ***** Interface Functions *************************************************//
//                                                                            //
////////////////////////////////////////////////////////////////////////////////


void GPIO_InitPin(GPIO *self, GPIOInitType *params)
{
    if(interface->GPIO_InitPin != NULL && self->instance != NULL 
            && params->instance != NULL)
    {
        (interface->GPIO_InitPin)(self->instance, params->instance);
    }
}


void GPIO_SetPin(GPIO *self)
{
    if(interface->GPIO_SetPin != NULL && self->instance != NULL)
    {
        (interface->GPIO_SetPin)(self->instance);
    }
}


void GPIO_ClearPin(GPIO *self)
{
    if(interface->GPIO_ClearPin != NULL && self->instance != NULL)
    {
        (interface->GPIO_ClearPin)(self->instance);
    }
}


void GPIO_InvertPin(GPIO *self)
{
    if(interface->GPIO_InvertPin != NULL && self->instance != NULL)
    {
        (interface->GPIO_InvertPin)(self->instance);
    }
}


void GPIO_WritePin(GPIO *self, bool setPinHigh)
{
    if(interface->GPIO_WritePin != NULL && self->instance != NULL)
    {
        (interface->GPIO_WritePin)(self->instance, setPinHigh);
    }
}


uint16_t GPIO_ReadPin(GPIO *self)
{
    if(interface->GPIO_ReadPin != NULL && self->instance != NULL)
    {
        return (interface->GPIO_ReadPin)(self->instance);
    }
    else
    {
        return 0;
    }
}


bool GPIO_ReadBool(GPIO *self)
{
    if(interface->GPIO_ReadBool != NULL && self->instance != NULL)
    {
        return (interface->GPIO_ReadBool)(self->instance);
    }
    else
    {
        return false;
    }
}


void GPIO_SetType(GPIO *self, GPIOType type)
{
    if(interface->GPIO_SetType != NULL && self->instance != NULL)
    {
        (interface->GPIO_SetType)(self->instance, type);
    }
}


GPIOType GPIO_GetType(GPIO *self)
{
    if(interface->GPIO_GetType != NULL && self->instance != NULL)
    {
        return (interface->GPIO_GetType)(self->instance);
    }
    else
    {
        return 0;
    }
}


void GPIO_SetPull(GPIO *self, GPIOPull pullType)
{
    if(interface->GPIO_SetPull != NULL && self->instance != NULL)
    {
        (interface->GPIO_SetPull)(self->instance, pullType);
    }
}


GPIOPull GPIO_GetPull(GPIO *self)
{
    if(interface->GPIO_GetPull != NULL && self->instance != NULL)
    {
        return (interface->GPIO_GetPull)(self->instance);
    }
    else
    {
        return 0;
    }
}

/*
 End of File
*/