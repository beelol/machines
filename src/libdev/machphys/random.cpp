/*
 * R A N D O M . C P P
 * (c) Charybdis Limited, 1997. All Rights Reserved
 */

//  Definitions of non-inline non-template methods and global functions

#include "machphys/random.hpp"

#include "mathex/random.hpp"

//  A single shared pseudo-random stream for all gameplay randomness. Previously every call
//  here used libc rand(), which is unseeded and platform-dependent - the algorithm and
//  RAND_MAX differ between macOS, glibc and MinGW, so identical call sequences produced
//  different numbers on different machines, an immediate cross-platform desync source.
//  Routing everything through one portable MexBasicRandom (a 32-bit LCG) lets a network game
//  seed every peer identically (see MachPhysRandom::seed / the START_GAME handshake).
static MexBasicRandom& sharedRandom()
{
    static MexBasicRandom gen = MexBasicRandom::constructSeededFromTime();
    return gen;
}

//  next() yields 15 bits (0 .. upperLimit()-1 == 0 .. 32767). Combine two draws for the
//  size_t overloads so large ranges are not silently truncated to 15 bits. Deterministic:
//  the same two calls happen on every platform.
static uint32 sharedRandom30()
{
    const uint32 hi = _STATIC_CAST( uint32, sharedRandom().next() );   // 15 bits
    const uint32 lo = _STATIC_CAST( uint32, sharedRandom().next() );   // 15 bits
    return ( hi << 15 ) | lo;                                          // 30 bits
}

MachPhysRandom::MachPhysRandom()
{

    TEST_INVARIANT;
}

MachPhysRandom::~MachPhysRandom()
{
    TEST_INVARIANT;

}

//  static
void MachPhysRandom::seed( uint32 newSeed )
{
    sharedRandom().seed( newSeed );
}

//  static
double MachPhysRandom::randomDouble( MATHEX_SCALAR lowerLimit, MATHEX_SCALAR upperLimit )
{
    PRE( lowerLimit <= upperLimit );

    //  Scale a single draw to [0,1] exactly as the old rand()/RAND_MAX did.
    const double unitRandom = _STATIC_CAST( double, sharedRandom().next() )
                            / _STATIC_CAST( double, MexBasicRandom::upperLimit() - 1 );
    double result = lowerLimit + unitRandom * ( upperLimit - lowerLimit );

    POST( lowerLimit <= result and result <= upperLimit );

    return result;
}

// static
int MachPhysRandom::randomInt( int lowerLimit, int upperLimit )
{
    PRE_INFO( lowerLimit );
    PRE_INFO( upperLimit );
    PRE( lowerLimit < upperLimit );

    int result = lowerLimit + sharedRandom().next() % ( upperLimit - lowerLimit );

    POST( lowerLimit <= result and result < upperLimit );

    return result;
}

// static
size_t MachPhysRandom::randomInt( size_t lowerLimit, size_t upperLimit )
{
    PRE( lowerLimit <= upperLimit );

    size_t result = lowerLimit + sharedRandom30() % ( upperLimit - lowerLimit );

    POST( lowerLimit <= result and result < upperLimit );

    return result;
}

// static
size_t MachPhysRandom::randomInt( size_t upperLimit )
{
    size_t result = sharedRandom30() % upperLimit;

    POST( result < upperLimit );

    return result;
}

void MachPhysRandom::CLASS_INVARIANT
{
}


/* End RANDOM.CPP ***************************************************/
