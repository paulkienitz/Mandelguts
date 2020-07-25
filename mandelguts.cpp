//  mandel.cpp: main window and input processing

#include <afxdlgs.h>
#include "mandel.h"

long DoMandelPoint(Complex addpt, Complex& point, int limit, int loopmax, Complex* loopcheck);

bool WriteBmpFile(CFrameWnd* frame, CBitmap* cbm, const char* tempath, const char* path);

// THE USER INTERFACE PART:

CMandelApp    mister_Big;

volatile bool stop_calculating = true;

volatile int  nextRow;


static inline double UnTenBitMagnitude(int m)      // inverse of TenBitMagnitude in points.cpp
{
    if (m <= 0)
        return 0.0;
    return pow(10.0, log2((double) m) * 3.0 - 28.0);
}


// CMandelApp is responsible for responding to menu commands.

BEGIN_MESSAGE_MAP(CMandelApp, CWinApp)
    ON_COMMAND(ID_APP_ABOUT,        OnAppAbout)
    ON_COMMAND(CM_MANDEL_NEW,       OnNew)
    ON_COMMAND(CM_MANDEL_SETBOUNDS, OnSetBounds)
    ON_COMMAND(CM_MANDEL_GO,        StartCalculation)
    ON_COMMAND(CM_MANDEL_STOP,      StopCalculation)
    ON_COMMAND(CM_MANDEL_MORE,      OnMoreIterations)
    ON_COMMAND(ID_FILE_SAVE_AS,     OnSaveAs)
    ON_NOTIFY_RANGE(NM_LINECOMPLETE,   0, MAX_THREADS - 1, OnLineComplete)
    ON_NOTIFY_RANGE(NM_THREADCOMPLETE, 0, MAX_THREADS - 1, OnThreadFinished)
END_MESSAGE_MAP()

BOOL CMandelApp::InitInstance()
{
    char eb[32];
    GetEnvironmentVariable("NUMBER_OF_PROCESSORS", eb, 32);
    if (sscanf(eb, " %d ", &howManyThreads) <= 0)
        howManyThreads = 1;
    else if (howManyThreads > MAX_THREADS)
        howManyThreads = MAX_THREADS;
    hasConsole = /*!!AllocConsole()*/ false;
//  calculation = new CWinThread*[howManyThreads]();     // WTF, isn't this supposed to fill it with nulls??
//  threadParam = new pvThreadParams[howManyThreads]();

//  _set_matherr(MathThrower);
//  SetRegistryKey("PK's MandelGuts");
//  LoadStdProfileSettings();

    frame = new CManFrame;
    if (!frame->LoadFrame(IDR_MAINDEL)) {
        ::MessageBox(NULL, "Could not create main window",
                     NAME_AND_VERSION, MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }
    m_pMainWnd = frame;

//  EnableShellOpen();
//  RegisterShellFileTypes(TRUE);
//  CCommandLineInfo cmdInfo;
//  ParseCommandLine(cmdInfo);
//  if (!ProcessShellCommand(cmdInfo))
//      return FALSE;

    frame->ShowWindow(/*m_nCmdShow*/ SW_SHOWMAXIMIZED);
    frame->contextMenu.LoadMenu(IDR_POPUP);
    SetMenuState(blank);
    OnNew();                    // let's have this be automatic
    frame->UpdateWindow();
    frame->GetWindowText(frame->basecaption, sizeof(frame->basecaption));
    return TRUE;
}

int CMandelApp::ExitInstance()
{
    if (hasConsole)
        FreeConsole();
    return CWinApp::ExitInstance();
}


static inline int MenuFlags(bool enable)
{
    return enable ? MF_BYCOMMAND | MF_ENABLED : MF_BYCOMMAND | MF_GRAYED;
}

void CMandelApp::EnableMenus(CMenu* menu, MenuState state)
{
    if (!menu)
        return;
    menu = menu->GetSubMenu(0);
    menu->EnableMenuItem(CM_MANDEL_GO, MenuFlags(state == ready || state == done));
    menu->EnableMenuItem(CM_MANDEL_SETBOUNDS, MenuFlags(state == ready || state == done));
    menu->EnableMenuItem(CM_MANDEL_MORE, MenuFlags(state == done));
    menu->EnableMenuItem(ID_FILE_SAVE_AS, MenuFlags(state == done));
    if (state == doing)
        menu->ModifyMenu(CM_MANDEL_GO, MF_BYCOMMAND | MF_STRING, CM_MANDEL_STOP,
                         "Stop &Calculating\tS");
    else if (lastState == doing)
        menu->ModifyMenu(CM_MANDEL_STOP, MF_BYCOMMAND | MF_STRING, CM_MANDEL_GO,
                         "Start &Calculating\tG");
}

void CMandelApp::SetMenuState(MenuState state)
{
    //EnableMenus(frame->GetMenu(), state);
    EnableMenus(&frame->contextMenu, state);
    lastState = state;
}


void CMandelApp::OnAppAbout()
{
    CDialog dlg(IDD_ABOUTDLG, frame);
    dlg.DoModal();
}

void CMandelApp::OnNew()
{
    try {
        CWaitCursor curse;
        StopCalculation();
        // XXX  PRESERVE BOUNDS WINDOW SETTINGS AS MUCH AS POSSIBLE (adjusting aspect ratio)
        delete frame->Picture;
        CRect rwin;
        frame->bAlreadySizing = true;
        frame->ShowScrollBar(SB_HORZ, false);
        frame->ShowScrollBar(SB_VERT, false);
        frame->GetClientRect(&rwin);
        frame->Picture = new MandelPaint(rwin.Width(), rwin.Height());
        frame->Picture->SetTopLeftOffset((rwin.Width() - frame->Picture->Width()) / 2,
                                         (rwin.Height() - frame->Picture->Height()) / 2);
        if (frame->pictureBuffer)
            delete frame->pictureBuffer;
        frame->pictureBuffer = new CBitmap();
        frame->pictureBuffer->CreateCompatibleBitmap(&CClientDC(frame), frame->Picture->Width(), frame->Picture->Height());
        frame->ClearResults();
        SetMenuState(ready);
        frame->Invalidate();
        frame->bAlreadySizing = false;
    } catch (CMemoryException&) {
        frame->MessageBox("Could not create picture of that size",
                          NAME_AND_VERSION, MB_OK | MB_ICONEXCLAMATION);
        frame->Picture = NULL;
        SetMenuState(blank);
    }
}

void CMandelApp::OnSetBounds()
{
    if (frame->Picture) {
        StopCalculation();
        CBoundsDlg dog(frame, &frame->Picture->left, &frame->Picture->top,
                       &frame->Picture->right, &frame->Picture->bottom,
                       &frame->Picture->lastlimit, &frame->Picture->lastloopmax,
                       &frame->Picture->loop_length_only,
                       frame->Picture->Width(), frame->Picture->Height());
        if (dog.DoModal() == IDOK) {
            SetMenuState(ready);
            frame->ClearResults();
            StartCalculation();
        }
    }
}

void CMandelApp::OnMoreIterations()
{
    if (frame->Picture)
    {
        StopCalculation();
        CMoreDlg dog(frame, &frame->Picture->lastlimit, &frame->Picture->lastloopmax);
        if (dog.DoModal() == IDOK) {
            frame->Invalidate();
            // if (start right away option)
            StartCalculation();
        }
    }
}

void CMandelApp::OnSaveAs()
{
    if (frame->Picture)
    {
        StopCalculation();
        CFileDialog picker(false, "bmp", NULL, OFN_EXPLORER | OFN_OVERWRITEPROMPT,
                           "Bitmap Files (*.bmp;*.dib)|*.bmp;*.dib|All Files|*.*||");
        if (picker.DoModal() == IDOK) {
            CString savepath = picker.GetPathName();
            if (savepath && savepath.GetLength() > 0)
                WriteBmpFile(frame, frame->pictureBuffer, NULL, savepath);
        }
    }
}

// This here is the background thread entry point.
static UINT DoCalculateRange(LPVOID threadParamHdr)
{
    ThreadParams* tp = (ThreadParams*) threadParamHdr;
    cprintf("ENTERING DoCalculateRange for thread %d\r\n", tp->hdr.idFrom);
    MandelPaint* pic = tp->mandel;
    pic->CalculateRange(tp, pic->lastlimit, pic->lastloopmax);
    cprintf("EXITING DoCalculateRange for thread %d\r\n", tp->hdr.idFrom);
    tp->hdr.code = NM_THREADCOMPLETE;
    ::PostMessage(tp->hdr.hwndFrom, WM_NOTIFY, NM_THREADCOMPLETE, (LPARAM) tp);
    return 0;
}

void CMandelApp::ProcessWindowMessages()
{
    MSG msg;
    Sleep(0); 
    while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
        if (!PumpMessage()) {       // will call OnThreadStopped pretty soon
            ::PostQuitMessage(0);
            return;
        }
}

// used both internally and directly by the Go command
void CMandelApp::StartCalculation()
{
    if (frame->Picture && lastState != doing) {     // don't try to start when already started
        cprintf("StartCalculation has been summoned.\r\n");
        nextRow = 0;
        frame->permil_done = 0;
        SetMenuState(doing);
        for (int threadIndex = 0; threadIndex < howManyThreads; threadIndex++)
        {
            threadParam[threadIndex] = new ThreadParams;
            threadParam[threadIndex]->hdr.hwndFrom = frame->m_hWnd;
            threadParam[threadIndex]->hdr.idFrom = threadIndex;
            threadParam[threadIndex]->mandel = frame->Picture;
            threadParam[threadIndex]->row = nextRow++;
            cprintf("Launching thread %d.\r\n", threadIndex);
            calculation[threadIndex] = AfxBeginThread(&DoCalculateRange, (LPVOID) threadParam[threadIndex], THREAD_PRIORITY_BELOW_NORMAL);
        }
        frame->UpdateTitle();
    }
}

// used directly by the Stop command
void CMandelApp::StopCalculation()
{
    cprintf("StopCalculation has been invoked.\r\n");
    stop_calculating = true;
    while (AnyCalculationsStillRunning()) {
        LONG lIdle = 0;
        ProcessWindowMessages();
        while (mister_Big.OnIdle(lIdle++))
           ;
    }
}

bool CMandelApp::AnyCalculationsStillRunning()
{
    for (int c = 0; c < howManyThreads; c++)
        if (calculation[c] != NULL)
            return true;
    return false;
}

void CMandelApp::OnThreadFinished(UINT id, NMHDR* pNotifyStruct, LRESULT* result)
{
    cprintf("@@ Finishment notification received for thread %d.\r\n", ((ThreadParams*) pNotifyStruct)->hdr.idFrom);
    calculation[((ThreadParams*) pNotifyStruct)->hdr.idFrom] = NULL;
    if (!AnyCalculationsStillRunning())
    {
        cprintf("ALL %d THREADS STOPPED.", howManyThreads);
        if (frame->Picture && frame->Picture->ResultAt(frame->Picture->Width() - 1, frame->Picture->Height() - 1)) {
            cprintf("All %d lines have been completed, so let's beep.", frame->Picture->Height());
            MessageBeep(-1);
            Sleep(1);
        }
        frame->permil_done = -1;
        frame->UpdateTitle();
        SetMenuState(done);
    }
}


// This is called by a synchronous message from the worker thread, so in effect
// it sort of runs in that thread's context.
void CMandelApp::OnLineComplete(UINT id, NMHDR* pNotifyStruct, LRESULT* result)
{
    int row = ((ThreadParams*) pNotifyStruct)->row;
    cprintf(" ## Line completed by thread %d: number %d\r\n", ((ThreadParams*) pNotifyStruct)->hdr.idFrom, row);
    MandelPaint* pic = ((ThreadParams*) pNotifyStruct)->mandel;
    // we paint the newly rendered row not to the screen, but to the buffer bitmap
    frame->pictureMutex.Lock();
    {
        CDC bufferdc;
        bufferdc.CreateCompatibleDC(&CClientDC(frame));
        bufferdc.SelectObject(frame->pictureBuffer);
        for (int x = 0; x < pic->width; x++)
            bufferdc.SetPixelV(x, row, pic->ColorOfResult(pic->ResultAt(x, row)));
    }
    frame->pictureMutex.Unlock();
    // send message to paint the row by blitting from the buffer
    frame->InvalidateRect(CRect(pic->LeftOffset(), row + pic->TopOffset(),
                                pic->LeftOffset() + pic->width, row + pic->TopOffset() + 1), false);
    cprintf(" ## Finished line %d invalidated, awaiting paint (pumping messages)\r\n", row);
    ((ThreadParams*) pNotifyStruct)->row = nextRow++;
    int pmd = 1000 * (row + 1) / pic->height;
    if (pmd > frame->permil_done)
    {
        frame->permil_done = pmd;
        frame->PostMessage(WM_COMMAND, CM_UPDATETITLE, NULL);
    }
    ProcessWindowMessages();
    *result = 0;
}



// CFrameWnd is responsible for display rendering, scrolling, and mouse handling.

IMPLEMENT_DYNCREATE(CManFrame, CFrameWnd);

BEGIN_MESSAGE_MAP(CManFrame, CFrameWnd)
    ON_WM_PAINT()
    ON_WM_CLOSE()
    ON_WM_ERASEBKGND()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_DESTROY()
    ON_WM_SIZE()
    ON_WM_SETCURSOR()
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_WM_CONTEXTMENU()
    ON_COMMAND(CM_UPDATETITLE, UpdateTitle)
END_MESSAGE_MAP()

CManFrame::CManFrame() : CFrameWnd()
{
    Picture = NULL;
    background = RGB(192, 192, 192);
    bDragging = bCursorInside = bAlreadySizing = false;
}

BOOL CManFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    // LoadFrame fails if there's no menu resource, so we load a dummy menu and
    // then eliminate it here.
    if (cs.hMenu) {
        ::DestroyMenu(cs.hMenu);
        cs.hMenu = NULL;
    }
    return CFrameWnd::PreCreateWindow(cs);
}

void CManFrame::OnLButtonDown(UINT flags, CPoint point)
{
    SetFocus();
    if (!Picture || !Picture->VisibleRect(this).PtInRect(point))
        return;
    SetCapture();
    bDragging = true;
    pDragStart = pDragLast = point;
}

void CManFrame::OnLButtonUp(UINT flags, CPoint point)
{
    if (bDragging) {
        ReleaseCapture();
        bDragging = false;
        if (pDragStart != pDragLast) {
            CClientDC dc(this);
            XorFrame(dc, pDragStart, pDragLast);
        }
        pDragLast = FixAspectRatio(pDragStart, point);
        CRect rDrag(pDragStart, pDragLast);
        rDrag.NormalizeRect();
        if (rDrag.IsRectEmpty())
            return;
        mister_Big.StopCalculation();
        double tleft = Picture->RealX(rDrag.left - Picture->LeftOffset());
        double ttop = Picture->RealY(rDrag.top - Picture->TopOffset());
        double tright = Picture->RealX(rDrag.right - Picture->LeftOffset());
        double tbottom = Picture->RealY(rDrag.bottom - Picture->TopOffset());
        CBoundsDlg dog(this, &tleft, &ttop, &tright, &tbottom,
                       &Picture->lastlimit, &Picture->lastloopmax,
                       &Picture->loop_length_only,
                       Picture->Width(), Picture->Height());
        if (dog.DoModal() == IDOK) {
            Picture->SetBounds(tleft, ttop, tright, tbottom);
            ClearResults();
            // if (start right away option)
            mister_Big.StartCalculation();
        }
    }
}

void CManFrame::OnMouseMove(UINT flags, CPoint point)
{
    if (bDragging) {
        CClientDC dc(this);
        if (pDragStart != pDragLast)
            XorFrame(dc, pDragStart, pDragLast);
        pDragLast = FixAspectRatio(pDragStart, point);
        if (pDragStart != pDragLast)
            XorFrame(dc, pDragStart, pDragLast);
    }
    if (Picture && Picture->VisibleRect(this).PtInRect(point)) {
        ::SetCursor(::LoadCursor(NULL, IDC_CROSS));
        bCursorInside = true;
        UpdateTitle();
    } else {
        CRect cr;
        GetClientRect(&cr);
        if (cr.PtInRect(point))
            ::SetCursor(::LoadCursor(NULL, IDC_ARROW));
        bCursorInside = false;
        UpdateTitle();
    }
}

void CManFrame::XorFrame(CDC& dc, CPoint start, CPoint end)
{
    dc.SelectStockObject(WHITE_PEN);
    dc.SetROP2(R2_NOT);
    dc.MoveTo(start);
    dc.LineTo(start.x, end.y);
    dc.LineTo(end);
    dc.LineTo(end.x, start.y);
    dc.LineTo(start);
}

CPoint CManFrame::FixAspectRatio(CPoint anchor, CPoint drag)
{
    long dwid = (drag.x - anchor.x) * Picture->Height();
    long dhit = (drag.y - anchor.y) * Picture->Width();
    bool verse = (drag.x - anchor.x) * (drag.y - anchor.y) < 0;
    if (abs(dwid) < abs(dhit))
        drag.x = anchor.x + (verse ? -dhit : dhit) / Picture->Height();
    else
        drag.y = anchor.y + (verse ? -dwid : dwid) / Picture->Width();
    return drag;
}

void CManFrame::UpdateTitle()
{
    static bool wasinside = false;
    static bool wascalculating = false;
    static CPoint lastmouse(-1, -1);
    static int lastpermilage = -1;
    CPoint nowmouse;
    char bluf[256], fluf[64], centage[100];

    if (mister_Big.AnyCalculationsStillRunning())
        sprintf(centage, "   [%d.%d%%]", permil_done / 10, permil_done % 10);
    else if (!bCursorInside || !Picture)
        strcpy(centage, "  -  N = new canvas, B = set bounds, G = go, S = stop, M = more iterations");
    else
        centage[0] = '\0';
    if (!Picture)
        return;
    if (bCursorInside) {
        ::GetCursorPos(&nowmouse);
        ScreenToClient(&nowmouse);
        if ((nowmouse == lastmouse && bCursorInside == wasinside && lastpermilage == permil_done)
                    || !Picture->VisibleRect(this).PtInRect(nowmouse))
            return;
        lastmouse = nowmouse;
        int x = nowmouse.x - Picture->LeftOffset(), y = nowmouse.y - Picture->TopOffset();
        int sij = 4 - int(floor(log10(Picture->right - Picture->left)));
        if (sij > 16) sij = 16;
        long r = Picture->ResultAt(x, y);
        fluf[0] = '\0';
        if (r > 0 && !Picture->loop_length_only) {
            int mag = (r >> 20) & 0x000007FF;
            double diam = sqrt(UnTenBitMagnitude(mag));
            int dij = 4 - int(floor(log10(diam)));
            sprintf(fluf, ",  loop diameter %.*f   (%d)", min(dij, 16), diam, mag);
            r &= 0x000FFFFF;
        }
        sprintf(bluf, "%s%s       %.*f, %.*f    iteration count %d%s", basecaption, centage,
                sij, Picture->RealX(x), sij, Picture->RealY(y), r, fluf);
    } else {
        if (bCursorInside == wasinside && lastpermilage == permil_done)
            return;
        sprintf(bluf, "%s%s", basecaption, centage);
    }
    wasinside = bCursorInside;
    lastpermilage = permil_done;
    SetWindowText(bluf);
}    

BOOL CManFrame::OnSetCursor(CWnd* w, UINT h, UINT m)
{
    return h == HTCLIENT ? TRUE : CFrameWnd::OnSetCursor(w, h, m);
}

void CManFrame::OnSize(UINT nType, int cx, int cy)
{
    CFrameWnd::OnSize(nType, cx, cy);
    if (Picture && !bAlreadySizing) {
        CRect clyde;
        int p1, p2;
        bool hasHorz, hasVert;

        bAlreadySizing = true;
        // we calculate here what the client area would be if scrollbars were absent,
        // and see whether the scrollbars are really necessary to show
        GetScrollRange(SB_HORZ, &p1, &p2);
        hasHorz = p1 != 0 || p2 != 0;
        GetScrollRange(SB_VERT, &p1, &p2);
        hasVert = p1 != 0 || p2 != 0;
        GetClientRect(&clyde);
        cx = clyde.Width();
        if (hasVert)
            cx += ::GetSystemMetrics(SM_CXVSCROLL);
        cy = clyde.Height();
        if (hasHorz)
            cy += ::GetSystemMetrics(SM_CYHSCROLL);
        bool wide = Picture->Width() > cx;
        bool tall = Picture->Height() > cy;
        // now switch the scrollbars on or off as appropriate, re-get the
        // new improved client rect, and center the picture
        ShowScrollBar(SB_HORZ, wide);
        ShowScrollBar(SB_VERT, tall);
        GetClientRect(&clyde);
        cx = clyde.Width();
        cy = clyde.Height();
        Picture->SetTopLeftOffset((cx - Picture->Width()) / 2, (cy - Picture->Height()) / 2);
        // if the scrollbars are present, set up their paging parameters
        SCROLLINFO si = {sizeof(SCROLLINFO), SIF_ALL, 0, 100, 50, 50, 0};
        if (wide) {
            si.nMax = Picture->Width();
            si.nPage = cx;
            si.nPos = -Picture->LeftOffset();
            SetScrollInfo(SB_HORZ, &si, TRUE);
        }
        if (tall) {
            si.nMax = Picture->Height();
            si.nPage = cy;
            si.nPos = -Picture->TopOffset();
            SetScrollInfo(SB_VERT, &si, TRUE);
        }
        bAlreadySizing = false;
    }
}

void CManFrame::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* ctl)
{
    if (ctl || !Picture)
        CFrameWnd::OnHScroll(nSBCode, nPos, ctl);
    else {
        CRect cr;
        GetClientRect(cr);
        int p = GetScrollPos(SB_HORZ), p0 = p;
        int mack = Picture->Width() - cr.Width();
        switch (nSBCode) {
            case SB_LEFT:          p = 0; break;
            case SB_RIGHT:         p = mack; break;
            case SB_LINELEFT:      p = max(p - 16, 0); break;
            case SB_LINERIGHT:     p = min(p + 16, mack); break;
            case SB_PAGELEFT:      p = max(p - (cr.Width() - 16), 0); break;
            case SB_PAGERIGHT:     p = min(p + (cr.Width() - 16), mack); break;
            case SB_THUMBTRACK:    // /* for now */ return;
            case SB_THUMBPOSITION: p = nPos; break;
        }
        SetScrollPos(SB_HORZ, p, TRUE);
        ScrollWindow(p0 - p, 0, NULL, &cr);
        Picture->SetTopLeftOffset(-p, Picture->TopOffset());
    }
}

void CManFrame::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* ctl)
{
    if (ctl || !Picture)
        CFrameWnd::OnVScroll(nSBCode, nPos, ctl);
    else {
        CRect cr;
        GetClientRect(cr);
        int p = GetScrollPos(SB_VERT), p0 = p;
        int mack = Picture->Height() - cr.Height();
        switch (nSBCode) {
            case SB_LINEUP:        p = max(p - 16, 0); break;
            case SB_LINEDOWN:      p = min(p + 16, mack); break;
            case SB_TOP:           p = 0; break;
            case SB_BOTTOM:        p = mack; break;
            case SB_PAGEUP:        p = max(p - (cr.Height() - 16), 0); break;
            case SB_PAGEDOWN:      p = min(p + (cr.Height() - 16), mack); break;
            case SB_THUMBTRACK:    // /* for now */ return;
            case SB_THUMBPOSITION: p = nPos; break;
        }
        SetScrollPos(SB_VERT, p, TRUE);
        ScrollWindow(0, p0 - p, NULL, &cr);
        Picture->SetTopLeftOffset(Picture->LeftOffset(), -p);
    }
}

void CManFrame::OnPaint()
{
    // On some systems the thing speeds up a lot if we suspend the calculation thread(s)
    //mister_Big.SuspendThreads();
//  cprintf("     -- Painting time.  ");
    CPaintDC paindc(this);
    CRect rClient;
    GetClientRect(&rClient);
    if (!Picture)
        paindc.FillSolidRect(rClient, background);
    else {
        CRect rPaint(paindc.m_ps.rcPaint);
        CRect rPicture = Picture->VisibleRect(this);
        if (!rPicture.PtInRect(rPaint.TopLeft()) || !rPicture.PtInRect(rPaint.BottomRight())) {
            // Christ, in OWL you'd just go FillRgn(TRegion(rClient) - rPicture)...
            // but in MFC CRgn is such a pain it's not even worth using here.
            if (rPicture.Width() < rClient.Width()) {
                CRect rLeft(rClient.left, rClient.top, rPicture.left, rClient.bottom);
                paindc.FillSolidRect(&rLeft, background);
                CRect rRight(rPicture.right, rClient.top, rClient.right, rClient.bottom);
                paindc.FillSolidRect(&rRight, background);
            }
            if (rPicture.Height() < rClient.Height()) {
                int left = max(rPicture.left, rClient.left);
                int right = min(rPicture.right, rClient.right);
                CRect rTop(left, rClient.top, right, rPicture.top);
                paindc.FillSolidRect(&rTop, background);
                CRect rBottom(left, rPicture.bottom, right, rClient.bottom);
                paindc.FillSolidRect(&rBottom, background);
            }
        }
        // the above evasiveness is to avoid flicker in the picture area...
        // that stuff is all just for background areas; the actual painting of the
        // rendered image is just a quick little blit from the buffer bitmap
        if (pictureMutex.Lock(0))
        {
            CDC memdc;
            memdc.CreateCompatibleDC(&paindc);
            if (memdc.SelectObject(pictureBuffer)) {
    //          cprintf("Blitting from pictureBuffer.\r\n");
                paindc.BitBlt(Picture->LeftOffset(), Picture->TopOffset(),
                              Picture->Width(), Picture->Height(),
                              &memdc, 0, 0, SRCCOPY);
            } else {
    //          cprintf("NO pictureBuffer, filling in solid color.\r\n");
    //          InvalidateRect(&Picture->VisibleRect(this), FALSE);
                paindc.FillSolidRect(&Picture->VisibleRect(this), RGB(100, 200, 50));
            }
            pictureMutex.Unlock();
        }
    }
}

void CManFrame::EraseBuffer()
{
    CDC mdc;
    mdc.CreateCompatibleDC(&CClientDC(this));
    mdc.SelectObject(pictureBuffer);
    mdc.FillSolidRect(0, 0, Picture->width, Picture->height, Picture->ColorOfResult(0));
}

void CManFrame::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    if (point.x <= 0 && point.y <= 0) {        // invoked by keyboard
        point.x = point.y = 10;
        ClientToScreen(&point);
    }
    contextMenu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, 
                                              point.x, point.y, AfxGetMainWnd()); 
    CFrameWnd::OnContextMenu(pWnd, point); 
} 


void CManFrame::OnClose()
{
    if (!Picture || mister_Big.lastState == ready ||
                MessageBox("Really quit?", NAME_AND_VERSION, MB_ICONQUESTION | MB_YESNO) == IDYES) {
        mister_Big.StopCalculation();
        DestroyWindow();
    }
}

