/*
 * R A N D O M . H P P
 * (c) Charybdis Limited, 1998. All Rights Reserved
 */

/*
    MexRandom

    Define a random number generator and accessor functions for
    getting the results of that generator in different forms.
*/

#ifndef _MATHEX_RANDOM_HPP
#define _MATHEX_RANDOM_HPP

#include "base/base.hpp"

#include "mathex/mathex.hpp"

class MexBasicRandom
// Memberwise canonical
{
public:
    MexBasicRandom( void );
    ~MexBasicRandom( void );

    //  The client should not call this method directly, the
    //  client should be using the accessor functions.
    int next();
    //  POST( 0 <= result and result < upperLimit() );

    static int upperLimit();

    void    seed( ulong newSeed );
    void    seedFromTime();

    //  Return the seed used for this RNG
    ulong   seed() const;

    //  Construct a random number generator seeded by the
    //  time and return it
    static  MexBasicRandom constructSeededFromTime();

    void CLASS_INVARIANT;

private:
    friend ostream& operator <<( ostream& o, const MexBasicRandom& t );
    friend class SeedRecorder;

    //  Used because of the poor resolution of the clock - ensures every call
    //  to seedFromTime will use a different seed.
    static  size_t  seedIncrement();

    //  Fixed-width (uint32 == unsigned int, 32-bit on both LP64/macOS and LLP64/Windows).
    //  Previously ulong, which is 64-bit on macOS but 32-bit on Windows, so the LCG below
    //  (state_ = state_ * 1103515245 + 12345) wrapped at different widths and produced a
    //  different sequence per platform - a determinism/desync source. uint32 wraps mod 2^32
    //  identically everywhere.
    uint32 state_;
    uint32 seed_;
};

//  Acccessor functions for getting the results of a
//  generator in different forms

template< class RANDOM >
MATHEX_SCALAR mexRandomScalar( RANDOM* pR, MATHEX_SCALAR lowerLimit, MATHEX_SCALAR upperLimit );
//  PRE( lowerLimit < upperLimit );
//  POST( lowerLimit <= result and result < upperLimit );

template< class RANDOM >
int mexRandomInt( RANDOM* pR, int lowerLimit, int upperLimit );
//  PRE( lowerLimit < upperLimit );
//  POST( lowerLimit <= result and result < upperLimit );

template< class RANDOM >
int mexRandomInt( RANDOM* pR, int upperLimit );
//  PRE( 0 < upperLimit );
//  POST( 0 <= result and result < upperLimit );

#endif

//#ifdef _INSTANTIATE_TEMPLATE_FUNCTIONS
    #include "mathex/random.ctf"
//#endif

/* End RANDOM.HPP ***************************************************/
