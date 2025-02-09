/***************************************************************************//**
 * @brief PID Library
 * 
 * @file PID.c
 * 
 * @author Matthew Spinks <https://github.com/mspinksosu>
 * 
 * @date 12/17/22  Original Creation
 * 
 * @details
 *      // TODO Work in progress. Ready to test
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

#include "PID.h"

// ***** Defines ***************************************************************

#define DEFAULT_I_REDUCE_FACTOR 0.05

// ***** Global Variables ******************************************************


// ***** Static Functions Prototypes *******************************************


// *****************************************************************************

void PID_Create(PID *self, float Kp, float Ki, float Kd, float min, float max)
{
    self->Kp = Kp;
    self->Ki = Ki;
    self->Kd = Kd;
    self->min = min;
    self->max = max;
    self->iReductionFactor = DEFAULT_I_REDUCE_FACTOR;
    self->integral = 0;
    self->prevError = 0;
    self->enable = false;
}

// *****************************************************************************

void PID_AdjustSetPoint(PID *self, float setPoint)
{
    self->setPoint = setPoint;
}

// *****************************************************************************

float PID_Compute(PID *self, float processVariable)
{
    if(self->enable == false)
        return;

    float error = self->setPoint - processVariable;
    float derivative = error - self->prevError;

    /* Limit output and integral windup. Sometimes clamping the integral term 
    completely causes negative side effects. I'm going to use an extra term to 
    adjust how it gets dampened. Setting it to 0.0 will clamp the integral 
    hard. Setting it to a number greater than 0.0 will allow a little bit of 
    overshoot. Setting it to 1.0 will apply no clamping.
    @follow-up Try a value of 0.05 to 0.25 */
    if(self->controlVariable >= self->max)
    {
        self->controlVariable = self->max; // limit output
        self->integral += error * self->iReductionFactor; // dampen integral
    }
    else if(self->controlVariable <= self->min)
    {
        self->controlVariable = self->min; // limit output
        self->integral += error * self->iReductionFactor; // dampen integral
    }
    else
    {
        self->integral += error; // Compute integral normally
    }

    /* Compute the new output. Output = P_Term + I_Term + D_Term */
    self->controlVariable = (self->Kp * error) + (self->Ki * self->integral) +
                            (self->Kd * derivative);
    
    /* Store previous error for the derivative term */
    self->prevError = error;

    return self->controlVariable;
}

// *****************************************************************************

float PID_GetOutput(PID *self)
{
    return self->controlVariable;
}

// *****************************************************************************

float PID_AdjustConstants(PID *self, float Kp, float Ki, float Kd)
{
    self->Kp = Kp;
    self->Ki = Ki;
    self->Kd = Kd;
}

// *****************************************************************************

void PID_Enable(PID *self)
{
    self->enable = true;
}

// *****************************************************************************

void PID_Disable(PID *self)
{
    self->enable = false;
    self->integral = 0;
    self->prevError = 0;
}

// *****************************************************************************

void PID_AdjustIReductionFactor(PID *self, float r)
{
    self->iReductionFactor = r;
}

/*
 End of File
 */