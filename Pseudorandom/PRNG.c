/***************************************************************************//**
 * @brief PRNG Library
 * 
 * @file PRNG.c
 * 
 * @author Matthew Spinks <https://github.com/mspinksosu>
 * 
 * @date 9/23/23   Original creation
 * 
 * @details
 *      TODO
 * 
 * @section license License
 * SPDX-FileCopyrightText: © 2023 Matthew Spinks
 * SPDX-License-Identifier: Zlib
 * 
 * This software is released under the Zlib license. You are free alter and
 * redistribute it, but you must not misrepresent the origin of the software.
 * This notice may not be removed. <http://www.zlib.net/zlib_license.html>
 * 
 ******************************************************************************/

#include "PRNG.h"

// ***** Defines ***************************************************************

/* Using this LCG requires 64-bit math. Modulus m = 2^63 */
#define LCG_MASK            ((1ULL << 63) - 1ULL)
#define LCG_A               3249286849523012805ULL
#define LCG_C               1ULL
/* m and c must be relatively prime, so c = 1 is commonly chosen */
#define LCG_DEFAULT_SEED    1ULL

/* For the Park Miller, modulus m is chosen to be a prime number. */
#if PM_USE_DOUBLE_WIDTH_64_PRODUCT
    #define PM_M            ((1ULL << 63) - 25ULL)
    #define PM_A            6458928179451363983ULL
#else
    /* This is the default value for C++ minstd_rand */
    #define PM_M            ((1UL << 31) - 1UL) 
    #define PM_A            48271UL
#endif
/* Park Miller seed must be 0 < X_0 < m */
#define PM_DEFAULT_SEED     1ULL

// ***** Global Variables ******************************************************


// ***** Static Functions Prototypes *******************************************


// *****************************************************************************

void PRNG_LCGSeed(LCG *self, uint32_t seed)
{
    if(seed == 0)
        seed = LCG_DEFAULT_SEED;

    self->state = seed;
    self->isSeeded = true;
}

// *****************************************************************************

uint32_t PRNG_LCGNext(LCG *self)
{
    /* TODO This version will use a power of two for the modulus for speed with
    the lower bits removed. Similar to C rand, but with 32-bit result. 
    Multiplier a will be chosen from L'Ecuyer research paper. Increment c 
    will need to be odd. Try with c = 1. */

    /* X_n+1 = (a * X_n + c) % m */
    if(self->isSeeded == false)
    {
        self->state = LCG_DEFAULT_SEED;
        self->isSeeded = true;
    }

    self->state = (LCG_A * self->state + LCG_C) & LCG_MASK;
    return (uint32_t)(self->state >> 30ULL);
}

// *****************************************************************************

uint32_t PRNG_LCGSkipAhead(LCG *self, uint32_t skip)
{
    return 0;
}

// *****************************************************************************

void PRNG_ParkMillerSeed(ParkMiller *self, uint32_t seed)
{
    /* The initial value X_0 must be co-prime to m. If m is chosen to be a 
    prime number, than any value from 0 < X_0 < m will work. */
    if(seed == 0)
        seed = PM_DEFAULT_SEED;

    self->state = seed;
    self->isSeeded = true;
}

// *****************************************************************************

uint32_t PRNG_ParkMillerNext(ParkMiller *self)
{
    /* TODO This version will be a full-cycle PRNG with a modulus of a prime 
    number and c = 0. */

    /* X_n+1 = (a * X_n) % m */
    if(self->isSeeded == false)
    {
        self->state = PM_DEFAULT_SEED;
        self->isSeeded = true;
    }

    self->state = (PM_A * self->state) % PM_M;
#if PM_USE_DOUBLE_WIDTH_64_PRODUCT
    return (uint32_t)(self->state >> 31ULL);
#else
    return (uint32_t)(self->state);
#endif
}

// *****************************************************************************

uint32_t PRNG_ParkMillerSkipAhead(ParkMiller *self, uint32_t skip)
{
    return 0;
}

/*
 End of File
 */