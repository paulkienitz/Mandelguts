public class PointCalculator
{
    // there is no standard Complex class, so let's make a rudimentary
    // one that can add and multiply
    public static class Complex
    {
        private double re, im;
        
        public Complex()                   { re = im = 0.0; }
        public Complex(double r, double i) { re = r; im = i; }
        public Complex(Complex other)      { re = other.re; im = other.im; }
        
        public double real()               { return re; }
        public double imag()               { return im; }
        
        public Complex add(Complex other)
        {
            re += other.re;
            im += other.im;
            return this;
        }
        public Complex sub(Complex other)
        {
            re -= other.re;
            im -= other.im;
            return this;
        }
        public Complex mul(Complex other)
        {
            double tre = re * other.re - im * other.im;
            im = re * other.im + im * other.re;
            re = tre;
            return this;
        }
        public double norm()
        {
            return re * re + im * im;       // abs() would be Math.sqrt(norm())
        }
    }


    public static long total_loopchecks = 0;

    // The lowest level functions for calculating mandelbrot values.
    // These functions are not members of any C++ class.

    public static int TenBitMagnitude(double d)
    {
        if (d < 1.0e-28)
            return 0;
        // input range is 1.0e-28 to about 10.0, output 1 to 1023, so:
        return (int) Math.pow(2.0, (Math.log10(d) + 28.0) / 3.0);
    }

    public static boolean ReallyClose(Complex a, Complex b)
    {
    //  Complex epsilon = a * 1.0e-14;
        return Math.abs(a.real() - b.real()) <= 1.0e-15 &&  // epsilon.real() &&
               Math.abs(a.imag() - b.imag()) <= 1.0e-15;    // epsilon.imag();
        // use of 1e-14 produces scallopy artifacts; 1e-15 produces stripes of dust;
        // CHECK_WITH_EQUALITY produces fairly shapeless dust patterns
    }

    // The result of this may consist of bitfields.  Bits 0-19 are the iteration count,
    // bits 20-29 are the TenBitMagnitude of the loop diameter, bit 30 is a flag
    // meaning that the point is still inconclusive.  The above only apply if bit 31
    // is zero; if it's set, the whole number is the negative of an iteration count.
    public static int DoMandelPoint(Complex addpt, Complex point, int limit, int loopmax,
                                    boolean loop_length_only, Complex[] loopcheck)
    {
        int c = 0;
        boolean maybeloop = false;
        Complex current = point;
        do {
            Complex lastcurrent = new Complex(current);
            current.mul(current);
            current.add(addpt);
            if (current.norm() > 4.0) {
                point = current;
                return Math.min(-c, -1);             // outside the set
            }
            if (loopmax > 0 && c > 0 && (c % loopmax == 0
                                        )) {
                int j;
                for (j = Math.min(c, loopmax) - 1; j > 0; j--) {
                    if (ReallyClose(current, loopcheck[j]) && ReallyClose(lastcurrent, loopcheck[j - 1])) {
                        if (!maybeloop) {
                            maybeloop = true;
                            break;      // check again after one more batch of iterations
                        }
                        point = current;        // remember one point on the loop
                        if (loop_length_only)
                            return loopmax - j;     // found attractor: return length of loop
                        else {
                            int k;
                            double diameter2 = 0.0;
                            Complex furthest = current;
                            for (k = loopmax - 1; k >= j; k--) {
                                Complex ct = new Complex(loopcheck[k]);
                                ct.sub(current);
                                double diam = ct.norm();
                                if (diam > diameter2) {     // find loop point furthest from current
                                    diameter2 = diam;
                                    furthest = loopcheck[k];
                                }
                            }
                            for (k = loopmax - 1; k >= j; k--) {
                                Complex ct = new Complex(loopcheck[k]);
                                ct.sub(furthest);
                                double diam = ct.norm();
                                if (diam > diameter2)       // find loop point furthest from that one
                                    diameter2 = diam;
                            }
                            // With luck, diameter2 now usually reflects the distance
                            // between the two furthest apart points in the loop.
                            // assert loopmax < 1<<20
                            return (loopmax - j) | TenBitMagnitude(diameter2) << 20;
                        }
                    }
                    total_loopchecks++;
                }
            }
            loopcheck[c % loopmax] = new Complex(current);
        } while (++c <= limit);
        point = current;                        // remember where we left off
        return limit | 0x40000000;              // too tough for me
    }

}