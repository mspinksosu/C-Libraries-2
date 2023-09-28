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
#define LCG_M               (1ULL << 63)
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
#define PM_DEFAULT_SEED     1UL

/* Precomputed values for Schrage's method. It uses the same multiplier and 
modulus as the 32-bit Park Miller */
#define SCH_M               ((1UL << 31) - 1UL)
#define SCH_A               48271UL
#define SCH_Q               44488 // Q = M / A
#define SCH_R               3399  // R = M % A

#define DEBUG_PRINT         true

#if DEBUG_PRINT
#include <stdio.h>
#endif

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

    /* TODO Possible output values should be in the range 0 to 2^32-1. But 
    output is not full-cycle */

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

uint32_t PRNG_LCGBounded(LCG *self, uint32_t lower, uint32_t upper)
{
    /* TODO I'll probably end up combining all these different kinds of 
    generators into a class since this function is almost always the same. */
    uint32_t result = 0;

    if(lower > upper)
    {
        uint32_t temp = lower;
        lower = upper;
        upper = lower;
    }

    // output = output % (upper - lower + 1) + min
    uint32_t range = upper - lower;
    if(range < 0xFFFFFFFF)
        range++;

    /* @debug method for removing modulo bias */
    /* threshold = RAND_MAX - RAND_MAX % range */
    uint32_t threshold = 0xFFFFFFFF - 0xFFFFFFFF % range;
    do {
        result = PRNG_LCGNext(self);
    } while(result >= threshold);

    return (result % range) + lower;
}

// *****************************************************************************

uint32_t PRNG_LCGSkip(LCG *self, int64_t n)
{
    /* Given the seed, compute the nth term by skipping ahead logarithmically. 
    This algorithm will complete in O(log2(n)) operations instead of O(n).

    Knuth, Art of Computer Programming 3.2.1 Equation (6):
    X_n+k = (a^k * X_n + c(a^k - 1) / (a - 1)) % m 
    multiplier: a^k % m 
    increment: (c(a^k -1) / (a - 1)) % m

    Brown, Random Number Generation With Arbitrary Strides 1994:
    Used in random number generation for "Monte Carlo" calculations. The same 
    formula "X_n+k = (A * X_n + C) % m" still applies. The psuedo code below is 
    basically just a method to compute the formula cited above by Donald Knuth.
    psuedo code for A:
    A = 1, h = a, i = k + 2^m % 2^m 
    while(i > 0) {
        if( i = odd)
            A = (A * h) % 2^m 
        h = (h^2) % 2^m
        i = floor(i / 2) }
    psuedo code for C:
    C = 0, f = c, h = a, i = (k + 2^m) % 2^m 
    while(i > 0) {
        if( i = odd)
            C = (C * h + f) % 2^m
        f = (f * (h + 1)) % 2^m
        h = (h^2) % 2^m
        i = floor(i / 2) } */

    /* Compute i (skipAhead). If number to skip is negative, add the period 
    until it is postive. Skipping backwards is the same as skipping forward 
    that many times. */
    int64_t skipAhead = n;
    while(skipAhead < 0)
        skipAhead += LCG_M;
    skipAhead = skipAhead & LCG_MASK;

    uint64_t A = 1, h = LCG_A, C = 0, f = LCG_C;
#if DEBUG_PRINT
    uint32_t loopCount = 0;
#endif
    /* Now compute A and C. */
    for(; skipAhead > 0LL; skipAhead >>= 1)
    {
        if(skipAhead & 1LL)
        {
            A = (A * h) & LCG_MASK;
            C = (C * h + f) & LCG_MASK;
        }
        f = (f * h + f) & LCG_MASK;
        h = (h * h) & LCG_MASK;
#if DEBUG_PRINT
        loopCount++;
#endif
    }

#if DEBUG_PRINT
    printf("Number of iterations: %llu\n", loopCount);
#endif
    self->state = (A * self->state + C) & LCG_MASK;
    return (uint32_t)(self->state >> 30ULL);
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
    number and c = 0. I believe the output values should be in the range of 
    1 to m - 1. */

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

uint32_t PRNG_ParkMillerBounded(ParkMiller *self, uint32_t lower, uint32_t upper)
{
    /* TODO I'll probably end up combining all these different kinds of 
    generators into a class since this function is almost always the same. */
    uint32_t result = 0;

    if(lower > upper)
    {
        uint32_t temp = lower;
        lower = upper;
        upper = lower;
    }

    // output = output % (upper - lower + 1) + min
    uint32_t range = upper - lower;
    if(range < 0x7FFFFFFF)
        range++;

    /* @debug method for removing modulo bias */
    /* threshold = RAND_MAX - RAND_MAX % range */
    uint32_t threshold = 0x7FFFFFFF - 0x7FFFFFFF % range;
    do {
        result = PRNG_ParkMillerNext(self);
    } while(result >= threshold);

    return (result % range) + lower;
}

// *****************************************************************************

uint32_t PRNG_ParkMillerSkip(ParkMiller *self, int64_t n)
{
    /* This is the exact same as the LCG skip ahead formula, except that this
    time I don't calculate C. And since m is a prime number and not a power of 
    two, I can't reduce the modulo operation.

    X_n+1 = (A * X_n + C) % m
    X_n+k = (a^k * X_n + c(a^k - 1) / (a - 1)) % m 
    multiplier: a^k % m 
    increment: (c(a^k -1) / (a - 1)) % m */

    /* Compute i (skipAhead). If number to skip is negative, add the period 
    until it is postive. Skipping backwards is the same as skipping forward 
    that many times. */
    int64_t skipAhead = n;
    while(skipAhead < 0)
        skipAhead += PM_M;
    skipAhead = skipAhead % PM_M;

    uint64_t A = 1, h = PM_A;
#if DEBUG_PRINT
    uint32_t loopCount = 0;
#endif
    /* Now compute A */
    for(; skipAhead > 0LL; skipAhead >>= 1)
    {
        if(skipAhead & 1LL)
        {
            A = (A * h) % PM_M;
        }
        h = (h * h) % PM_M;
#if DEBUG_PRINT
        loopCount++;
#endif
    }

#if DEBUG_PRINT
    printf("Number of iterations: %llu\n", loopCount);
#endif
    self->state = (A * self->state) % PM_M;
    return (uint32_t)(self->state);
}

// *****************************************************************************

void PRNG_SchrageSeed(Schrage *self, uint32_t seed)
{
    /* The initial value X_0 must be co-prime to m. If m is chosen to be a 
    prime number, than any value from 0 < X_0 < m will work. */
    if(seed == 0)
        seed = PM_DEFAULT_SEED;

    self->state = seed;
    self->isSeeded = true;
}

// *****************************************************************************

uint32_t PRNG_SchrageNext(Schrage *self)
{
    int32_t result;
    uint32_t X = self->state;
    int32_t X_Div_Q, X_Mod_Q;
    /* Schrage's method is a version of a Park Miller that avoids the need to 
    use a 64-bit variable to store the product of a * x. For any integer "m" 
    and "a > 0" there exists unique integers "q" (quotient) and "r" (remainder) 
    such that "m = a * q + r" and "0 <= r < m". 
    
    "Then q = m / a (integer division) and "r = m % a". The product a*x is 
    approximately: a*x = a(x % q) - r[x / q] (integer division). Then we can 
    take the result of a*x and perform % m to it. This modulo m is further 
    simplified: if a*x = a(x % q) - r[x / q] is negative, m is added to it. */

    if(self->isSeeded == false)
    {
        self->state = PM_DEFAULT_SEED;
        self->isSeeded = true;
    }

    /* ax % m = a(x % q) - r[x / q] % m */
    X_Div_Q = X / SCH_Q;
    X_Mod_Q = X - X_Div_Q * SCH_Q;
    result = SCH_A * X_Mod_Q - SCH_R * X_Div_Q;
    if(result < 0)
        result += SCH_M;

    self->state = result;
    return result;
}

// *****************************************************************************

uint32_t PRNG_SchrageBounded(Schrage *self, uint32_t lower, uint32_t upper)
{
    /* TODO I'll probably end up combining all these different kinds of 
    generators into a class since this function is almost always the same. */
    uint32_t result = 0;

    if(lower > upper)
    {
        uint32_t temp = lower;
        lower = upper;
        upper = lower;
    }

    /* output = output % (upper - lower + 1) + min */
    uint32_t range = upper - lower;
    if(range < 0x7FFFFFFF)
        range++;

    /* @debug method for removing modulo bias */
    /* threshold = RAND_MAX - RAND_MAX % range */
    uint32_t threshold = 0x7FFFFFFF - 0x7FFFFFFF % range;
    do {
        result = PRNG_SchrageNext(self);
    } while(result >= threshold);

    return (result % range) + lower;
}

/*
 End of File
 */