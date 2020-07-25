// ARRRRGH -- somehow, I can't explain how, this ends up being a good deal SLOWER
// than the same code in Java!  Wtf, compiler??  (Debug vs release optimization
// only makes like a ten percent difference, btw.)

#include <math.h>
#include <stdlib.h>

#ifndef max
  #define max __max
  #define min __min
#endif

#ifdef BENCH
    __int64 total_loopchecks = 0;
#endif

// The lowest level functions for calculating mandelbrot values.
// These functions are not members of any C++ class.


static inline int TenBitMagnitude(double d)
{
    if (d < 1.0e-28)
        return 0;
    // input range is 1.0e-28 to about 10.0, output 1 to 1023, so:
    return (int) pow(2.0, (log10(d) + 28.0) / 3.0);
}

#ifdef EPSILON
static inline bool ReallyClose(Complex& a, Complex& b)
{
    return fabs(a.real() - b.real()) <= EPSILON &&  // epsilon.real() &&
           fabs(a.imag() - b.imag()) <= EPSILON;    // epsilon.imag();
    // EPSILON of 1e-14 produces scallopy artifacts; 1e-15 produces stripes of dust;
    // smaller values or no EPSILON produces dust patterns that are shapeless but
    // pervasive; fat values like 1e-13 produce clean lack of artifacts on large scale
    // a mess at small scale
}
#endif

#ifdef POWA_2
static inline IsPowerOf2(unsigned c)
{
    for (int x = 0; x < 8 * sizeof(int); x++) {
        int v = c & ~(1 << x);
        if (v == 0)
            return true;
        if (v != c)
            return false;
    }
    return false;
}
#endif


// called by worker thread, from CalculateAt
//
// The result of this may consist of bitfields.  Bits 0-19 are the iteration count,
// bits 20-29 are the TenBitMagnitude of the loop diameter, bit 30 is a flag
// meaning that the point is still inconclusive.  The above only apply if bit 31
// is zero; if it's set, the whole number is the negative of an iteration count.
static inline long DoMandelPoint(Complex addpt, Complex& point, int limit, int loopmax,
                                 bool loop_length_only, Complex* loopcheck)
{
    int c = 0;
    bool maybeloop = false;
    Complex current = point;
    do {
#ifdef EPSILON
        Complex lastcurrent = current;
#endif
        current *= current;
        current += addpt;
        if (norm(current) > FOUR) {
            point = current;
            return min(-c, -1);             // outside the set
        }
        if (loopmax > 0 && c > 0 && (c % loopmax == 0
#ifdef POWA_2
                                     || (c % 32 == 0 && c < loopmax && IsPowerOf2(c))
#endif
                                    )) {
            int j;
#ifndef EPSILON
            for (j = min(c, loopmax) - 1; j >= 0; j--) {
                if (current == loopcheck[j]) {
#else
            for (j = min(c, loopmax) - 1; j > 0; j--) {
                if (ReallyClose(current, loopcheck[j]) && ReallyClose(lastcurrent, loopcheck[j - 1])) {
#endif
                    if (!maybeloop) {
                        maybeloop = true;
                        break;      // check again after one more batch of iterations
                    }
                    point = current;        // remember one point on the loop
#ifdef RETURN_LOOP_DELAY
                    int k;
                    for (k = j; k >= 0 && loopcheck[k] == current; k--) ;
                    return c - (j - k);     // found an attractor in the set
#else
                    if (loop_length_only)
                        return loopmax - j;     // found attractor: return length of loop
                    else {
                        int k;
                        double diameter2 = 0.0;
                        Complex furthest = current;
                        for (k = loopmax - 1; k >= j; k--) {
                            double diam = norm(loopcheck[k] - current);
                            if (diam > diameter2) {     // find loop point furthest from current
                                diameter2 = diam;
                                furthest = loopcheck[k];
                            }
                        }
                        for (k = loopmax - 1; k >= j; k--) {
                            double diam = norm(loopcheck[k] - furthest);
                            if (diam > diameter2)       // find loop point furthest from that one
                                diameter2 = diam;
                        }
                        // With luck, diameter2 now usually reflects the distance
                        // between the two furthest apart points in the loop.
                        // assert loopmax < 1<<20
                        return (loopmax - j) | TenBitMagnitude(diameter2) << 20;
                    }
#endif
                }
#ifdef BENCH
                total_loopchecks++;
#endif
            }
        }
        loopcheck[c % loopmax] = current;
    } while (++c <= limit);
    point = current;                        // remember where we left off
    return limit | 0x40000000;              // too tough for me
}
