//  dialogs.cpp: the dialogs for inputting settings

#include "mandel.h"


//  CONTROL CLASSES:

BEGIN_MESSAGE_MAP(CFloatEdit, CEdit)
    ON_WM_CHAR()
    ON_WM_KEYDOWN()
    ON_WM_SETFOCUS()
END_MESSAGE_MAP()

bool CFloatEdit::Valid()
{
    char buf[64], *p; 
    GetWindowText(buf, 64); 
    double v = strtod(buf, &p);
    return p - buf == strlen(buf) && (v != 0.0 || strchr(buf, '0'));
}

void CFloatEdit::Set(double v)
{
    bool sold = LargelySelected();
    char buf[64];
    sprintf(buf, "%.14g", v);
    int bl = strlen(buf);
    char *dp = strchr(buf, '.');
    if (bl > 10 && dp) {
        char* p = buf + bl - 5;
        int cision = bl - (dp - buf) - 1;
        double frack = pow(10.0, (double) -cision);
        if (!strcmp(p, "99998"))
            v += 2.0 * frack;
        else if (!strcmp(p, "99999"))
            v += frack;
        else if (!strcmp(p, "00001"))
            v -= frack;
        else if (!strcmp(p, "00002"))
            v -= 2.0 * frack;
        sprintf(buf, "%.14g", v);
    }
    SetWindowText(buf);
    if (sold)
        SetSel(0, -1);
    Invalidate();
    UpdateWindow();
    edited = false;
    lastset = v;
}

BEGIN_MESSAGE_MAP(CIntEdit, CEdit)
    ON_WM_CHAR()
    ON_WM_KEYDOWN()
    ON_WM_SETFOCUS()
END_MESSAGE_MAP()

bool CIntEdit::Valid()
{
    char buf[64], *p;
    GetWindowText(buf, 64);
    long v = strtol(buf, &p, 10);
    return p - buf == strlen(buf) && (v != 0 || strchr(buf, '0'));
}

void CIntEdit::Set(long v)
{
    bool sold = LargelySelected();
    char buf[64];
    ltoa(v, buf, 10);
    SetWindowText(buf);
    if (sold)
        SetSel(0, -1);
    Invalidate();
    UpdateWindow();
    edited = false;
    lastset = v;
}


// THE BOUNDS SETTING DIALOG:

BEGIN_MESSAGE_MAP(CBoundsDlg, CDialog)
    ON_COMMAND(IDOK, OnOK)
    ON_EN_KILLFOCUS(IDC_BOUNDS_REALCENTER, OnUpdateRealCenter)
    ON_EN_KILLFOCUS(IDC_BOUNDS_IMAGCENTER, OnUpdateImagCenter)
    ON_EN_KILLFOCUS(IDC_BOUNDS_WIDTH, OnUpdateWidth)
    ON_EN_KILLFOCUS(IDC_BOUNDS_LEFT, OnUpdateLeft)
    ON_EN_KILLFOCUS(IDC_BOUNDS_RIGHT, OnUpdateRight)
    ON_EN_KILLFOCUS(IDC_BOUNDS_BOTTOM, OnUpdateBottom)
    ON_EN_KILLFOCUS(IDC_BOUNDS_TOP, OnUpdateTop)
    ON_EN_KILLFOCUS(IDC_BOUNDS_ITERATIONS, OnUpdateIterations)
    ON_EN_KILLFOCUS(IDC_BOUNDS_LOOPMAX, OnUpdateLoopMax)
END_MESSAGE_MAP()

CBoundsDlg::CBoundsDlg(CWnd* parent, double* left, double* top, double* right, double* bottom,
                       int* iterations, int* loopmax, bool* lengthonly, int wide, int high
                      ) : CDialog(IDD_BOUNDSDLG, parent),
                          pLeft(left), pTop(top), pRight(right), pBottom(bottom),
                          pIterations(iterations), pLoopMax(loopmax),
                          pLengthOnly(lengthonly), pixelwidth(wide), pixelheight(high)
{
    realcenter = (*right + *left) / 2.0;
    imagcenter = (*top + *bottom) / 2.0;
    width = *right - *left;
    templeft = *left;
    temptop = *top;
    tempright = *right;
    tempbottom = *bottom;
    tempiterations = *iterations;
    temploopmax = *loopmax;
    templengthonly = *lengthonly;
}

BOOL CBoundsDlg::OnInitDialog()
{
    char buf[64];
    // with all these floats maybe I shoulda used DDX_Control and DDV_MinMaxDouble
    s_pixelsize. SubclassDlgItem(IDC_BOUNDS_PIXELSIZE, this);
    z_realcenter.SubclassDlgItem(IDC_BOUNDS_REALSLIDER, this);
    z_imagcenter.SubclassDlgItem(IDC_BOUNDS_IMAGSLIDER, this);
    z_width.     SubclassDlgItem(IDC_BOUNDS_WIDTHSLIDER, this);
    e_realcenter.SubclassDlgItem(IDC_BOUNDS_REALCENTER, this);
    e_imagcenter.SubclassDlgItem(IDC_BOUNDS_IMAGCENTER, this);
    e_width.     SubclassDlgItem(IDC_BOUNDS_WIDTH, this);
    e_left.      SubclassDlgItem(IDC_BOUNDS_LEFT, this);
    e_top.       SubclassDlgItem(IDC_BOUNDS_TOP, this);
    e_right.     SubclassDlgItem(IDC_BOUNDS_RIGHT, this);
    e_bottom.    SubclassDlgItem(IDC_BOUNDS_BOTTOM, this);
    e_iterate.   SubclassDlgItem(IDC_BOUNDS_ITERATIONS, this);
    e_loopmax.   SubclassDlgItem(IDC_BOUNDS_LOOPMAX, this);
    b_lengthonly.SubclassDlgItem(IDC_BOUNDS_LENGTHONLY, this);
    wsprintf(buf, "Pixel size: %d × %d", pixelwidth, pixelheight);
    s_pixelsize.SetWindowText(buf);
    e_realcenter = realcenter;
    e_imagcenter = imagcenter;
    e_width      = width;
    e_left       = templeft;
    e_top        = temptop;
    e_right      = tempright;
    e_bottom     = tempbottom;
    e_iterate    = tempiterations;
    e_loopmax    = temploopmax;
    b_lengthonly.SetCheck(templengthonly ? BST_CHECKED : BST_UNCHECKED);
    // XXX  arrgh, figure out how to match sliders with float values.... later!
    z_width.SetFocus();
    return FALSE;
}

void CBoundsDlg::OnOK()
{
    if (e_realcenter.Valid())
    	realcenter = e_realcenter.Value();
    if (e_imagcenter.Valid())
    	imagcenter = e_imagcenter.Value();
    if (e_width.Valid())
    	width = e_width.Value();
    if (e_left.Valid())
        templeft = e_left.Value();  // hitting return in an edit doesn't trigger killfocus before OnOK
    if (e_right.Valid())
        tempright = e_right.Value();
    if (e_top.Valid())
        temptop = e_top.Value();
    if (e_bottom.Valid())
        tempbottom = e_bottom.Value();
    if (e_iterate.Valid())
        tempiterations = e_iterate.Value();
    if (e_loopmax.Valid())
        temploopmax = e_loopmax.Value();
    templengthonly = b_lengthonly.GetCheck() == BST_CHECKED;
    if (templeft >= tempright || width <= 0) {
        MessageBox("Width is zero or negative", "Bounds", MB_ICONEXCLAMATION | MB_OK);
        e_width.SetFocus();
    } else if (tempbottom >= temptop) {
        MessageBox("Height is zero or negative", "Bounds", MB_ICONEXCLAMATION | MB_OK);
        e_bottom.SetFocus();
    } else if (!e_left.Valid()) {
        MessageBox("Left edge is not a readable number", "Bounds", MB_ICONEXCLAMATION | MB_OK);
        e_left.SetFocus();
    } else if (!e_top.Valid()) {
        MessageBox("Top edge is not a readable number", "Bounds", MB_ICONEXCLAMATION | MB_OK);
        e_top.SetFocus();
    } else if (!e_right.Valid()) {
        MessageBox("Right edge is not a readable number", "Bounds", MB_ICONEXCLAMATION | MB_OK);
        e_right.SetFocus();
    } else if (!e_bottom.Valid()) {
        MessageBox("Bottom edge is not a readable number", "Bounds", MB_ICONEXCLAMATION | MB_OK);
        e_bottom.SetFocus();
    } else if (!e_realcenter.Valid()) {
        MessageBox("Real centerpoint is not a readable number", "Bounds", MB_ICONEXCLAMATION | MB_OK);
        e_realcenter.SetFocus();
    } else if (!e_imagcenter.Valid()) {
        MessageBox("Imaginary centerpoint is not a readable number", "Bounds", MB_ICONEXCLAMATION | MB_OK);
        e_imagcenter.SetFocus();
    } else if (!e_width.Valid()) {
        MessageBox("Width is not a readable number", "Bounds", MB_ICONEXCLAMATION | MB_OK);
        e_width.SetFocus();
    } else if (!e_iterate.Valid()) {
        MessageBox("Iteration limit is not a readable integer", "Bounds", MB_ICONEXCLAMATION | MB_OK);
        e_iterate.SetFocus();
    } else if (!e_loopmax.Valid()) {
        MessageBox("Loop checking limit is not a readable integer", "Bounds", MB_ICONEXCLAMATION | MB_OK);
        e_loopmax.SetFocus();
    } else if (e_loopmax.Value() > 1000000) {
        MessageBox("Loop checking limit over 1000000 is not supported", "Bounds", MB_ICONEXCLAMATION | MB_OK);
        e_loopmax.SetFocus();
    } else {
        //char buf[256];
        CString buf;
        if (tempright - templeft > ((double) pixelwidth / (double) pixelheight) * (temptop - tempbottom)) {
            double newhi = (tempright - templeft) * (double) pixelheight / (double) pixelwidth;
            if (newhi / (temptop - tempbottom) > 1.05) {
                buf.Format("Increasing height to %.6g\r\nto preserve aspect ratio.", newhi);
                MessageBox(buf, "MandelGuts", MB_OK | MB_ICONINFORMATION);
            }
            tempbottom = imagcenter - newhi / 2.0;
            temptop = imagcenter + newhi / 2.0;
        } else {
            double newid = (temptop - tempbottom) * (double) pixelwidth / (double) pixelheight;
            if (newid / (tempright - templeft) > 1.05) {
                buf.Format("Increasing width to %.6g\r\nto preserve aspect ratio.", newid);
                MessageBox(buf, "MandelGuts", MB_OK | MB_ICONINFORMATION);
            }
            templeft = realcenter - newid / 2.0;
            tempright = realcenter + newid / 2.0;
        }
        *pLeft = templeft;
        *pTop = temptop;
        *pRight = tempright;
        *pBottom = tempbottom;
        *pIterations = max(tempiterations, 8);
        *pLoopMax = max(temploopmax, 0);
        *pLengthOnly = templengthonly;
        EndDialog(IDOK);
    }
}

void CBoundsDlg::SyncShitUp(CWnd *trust)
{
    // XXX  the idea here is to check if some settings are invalid, and if so,
    // figure out which ones are more trustworthy and use them to put valid
    // settings into the less trustworthy ones.
}

void CBoundsDlg::OnUpdateLeft()
{
    if (!e_left.Valid()) return;
    double old = width;
    /*e_left =*/ templeft = e_left.Value();
    e_width = width = tempright - templeft;
    e_realcenter = realcenter = (tempright + templeft) / 2.0;
//  if (tempright <= templeft) return;
//  SyncShitUp(&e_left);
//  e_bottom = tempbottom = imagcenter + (tempbottom - imagcenter) * width / old;
//  e_top = temptop = imagcenter + (temptop - imagcenter) * width / old;
}

void CBoundsDlg::OnUpdateTop()
{
    if (!e_top.Valid()) return;
    double old = width;
    /*e_top =*/ temptop = e_top.Value();
    e_imagcenter = imagcenter = (temptop + tempbottom) / 2.0;
//  if (temptop <= tempbottom) return;
//  SyncShitUp(&e_top);
//  e_width = width = (temptop - tempbottom) * pixelwidth / (double) pixelheight;
//  e_left = templeft = realcenter + (templeft - realcenter) * width / old;
//  e_right = tempright = realcenter + (tempright - realcenter) * width / old;
}

void CBoundsDlg::OnUpdateRight()
{
    if (!e_right.Valid()) return;
    double old = width;
    /*e_right =*/ tempright = e_right.Value();
    e_width = width = tempright - templeft;
    e_realcenter = realcenter = (tempright + templeft) / 2.0;
//  if (tempright <= templeft) return;
//  SyncShitUp(&e_right);
//  e_width = width = tempright - templeft;
//  e_bottom = tempbottom = imagcenter + (tempbottom - imagcenter) * width / old;
//  e_top = temptop = imagcenter + (temptop - imagcenter) * width / old;
}

void CBoundsDlg::OnUpdateBottom()
{
    if (!e_bottom.Valid()) return;
    double old = width;
    /*e_bottom =*/ tempbottom = e_bottom.Value();
    e_imagcenter = imagcenter = (temptop + tempbottom) / 2.0;
//  if (temptop <= tempbottom) return;
//  SyncShitUp(&e_bottom);
//  e_width = width = (temptop - tempbottom) * pixelwidth / (double) pixelheight;
//  e_left = templeft = realcenter + (templeft - realcenter) * width / old;
//  e_right = tempright = realcenter + (tempright - realcenter) * width / old;
}

void CBoundsDlg::OnUpdateRealCenter()
{
    if (!e_realcenter.Valid()) return;
    double old = realcenter;
    /*e_realcenter =*/ realcenter = e_realcenter.Value();
//  SyncShitUp(&e_realcenter);
    templeft += realcenter - old;
    tempright += realcenter - old;
    e_left = templeft;
    e_right = tempright;
}

void CBoundsDlg::OnUpdateImagCenter()
{
    if (!e_imagcenter.Valid()) return;
    double old = imagcenter;
    /*e_imagcenter =*/ imagcenter = e_imagcenter.Value();
//  SyncShitUp(&e_imagcenter);
    tempbottom += imagcenter - old;
    temptop += imagcenter - old;
    e_bottom = tempbottom;
    e_top = temptop;
}

void CBoundsDlg::OnUpdateWidth()
{
    if (!e_width.Valid()) return;
    /*e_width =*/ width = e_width.Value();
    if (width <= 0) return;
    SyncShitUp(&e_width);
    double old = tempright - templeft;
    e_left = templeft = realcenter + (templeft - realcenter) * width / old;
    e_right = tempright = realcenter + (tempright - realcenter) * width / old;
    e_bottom = tempbottom = imagcenter + (tempbottom - imagcenter) * width / old;
    e_top = temptop = imagcenter + (temptop - imagcenter) * width / old;
}

void CBoundsDlg::OnUpdateIterations()
{
    if (!e_iterate.Valid()) return;
    /*e_iterate =*/ tempiterations = e_iterate.Value();
    if (tempiterations < 8)
        e_iterate = tempiterations = 8;
}

void CBoundsDlg::OnUpdateLoopMax()
{
    if (!e_loopmax.Valid()) return;
    /*e_loopmax =*/ temploopmax = e_loopmax.Value();
    if (temploopmax < 0)
        e_loopmax = temploopmax = 0;
}


// THE NEW PICTURE DIALOG:


// THE INCREASE ITERATIONS DIALOG:

BEGIN_MESSAGE_MAP(CMoreDlg, CDialog)
    ON_COMMAND(IDOK, OnOK)
    ON_EN_KILLFOCUS(IDC_MORE_ITERATIONS, OnUpdateIterations)
    ON_EN_KILLFOCUS(IDC_MORE_LOOPMAX, OnUpdateLoopMax)
END_MESSAGE_MAP()

CMoreDlg::CMoreDlg(CWnd* parent, int* iterations, int* loopmax) : CDialog(IDD_MOREITERATIONSDLG, parent)
{
    pIterations = iterations;
    pLoopMax = loopmax;
    startiterations = tempiterations = *iterations;
    startloopmax = temploopmax = *loopmax;
}

BOOL CMoreDlg::OnInitDialog()
{
    e_iterate.SubclassDlgItem(IDC_MORE_ITERATIONS, this);
    e_loopmax.SubclassDlgItem(IDC_MORE_LOOPMAX, this);
    e_iterate = tempiterations;
    e_loopmax = temploopmax;
    e_iterate.SetFocus();
    return FALSE;
}

void CMoreDlg::OnOK()
{
    if (!e_iterate.Valid()) {
        MessageBox("Iteration limit is not a readable integer", "More Iterations", MB_ICONEXCLAMATION | MB_OK);
        e_iterate.SetFocus();
        return;
    } else
        tempiterations = e_iterate.Value();
    if (!e_loopmax.Valid()) {
        MessageBox("Loop checking limit is not a readable integer", "More Iterations", MB_ICONEXCLAMATION | MB_OK);
        e_loopmax.SetFocus();
        return;
    } else if (e_loopmax.Value() > 1000000) {
        MessageBox("Loop checking limit over 1000000 is not supported", "More Iterations", MB_ICONEXCLAMATION | MB_OK);
        e_loopmax.SetFocus();
        return;
    } else
        temploopmax = e_loopmax.Value();
    *pIterations = max(tempiterations, startiterations);
    *pLoopMax = max(temploopmax, startloopmax);
    EndDialog(IDOK);
}

void CMoreDlg::OnUpdateIterations()
{
    if (!e_iterate.Valid()) return;
    /*e_iterate =*/ tempiterations = e_iterate.Value();
    if (tempiterations < startiterations)
        e_iterate = tempiterations = startiterations;
}

void CMoreDlg::OnUpdateLoopMax()
{
    if (!e_loopmax.Valid()) return;
    /*e_loopmax =*/ temploopmax = e_loopmax.Value();
    if (temploopmax < startloopmax)
        e_loopmax = temploopmax = startloopmax;
}


