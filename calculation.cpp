//  calculation.cpp: calculation threads and image rendering

#include "mandel.h"
    
//#define CHECK_WITH_EQUALITY  1

// The functions defined in point.cpp are inline, so we #include them instead of linking:
#include "point.cpp"


extern CMandelApp    mister_Big;


// class MandelPaint is responsible for relating the mathematical plane
// to a grid of colored pixels.

MandelPaint::MandelPaint(/*CWnd* parent,*/ int wide, int high) : Mandel(wide, high)
{
    SetTopLeftOffset(0, 0);
}

void MandelPaint::SetTopLeftOffset(int left, int top)
{
    leftoffset = left;
    topoffset = top;
}

CRect MandelPaint::VisibleRect(CWnd* owner)
{
    CRect rwin;
    owner->GetClientRect(&rwin);
    rwin &= CRect(leftoffset, topoffset, leftoffset + width, topoffset + height);
    return rwin;
}

// called by worker thread
// Formerly this created a DC and called Display1Point directly as each point was
// completed.  This is now deemed not thread-safe.
bool MandelPaint::CalculateRange(ThreadParams* threadParam, int limit, int loopmax,
                                 double l, double t, double r, double b)
{
    int x, y, pc = 0;
    Complex* loopcheck;
    HWND owner = threadParam->hdr.hwndFrom;
    cprintf("   > worker thread %d calculating a range!\r\n", threadParam->hdr.idFrom);
    if (r != HUGE_VAL && b != HUGE_VAL)
        SetBounds(l, t, r, b);
    lastlimit = limit;
    lastloopmax = loopmax;
    stop_calculating = false;
    loopcheck = new Complex[loopmax];
    y = threadParam->row;
    while (y < height && !stop_calculating)
    {
        cprintf("   > commencing to calculate row %d in thread %d\r\n", y, threadParam->hdr.idFrom);
        for (x = 0; x < width && !stop_calculating; x++)
        {
            long r = ResultAt(x, y);
            if (!r || (r > 0 && r & 0x40000000 && (r & ~0x40000000) < limit))
            {
                CalculateAt(x, y, limit, loopcheck, loopmax);
                // without something like this, the foreground process gets stuck under 95/98/ME:
                if (x % 16 == 0)
                    ::Sleep(0);     // end this time slice, run other threads including painter
            }
        }
        threadParam->hdr.code = NM_LINECOMPLETE;
        threadParam->rowIsComplete = !stop_calculating;
        threadParam->row = y;
//        cprintf("   > row done, sending CM_LINEDONE_T0 -- row is %d, rowIsComplete %d,\r\n"
//                "     threadIndex %d, hdr.code is %d, hdr.hwndFrom is %X, mandel is %X\r\n",
//                threadParam->row, threadParam->rowIsComplete, threadParam->hdr.idFrom,
//                threadParam->hdr.code, threadParam->hdr.hwndFrom, threadParam->mandel);
//        cprintf("   > completion of row %d; midpoint is state %X color %X\r\n",
//                y, ResultAt(width / 2, y), ColorOfResult(ResultAt(width / 2, y)));
        ::SendMessage(owner, WM_NOTIFY, NM_LINECOMPLETE, (LPARAM) threadParam);
        y = threadParam->row;
        cprintf("   > resuming after return of SendMessage -- next row for thread %d is %d\r\n", threadParam->hdr.idFrom, y);
        Sleep(0);
    }
    delete[] loopcheck;
    return true;
}

int RawColors[8][3] = {
    {255, 0, 0},        // red for point attractors
    {0, 255, 0},        // green for cycle 2
    {255, 0, 255},      // magenta for cycle 3
    {0, 255, 255},      // cyan for cycle 4
    {255, 255, 0},      // yellow for cycle 5
    {0, 0, 255},        // blue for cycle 6
    {255, 128, 0},      // orange for cycle 7
    {128, 255, 128}     // light blue for cycle 8
};

// This is where we translate results into colors.
COLORREF MandelPaint::ColorOfResult(long result)
{
    if (!result)                        // point not calculated yet
        return RGB(160, 160, 160);
    if (result < 0) {                   // bands outside the border
        int r = abs(result % 50);
        int g = result >= -50 ? 120 + 2 * result : 20 + 3 * min(r, 50 - r);
        int g2 = g * 4 / 3;
        switch (abs(result / 8 + (result / 4) % 3) % 16) {
            default: return RGB(g, g, g);
            case  5: return RGB(g2, g2, g);
            case  7: return RGB(g2, g, g2); 
            case  9: return RGB(g, g2, g2);
            case 11: return RGB(g2, g, g);
            case 13: return RGB(g, g2, g); 
            case 15: return RGB(g, g, g2);
        }
    }
    if (result & 0x40000000)            // point is indeterminate, needs more iterations
        return RGB(255, 255, 255);

    // inside the set: this is where traditional Mandels just show black!  I want to see more.
    const int raws = sizeof(RawColors) / sizeof(RawColors[0]);
    int red, green, blue;
    result--;
    if (loop_length_only)
    {
        int ix = result % raws;
        red = RawColors[ix][0];
        green = RawColors[ix][1];
        blue = RawColors[ix][2];
        for (int j = 0; j < result / raws && j < lastloopmax % 32; j += raws)
        {
            red = (red * 256) / 320 + 3;
            green = (green * 256) / 320 + 3;
            blue = (blue * 256) / 320 + 3;
        }
        if (!((result / raws) & 1))
        {
            red = red * 3 / 4;
            green = green * 3 / 4;
            blue = blue * 3 / 4;
        }
    }
    else
    {
        int ix = (result & 0x000FFFFF) % raws;
        red = RawColors[ix][0];
        green = RawColors[ix][1];
        blue = RawColors[ix][2];
        // we rotate the hue according to the magnitude of the loop length...
        int neighborpart = (int) (128.0 * (1.0 + sin((double) (result & 0x000FFFFF) / 100.0)));
        int tempred = (red * (256 - neighborpart) + green * neighborpart) / 256;
        green = (green * (256 - neighborpart) + blue * neighborpart) / 256;
        blue = (blue * (256 - neighborpart) + red * neighborpart) / 256;
        red = tempred;
        // and set the luminosity according to the loop "diameter"... let's try it with
        // a triangle-wave transform to exaggerate the near differences:
        int lf = (result >> 20) & 0x000003FF;
        lf = 1023 - ((lf + 64) & 0x000003FF);           // reverse ends, add offset so cardioid not black
        lf = (lf & 0x0000007F) << 3 | lf >> 7;          // move top three bits to bottom
        if (lf & 1) lf = 1023 - lf;                     // convert sawtooth wave to triangle wave
        red = red * (lf + 128) / 1151;
        green = green * (lf + 128) / 1151;
        blue = blue * (lf + 128) / 1151;
    }
    return RGB(red, green, blue);
}


// class Mandel, the base of MandelPaint, is responsible for representing the
// complex plane as a grid of sample points, and performing the actual calculation
// of testing a point's relation to the Mandelbrot set.

Mandel::Mandel(int wide, int high) : width(wide), height(high)
{
    results = new long[wide * high];
    state = new Complex[wide * high];
    left = -2.75;       // a good overall view of the set with a widescreen aspect ratio
    right = 1.75;
    top = (right - left) * height / (2.0 * width);
    bottom = -top;
    lastlimit = 5000;
    lastloopmax = 250;
    loop_length_only = false;
    ClearResults();
}


/*
complex<double> Mandel::CalculateAt(int column, int row, int limit, int loopmax)
{
    long i;
    complex<double> ret;
    if (usedouble) {
        complex<double> p(RealX(column), RealY(row));
        complex<double>* loopcheck = new complex<double>[loopmax];
        i = DoMandelPoint(p, limit, loopmax, loopcheck);           // more accurate
        delete[] loopcheck;
        ret = p;
    } else {
        complex<fix32> q(fix32(RealX(column)), fix32(RealY(row)));
        complex<fix32>* loopchex = new complex<fix32>[loopmax];
        i = DoMandelPoint(q, limit, loopmax, loopchex);            // faster, hopefully
        delete[] loopchex;
        ret = complex<double>((double) q.r, (double) q.i);
    }
    results[column + width * row] = i;
    return ret;
}
*/

// called by worker thread, from CalculateRange
void Mandel::CalculateAt(int column, int row, int limit, Complex* loopcheck, int loopmax)
{
    long& our_result = ResultAt(column, row);
    int oldlimit = our_result & ~0x40000000;
    Complex p;
    Complex p0(RealX(column), RealY(row));
    if (oldlimit > 0) {
        limit -= oldlimit;
        if (limit <= 0)
            return;
        p = StateAt(column, row);
    } else 
        p = p0;
    our_result = DoMandelPoint(p0, p, limit, loopmax, loop_length_only, loopcheck);
    StateAt(column, row) = p;
    if (our_result < 0)
        our_result -= oldlimit;
//    else                          // do this if we return iterations *until* loop...
//        our_result += oldlimit;
}


