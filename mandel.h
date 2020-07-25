#include <new.h>
#include <afxwin.h>
#include <afxcmn.h>
#include <afxmt.h>
#include <math.h>
#include <conio.h>
#include "resource.h"

// #define OUR_COMPLEX


#ifdef OUR_COMPLEX

template <int FRAC> class fix32     // a 32 bit signed fixed-point type with a (32 - FRAC) bit mantissa
{
private:
    long v;
    fix32(long internal) : v(internal)      { }

public:
    fix32()                                 { }
    fix32(double init)                      { v = (long) (init * (double) (1L << FRAC)); }
    operator double () const                { return (double) v / (double) (1L << FRAC); }

    fix32& operator  = (const fix32& o)     { v = o.v; return *this; }
    fix32& operator += (const fix32& o)     { v += o.v; return *this; }
    fix32& operator -= (const fix32& o)     { v -= o.v; return *this; }
    fix32& operator *= (const fix32& o)     { v = Int64ShraMod32(Int32x32To64(v, o.v), FRAC); return *this; }
    fix32 operator + (const fix32& o) const { return fix32(v + o.v); }
    fix32 operator - (const fix32& o) const { return fix32(v - o.v); }
    fix32 operator * (const fix32& o) const { return fix32((long) Int64ShraMod32(Int32x32To64(v, o.v), FRAC)); }
    bool operator == (const fix32& o) const { return v == o.v; }
    bool operator <  (const fix32& o) const { return v < o.v; }
    bool operator >  (const fix32& o) const { return v > o.v; }
    bool operator != (const fix32& o) const { return v != o.v; }
    bool operator <= (const fix32& o) const { return v <= o.v; }
    bool operator >= (const fix32& o) const { return v >= o.v; }
};

class fix64     // a 64 bit signed fixed-point type with a 16 bit mantissa
{
private:
    __int64 v;
    fix64(__int64 internal) : v(internal)   { }

public:
    fix64()                                 { }
    fix64(double init)                      { v = (__int64) (init * (double) ((__int64) 1 << 48)); }
    operator double () const                { return (double) v / (double) ((__int64) 1 << 48); }

    fix64& operator  = (const fix64& o)     { v = o.v; return *this; }
    fix64& operator += (const fix64& o)     { v += o.v; return *this; }
    fix64& operator -= (const fix64& o)     { v -= o.v; return *this; }
    fix64& operator *= (const fix64& o)     { v = (v >> 24) * (o.v >> 24); return *this; }
    fix64 operator + (const fix64& o) const { return fix64(v + o.v); }
    fix64 operator - (const fix64& o) const { return fix64(v - o.v); }
    fix64 operator * (const fix64& o) const { return fix64((v >> 24) * (o.v >> 24)); }
    bool operator == (const fix64& o) const { return v == o.v; }
    bool operator <  (const fix64& o) const { return v < o.v; }
    bool operator >  (const fix64& o) const { return v > o.v; }
    bool operator != (const fix64& o) const { return v != o.v; }
    bool operator <= (const fix64& o) const { return v <= o.v; }
    bool operator >= (const fix64& o) const { return v >= o.v; }
};


// Instantiating with double now produces results that seem identical to those obtained with
// complex.h, though in an older version, it was slightly slower and a bit different looking.
// Instantiating with long double is the same -- Watcom doesn't support larger floating point.
// Instantiating with float speeds it up just a bit, with a great increase in artifacts.
// Instantiating with fix32<28> produces remarkable artifacts outside the set boundary, and a
// look similar to float inside it, and is slower by a factor of two or three.  With fix32<27>
// the artifacts decrease, with fix32<26> they're mostly gone, and with fix32<25> they're gone.
// Instantiating with fix64 is about as slow as fix32, with artifacts somewhat like float but
// more patterned... strangely, these artifacts seem to be unaffected both by changes in the
// precision of float64 and different values of EPSILON, including no value; this is hard to explain.
template <class NUMBER> class complex
{
private:
    NUMBER r, i;

public:
    complex(NUMBER rr, NUMBER ii) : r(rr), i(ii)  { }
    complex(const complex& o) : r(o.r), i(o.i)    { }
    complex()                                     { }
    
    friend NUMBER norm(const complex& o);
    NUMBER real() const                           { return r; }
    NUMBER imag() const                           { return i; }

    complex& operator  = (const complex& o)       { r = o.r; i = o.i; return *this; }
    complex& operator += (const complex& o)       { r += o.r; i += o.i; return *this; }
    complex& operator -= (const complex& o)       { r -= o.r; i -= o.i; return *this; }
    complex& operator *= (const complex& o)       { NUMBER tr = r * o.r - i * o.i;
                                                    i = r * o.i + i * o.r; r = tr; return *this; }
    complex  operator +  (const complex& o) const { return complex(r + o.r, i + o.i); }
    complex  operator -  (const complex& o) const { return complex(r - o.r, i - o.i); }
    complex  operator *  (const complex& o) const { return complex(r * o.r - i * o.i, r * o.i + i * o.r); }
    // this simple class does not support division
    bool     operator == (const complex& o) const { return r == o.r && i == o.i; }
    bool     operator != (const complex& o) const { return r != o.r || i != o.i; }
};

template<class NUMBER> NUMBER norm(const complex<NUMBER>& o)
{
    return o.r * o.r + o.i * o.i;
}

typedef fix32<27> fix27;

typedef complex<fix64> Complex;
  #define FOUR fix64(4.0)
  #define EPSILON 1e-14

#else
  #include <complex.h>
  #define FOUR 4.0
  #define EPSILON 1e-14
#endif


class Mandel
{
protected:
    volatile long*    results;
    volatile Complex* state;

public:
    int lastlimit, lastloopmax, previouslimit, previousloopmax;
    int width, height;
    double left, top, right, bottom;
    bool loop_length_only;
    
    Mandel(int wide, int high);
    ~Mandel()                      { delete[] results; delete[] state; }

    long& ResultAt(int x, int y)   { return (long&) results[x + width * y]; }
    Complex& StateAt(int x, int y) { return (Complex&) state[x + width * y]; }
    void ClearResults()            { memset((void *) results, 0, width * height * sizeof(long)); }
    int Width()                    { return width; }
    int Height()                   { return height; }
    int GridX(double x)            { return (int) (width * (x - left) / (right - left)); }
    int GridY(double y)            { return (int) (height * (y - top) / (bottom - top)); }
    double RealX(int column)       { return left + (column + 0.5) * (right - left) / width; }
    double RealY(int row)          { return top + (row + 0.5) * (bottom - top) / height; }
    void SetBounds(double l, double t, double r, double b)
                                   { left = l; top = t; right = r; bottom = b; }
    void CalculateAt(int column, int row, int limit, Complex* loopcheck, int loopmax);
};

// The value of results[n] is defined as follows:
// 0: point is as yet uncalculated
// negative (bits 0-29): number of iterations before definitely outside of set
// bit 30 set: still calculating this point with no conclusion
// positive (bits 0-19): length of loop detected
// positive (bits 20-29): additional information to be displayed about loop
// Alternately: positive (bits 0-29): number of iterations before loop detected


class MandelPaint;


// This is the struct that is passed in to worker threads, and sent back via
// WM_NOTIFY -- no, make that CM_LINEDONE_T* -- when the thread completes a line.
struct ThreadParams
{
    NMHDR        hdr;                   // hdr.idFrom is the index of the thread
    MandelPaint* mandel;
    int          row;
    bool         rowIsComplete;         // false if user stopped thread in the middle
};

typedef volatile struct ThreadParams* pvThreadParams;


class MandelPaint : public Mandel
{
protected:
    int leftoffset, topoffset;

public:
    MandelPaint(int wide, int high);

    int LeftOffset()            { return leftoffset; }
    int TopOffset()             { return topoffset; }
    void WindowResized()        { SetTopLeftOffset(leftoffset, topoffset); }
    void SetTopLeftOffset(int left, int top);
    CRect VisibleRect(CWnd* owner);
    bool CalculateRange(ThreadParams* threadParam, int limit, int loopmax,
                        double l = HUGE_VAL, double t = HUGE_VAL,
                        double r = HUGE_VAL, double b = HUGE_VAL);
    COLORREF ColorOfResult(long resultval);
};

extern volatile bool stop_calculating;




///////  That takes care of the mathematical part.  Now we do the Windows part.

// here is our window!
class CManFrame : public CFrameWnd
{
    DECLARE_DYNCREATE(CManFrame)
public:
    CManFrame();
    ~CManFrame() { delete Picture; }

    CMenu        contextMenu;
    CBitmap*     pictureBuffer;      // an offscreen copy of the rendered color image
    CMutex       pictureMutex;       // controls access to picturebuffer
    char         basecaption[64];
    MandelPaint* Picture;
    int          permil_done;
    bool         bAlreadySizing;

    void UpdateTitle();
    void EraseBuffer();
    void ClearResults()    { Picture->ClearResults(); EraseBuffer(); Invalidate(); }

protected:
    COLORREF background;        // for area outside the mandel picture
    bool     bDragging;
    bool     bCursorInside;
    CPoint   pDragStart;
    CPoint   pDragLast;
    // XXX handle screen depth/resolution change by reallocating picturebuffer

    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC*)  {return TRUE;}
    afx_msg void OnClose();
    afx_msg void OnMouseMove(UINT flags, CPoint point);
    afx_msg void OnLButtonDown(UINT flags, CPoint point);
    afx_msg void OnLButtonUp(UINT flags, CPoint point);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL OnSetCursor(CWnd* w, UINT h, UINT m);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* ctl);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* ctl);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

    BOOL PreCreateWindow(CREATESTRUCT& cs);
    void XorFrame(CDC& dc, CPoint start, CPoint end);
    CPoint FixAspectRatio(CPoint anchor, CPoint drag);

    DECLARE_MESSAGE_MAP()
};
    

enum MenuState { blank, ready, doing, done };

#define MAX_THREADS 32

class CMandelApp : public CWinApp
{
public:
    MenuState  lastState;
    int        howManyThreads;

    afx_msg void StopCalculation();
    afx_msg void StartCalculation();
    void SetMenuState(MenuState state);
    bool AnyCalculationsStillRunning();
    
private:
    CManFrame*      frame;
    pvThreadParams  threadParam[MAX_THREADS];
    CWinThread*     calculation[MAX_THREADS];
    bool            hasConsole;

    void EnableMenus(CMenu* menu, MenuState state);
    void ProcessWindowMessages();
    BOOL InitInstance();
    int  ExitInstance();
    void OnThreadLineDone(int threadIndex);
    void OnThreadStopped(int threadIndex);
    afx_msg void OnAppAbout();
    afx_msg void OnNew();
    afx_msg void OnSetBounds();
    afx_msg void OnSaveAs();
    afx_msg void OnMoreIterations();
    afx_msg void OnLineComplete(UINT id, NMHDR* pNotifyStruct, LRESULT* result);
    afx_msg void OnThreadFinished(UINT id, NMHDR* pNotifyStruct, LRESULT* result);

    DECLARE_MESSAGE_MAP()
};


// Dialogs:

class CFloatEdit : public CEdit
{
public:
    double Value()  {if (!edited) return lastset; char buf[64]; GetWindowText(buf, 64); return atof(buf);}
    bool Valid();
    void Set(double v);
    double operator =(double v)  {Set(v); return v;}
protected:
    bool edited;
    double lastset;
    bool LargelySelected() { int s, e; GetSel(s, e); return e - s > LineLength() / 2; }
    bool Floaty(UINT ch)   { return (ch >= '0' && ch <= '9') || ch == '-' || ch == '.' || ch < ' '; }
    afx_msg void OnChar(UINT chair, UINT repeat, UINT flags)
                           { if (Floaty(chair)) { CEdit::OnChar(chair, repeat, flags); edited = true; }
                             else ::MessageBeep(-1); }
    afx_msg void OnKeyDown(UINT key, UINT repeat, UINT flags)
                           { if (key == VK_DELETE) edited = true; CEdit::OnKeyDown(key, repeat, flags); }
    afx_msg void OnSetFocus(CWnd* old)  { CEdit::OnSetFocus(old); SetSel(0, -1); };
    DECLARE_MESSAGE_MAP()
};

class CIntEdit : public CEdit
{
public:
    long Value()  { if (!edited) return lastset; char buf[64]; GetWindowText(buf, 64); return atol(buf); }
    bool Valid();
    void Set(long v);
    long operator =(long v)  {Set(v); return v;}
protected:
    bool edited;
    long lastset;
    bool LargelySelected() { int s, e; GetSel(s, e); return e - s > LineLength() / 2; }
    bool Inty(UINT ch)     { return (ch >= '0' && ch <= '9') /*|| ch == '-'*/ || ch < ' '; }
    afx_msg void OnChar(UINT chair, UINT repeat, UINT flags)
                           { if (Inty(chair)) { CEdit::OnChar(chair, repeat, flags); edited = true; }
                             else ::MessageBeep(-1); }
    afx_msg void OnKeyDown(UINT key, UINT repeat, UINT flags)
                           { if (key == VK_DELETE) edited = true; CEdit::OnKeyDown(key, repeat, flags); }
    afx_msg void OnSetFocus(CWnd* old)  {CEdit::OnSetFocus(old); SetSel(0, -1);};
    DECLARE_MESSAGE_MAP()
};

class CBoundsDlg : public CDialog
{
public:
    CBoundsDlg(CWnd* parent, double* left, double* top, double* right, double* bottom,
               int* iterations, int* loopmax, bool* lengthonly, int wide, int high);
protected:
    double *pLeft, *pTop, *pRight, *pBottom;
    int *pIterations, *pLoopMax;
    bool *pLengthOnly;
    double realcenter, imagcenter, width;
    double templeft, temptop, tempright, tempbottom;
    int tempiterations, temploopmax, pixelwidth, pixelheight;
    bool templengthonly;

    CFloatEdit e_realcenter, e_imagcenter, e_width, e_left, e_top, e_right, e_bottom;
    CIntEdit e_iterate, e_loopmax;
    CSliderCtrl z_realcenter, z_imagcenter, z_width;
    CButton b_lengthonly;
    CStatic s_pixelsize;

    afx_msg BOOL OnInitDialog();
    afx_msg void OnOK();
    afx_msg void OnUpdateLeft();
    afx_msg void OnUpdateTop();
    afx_msg void OnUpdateRight();
    afx_msg void OnUpdateBottom();
    afx_msg void OnUpdateRealCenter();
    afx_msg void OnUpdateImagCenter();
    afx_msg void OnUpdateWidth();
    afx_msg void OnUpdateIterations();
    afx_msg void OnUpdateLoopMax();
    void SyncShitUp(CWnd* trust);
    DECLARE_MESSAGE_MAP()
};

class CNewDlg : public CDialog
{
public:
    CNewDlg(CWnd* parent) : CDialog(IDD_NEWDLG, parent) { }
protected:
    DECLARE_MESSAGE_MAP()
};

class CMoreDlg : public CDialog
{
public:
    CMoreDlg(CWnd* parent, int* iterations, int* loopmax);
protected:
    int tempiterations, temploopmax, startiterations, startloopmax;
    int *pIterations, *pLoopMax;
    CIntEdit e_iterate, e_loopmax;

    afx_msg BOOL OnInitDialog();
    afx_msg void OnUpdateIterations();
    afx_msg void OnUpdateLoopMax();
    afx_msg void OnOK();
    DECLARE_MESSAGE_MAP()
};
