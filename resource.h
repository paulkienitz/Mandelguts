#define NAME_AND_VERSION         "MandelGuts 1.2"

// main frame window's menu, caption, and accelerators:
#define IDR_MAINDEL              100
#define IDR_POPUP                101

// dialog IDs:
#define IDD_ABOUTDLG             1000
#define IDD_BOUNDSDLG            1001
#define IDD_NEWDLG               1002
#define IDD_MOREITERATIONSDLG    1003

// menu commands:
#define CM_MANDEL_NEW            4001
#define CM_MANDEL_SETBOUNDS      4002
#define CM_MANDEL_GO             4003
#define CM_MANDEL_STOP           4004
#define CM_MANDEL_MORE           4005

// internal signals -- TO BE PHASED OUT in favor of notifications below:
#define CM_UPDATETITLE           5000

// internal signals sent by WM_NOFITY:
#define NM_LINECOMPLETE          5300       // ID = zero-based thread number
#define NM_THREADCOMPLETE        5301       // ID = zero-based thread number


// dialog control IDs:
#define IDC_BOUNDS_LEFT          8010
#define IDC_BOUNDS_BOTTOM        8020
#define IDC_BOUNDS_RIGHT         8030
#define IDC_BOUNDS_TOP           8040
#define IDC_BOUNDS_REALCENTER    8050
#define IDC_BOUNDS_IMAGCENTER    8060
#define IDC_BOUNDS_WIDTH         8070
#define IDC_BOUNDS_REALSLIDER    8080
#define IDC_BOUNDS_IMAGSLIDER    8090
#define IDC_BOUNDS_WIDTHSLIDER   8100
#define IDC_BOUNDS_ITERATIONS    8110
#define IDC_BOUNDS_LOOPMAX       8120
#define IDC_BOUNDS_PIXELSIZE     8130
#define IDC_BOUNDS_LENGTHONLY    8140

#define IDC_MORE_ITERATIONS      8500
#define IDC_MORE_LOOPMAX         8510
