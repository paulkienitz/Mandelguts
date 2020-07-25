//  mandel_bench.cpp: a timing benchmark that does mandelbrot calculations

#include <math.h>
#include <complex.h>
#include <iostream>

// The functions defined in point.cpp are inline, so we #include them instead of linking:
#define BENCH
#include "point.cpp"

extern __int64 total_loopchecks;

long CalculateRange(int limit, int loopmax, double l, double t, double r, double b, int w, int h)
{
    int x, y, pc = 0;
    long z = 0;
    Complex* loopcheck;
    loopcheck = new Complex[loopmax];
    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            Complex p0(l + (x + 0.5) * (r - l) / w, t + (y + 0.5) * (b - t) / h);
            Complex p(p0);
            z ^= DoMandelPoint(p0, p, limit, loopmax, true, loopcheck);
            // without something like this, the foreground process gets stuck under 95/98/ME:
//          if (x % 16 == 0)
//              ::Sleep(0);     // end this time slice, run other threads including painter
        }
        cout << y << "\r" << flush;
    }
    delete[] loopcheck;
    return z;
}


int main()
{
    double l = -2.75, r = 1.75;
    int w = 1920, h = 1080;
    double t = (r - l) * h / (2.0 * w);
    double b = -t;
    CalculateRange(50000, 500, l, t, r, b, w, h);
    cout << "Done.  Total loop-check count is " << total_loopchecks << ".\r\n";
    return 0;
}
