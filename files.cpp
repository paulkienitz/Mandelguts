// files.cpp: load and save mandel data, save images as .bmps

#include "mandel.h"

bool WriteBmpFile(CFrameWnd* frame, CBitmap* cbm, const char* tempath, const char* path)
{
    BITMAP bm;
    int y;
    int bytewid, longwid, height;
    CFile *ff;
    ULONG headsize, outbufsize;
	BYTE *outbuf, *outbufend;
    BITMAPFILEHEADER bmfh;
    BITMAPINFOHEADER bmih;

    cbm->GetBitmap(&bm);                    // note that this leaves bmBits null
    bytewid = 3 * bm.bmWidth;
    longwid = ((bytewid + 3) / 4) * 4;
    height = abs(bm.bmHeight);
    bmih.biSize = sizeof(BITMAPINFOHEADER);
    bmih.biWidth = bm.bmWidth;
    bmih.biHeight = height;
    bmih.biPlanes = 1;
    bmih.biBitCount = 24;
    bmih.biCompression = BI_RGB;            // we always write 24 bits though 8 commonly suffices
    bmih.biSizeImage = longwid * height;
    bmih.biXPelsPerMeter = bmih.biYPelsPerMeter = 2835;
    bmih.biClrUsed = bmih.biClrImportant = 0;
	outbufsize = longwid * height;
	outbuf = new BYTE[outbufsize];
    if (!outbuf) {
        frame->MessageBox("Could not save -- no buffer memory",
                          NAME_AND_VERSION, MB_OK | MB_ICONEXCLAMATION);
        return false;
    }
    HDC thdc = ::CreateCompatibleDC(NULL);
    outbufend = outbuf;
    for (y = 0; y < height; y++) {
        if (!::GetDIBits(thdc, (HBITMAP) *cbm, y, 1, outbufend, (BITMAPINFO*) &bmih, DIB_RGB_COLORS))
        {
            frame->MessageBox("Could not extract pixels", NAME_AND_VERSION, MB_OK | MB_ICONEXCLAMATION);
            delete[] outbuf;
            return false;
        }
        outbufend += longwid;
    }

    headsize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfh.bfType = 'B' | ('M' << 8);
    bmfh.bfOffBits = headsize;
    bmfh.bfSize = bmfh.bfOffBits + (outbufend - outbuf);
    bmfh.bfReserved1 = bmfh.bfReserved2 = 0;
    if (!tempath || !tempath[0])
        tempath = path;
    try {
        ff = new CFile(tempath, CFile::modeCreate | CFile::modeWrite);
    } catch (...) {
        if (tempath == path)
            frame->MessageBox("Could not open output file",
                              NAME_AND_VERSION, MB_OK | MB_ICONEXCLAMATION);
        else
            frame->MessageBox("Could not create temporary output file",
                              NAME_AND_VERSION, MB_OK | MB_ICONEXCLAMATION);
        delete[] outbuf;
        return false;
    }
    try {
        ff->Write(&bmfh, sizeof(BITMAPFILEHEADER));
        ff->Write(&bmih, sizeof(BITMAPINFOHEADER));
    } catch (...) {
        frame->MessageBox("Could not write file",
                          NAME_AND_VERSION, MB_OK | MB_ICONEXCLAMATION);
        delete[] outbuf;
        ff->Close();
        delete ff;
        return false;
    }
    try {
        ff->Write(outbuf, outbufend - outbuf);
    } catch (...) {
        frame->MessageBox("Could not write file (header OK, failure in pixel data)",
                          NAME_AND_VERSION, MB_OK | MB_ICONEXCLAMATION);
        delete[] outbuf;
        ff->Close();
        delete ff;
        return false;
    }
    delete[] outbuf;
    ff->Close();
    delete ff;
    return true;
}
