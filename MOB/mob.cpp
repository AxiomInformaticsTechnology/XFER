
#include "mob.h"

#define PALVERSION   0x300

#define SCROLL_RATIO    8
#define RECTWIDTH(lpRect)     ((lpRect)->right - (lpRect)->left)
#define RECTHEIGHT(lpRect)    ((lpRect)->bottom - (lpRect)->top)

int   WINAPI  WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow );
BOOL          CheckPreviousApp(void);
BOOL          InitMainWindow(void);
void          RunMain(void);

int       g_nExitCode     = 0;

HCURSOR   g_hGadgetCursor = NULL;
TCHAR     g_szGadgetClass[] = _T("MOB_GADGET");
TCHAR     g_szGadgetTitle[] = _T("MOB");
HWND      g_hGadgetWindow   = NULL;

TCHAR     g_szMainClass[] = _T("PDA_MAIN");
TCHAR     g_szMainTitle[] = _T("PDA");

HCURSOR   g_hMainCursor   = NULL;
HINSTANCE g_hMainInstance = NULL;
HACCEL    g_hMainAccel    = NULL;
HWND      g_hMainWindow   = NULL;
HMENU     g_hMenu         = NULL;
HDC       g_hMemDC  = NULL;
HBITMAP   g_hBM     = NULL;
HBITMAP   g_bmPB    = NULL;
COLORREF  g_crColor = NULL;
COLORREF  g_brColor = NULL;
HBRUSH    g_hBB     = NULL;
HPEN      g_hSP     = NULL;
HPALETTE  g_hPal    = NULL;         // Handle to our bitmap's palette

void DoSize(HWND hWnd);
void DoPaint(HWND hWnd);
void DoScroll(HWND hWnd, int message, int wPos, int wScrollType);
void SetupScrollBars(HWND hWnd, WORD cxBitmap, WORD cyBitmap);
BOOL PaintBitmap(HWND hWnd, HDC hDC, LPRECT lpDCRect, LPRECT lpDDBRect,HDC hMemDC, HBITMAP hDDB, HPALETTE hPal);


BOOL Closable(int mode)
{
  static BOOL CloseFlag = TRUE; 
  if ( mode == 0 && CloseFlag == TRUE ) return TRUE;
  if ( mode == 1 ) return CloseFlag;
  if ( mode == 2 ) { BOOL res = CloseFlag; CloseFlag = FALSE; return res;}
  if ( mode == 3 ) { BOOL res = CloseFlag; CloseFlag = TRUE;  return res;}
  return FALSE;
}

int PalEntriesOnDevice(HDC hDC)
{
  int nColors;
  nColors = (1 << (GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES)));
  return nColors;
}

HPALETTE GetSystemPalette(void)
{
  static HPALETTE hPal = NULL;
  
  HDC             hDC;
  HANDLE          hLogPal;
  LPLOGPALETTE    lpLogPal;
  int             nColors;

  hDC = GetDC(NULL);

  if (!hDC) return NULL;

  nColors = PalEntriesOnDevice(hDC);

  hLogPal = GlobalAlloc(GHND, sizeof(LOGPALETTE) + nColors * sizeof(PALETTEENTRY));

  if (!hLogPal) return NULL;


  lpLogPal = (LPLOGPALETTE)GlobalLock(hLogPal);
  lpLogPal->palVersion = PALVERSION;
  lpLogPal->palNumEntries = nColors;

  GetSystemPaletteEntries(hDC, 0, nColors, (LPPALETTEENTRY)(lpLogPal->palPalEntry));

  hPal = CreatePalette(lpLogPal);

  GlobalUnlock(hLogPal);
  GlobalFree(hLogPal);
  ReleaseDC(NULL, hDC);

  return hPal;
}

void ReallyGetClientRect(HWND hWnd, LPRECT lpRect)
{
  DWORD dwWinStyle = GetWindowLong(hWnd, GWL_STYLE);
  GetClientRect(hWnd, lpRect);
  if (dwWinStyle & WS_HSCROLL) lpRect->bottom += (GetSystemMetrics(SM_CYHSCROLL) - 1);
  if (dwWinStyle & WS_VSCROLL) lpRect->right  += (GetSystemMetrics(SM_CXVSCROLL) - 1);
}

void SetupScrollBars(HWND hWnd, WORD cxBitmap, WORD cyBitmap)
{
  RECT        rect;
  BOOL        bNeedScrollBars=FALSE;
  unsigned    cxWindow, cyWindow;
  int         cxRange=0, cyRange=0;

  ReallyGetClientRect(hWnd, &rect);
  cxWindow = rect.right - rect.left;
  cyWindow = rect.bottom - rect.top;

  if ((cxWindow < (unsigned)cxBitmap) || (cyWindow < (unsigned)cyBitmap))
  {
    bNeedScrollBars = TRUE;
  }
  if (bNeedScrollBars)
  {
    cyRange = (unsigned)cyBitmap - cyWindow - 1 + GetSystemMetrics(SM_CYHSCROLL);
    cxRange = (unsigned)cxBitmap - cxWindow - 1 + GetSystemMetrics(SM_CXVSCROLL);
  }
  SetScrollRange(hWnd, SB_VERT, 0, cyRange, TRUE);
  SetScrollRange(hWnd, SB_HORZ, 0, cxRange, TRUE);
}

BOOL PaintBitmap(HWND hWnd, HDC hDC, LPRECT lpDCRect, LPRECT lpDDBRect,HDC hMemDC, HBITMAP hDDB, HPALETTE hPal)
{
    
//HBITMAP     hOldBitmap;
  HPALETTE    hOldPal1 = NULL;
  HPALETTE    hOldPal2 = NULL;
  BOOL        bSuccess = FALSE;

//hMemDC = CreateCompatibleDC(hDC);

  if (!hMemDC) return FALSE;

  if (hPal)
  {
    hOldPal1 = SelectPalette(hMemDC, hPal, TRUE);
    hOldPal2 = SelectPalette(hDC,    hPal, TRUE);
    RealizePalette(hDC);
  }

//hOldBitmap = (HBITMAP)SelectObject(hMemDC, hDDB);

  SetStretchBltMode(hDC, COLORONCOLOR);

  if (( (RECTWIDTH (lpDCRect)) == (RECTWIDTH (lpDDBRect)) ) && 
      ( (RECTHEIGHT(lpDCRect)) == (RECTHEIGHT(lpDDBRect)) ) )
  {
/*
    RECT rc;
    GetClientRect(hWnd, &rc);
    int sx=0, sy=0;
    if ( (rc.right-rc.left) > (lpDCRect->right-lpDCRect->left) )
    {
      sx = ((rc.right-rc.left) - (lpDCRect->right-lpDCRect->left))/2;
    }

    if ( (rc.bottom-rc.top) > (lpDCRect->bottom-lpDCRect->top) )
    {
      sy = ((rc.bottom-rc.top) - (lpDCRect->bottom-lpDCRect->top))/2;
    }
*/
    bSuccess = BitBlt(hDC, 
                      lpDCRect->left, 
                      lpDCRect->top,
                      lpDCRect->right - lpDCRect->left,
                      lpDCRect->bottom - lpDCRect->top, 
                      hMemDC, 
                      lpDDBRect->left,
                      lpDDBRect->top, 
                      SRCCOPY);
  }
  else
  {
    bSuccess = StretchBlt(hDC,  
                          lpDCRect->left,
                          lpDCRect->top, 
                          lpDCRect->right - lpDCRect->left,
                          lpDCRect->bottom - lpDCRect->top, 
                          hMemDC, 
                          lpDDBRect->left, 
                          lpDDBRect->top,
                          lpDDBRect->right - lpDDBRect->left,
                          lpDDBRect->bottom - lpDDBRect->top,
                          SRCCOPY);
  }

//SelectObject(hMemDC, hOldBitmap);

  if (hOldPal1) SelectPalette(hMemDC, hOldPal1, FALSE);
  if (hOldPal2) SelectPalette(hDC,    hOldPal2, FALSE);

//DeleteDC(hMemDC);

  return bSuccess;
}

void DoScroll(HWND hWnd, int message, int wPos, int wScrollType)
{
  int  xBar;
  int  nMin;
  int  nMax;
  int  dx;
  int  nOneUnit;
  int  cxClient;
  int  nHorzOrVert;
  RECT rect;
  GetClientRect(hWnd, &rect);

  if (message == WM_HSCROLL)
  {
    nHorzOrVert = SB_HORZ;
    cxClient    = rect.right - rect.left;
  }
  else
  {
    nHorzOrVert = SB_VERT;
    cxClient    = rect.bottom - rect.top;
  }

  nOneUnit = cxClient / SCROLL_RATIO;
  if (!nOneUnit) nOneUnit = 1;

  xBar = GetScrollPos(hWnd, nHorzOrVert);
  GetScrollRange(hWnd, nHorzOrVert, &nMin, &nMax);

  switch (wScrollType)
  {
    case SB_LINEDOWN:             // One line right.
         dx = nOneUnit;
         break;

    case SB_LINEUP:               // One line left.
         dx = -nOneUnit;
         break;

    case SB_PAGEDOWN:             // One page right.
         dx = cxClient;
         break;

    case SB_PAGEUP:               // One page left.
         dx = -cxClient;
         break;

    case SB_THUMBPOSITION:        // Absolute position.
         dx = wPos - xBar;
         break;

    default:                      // No change.
         dx = 0;
         break;
  }

  if (dx)
  {
    xBar += dx;

    if (xBar < nMin)
    {
      dx  -= xBar - nMin;
      xBar = nMin;
    }

    if (xBar > nMax)
    {
      dx  -= xBar - nMax;
      xBar = nMax;
    }

    if (dx)
    {
      SetScrollPos(hWnd, nHorzOrVert, xBar, TRUE);
      if (nHorzOrVert == SB_HORZ)
      {
        ScrollWindow(hWnd, -dx, 0, NULL, NULL);
      }
      else
      {
        ScrollWindow(hWnd, 0, -dx, NULL, NULL);
      }
      UpdateWindow(hWnd);
    }
  }
}

void DoSize(HWND hWnd)
{
     
  if (g_hBM)
  {
    BITMAP      bm;
    int         cxBitmap=0, cyBitmap=0;
    int         cxScroll, cyScroll;
    RECT        rect;
    GetObject(g_hBM, sizeof(BITMAP), (LPSTR)&bm);
    cxBitmap = bm.bmWidth;
    cyBitmap = bm.bmHeight;

    GetClientRect(hWnd, &rect);

    cxScroll = GetScrollPos(hWnd, SB_HORZ);
    cyScroll = GetScrollPos(hWnd, SB_VERT);

    if (cxScroll + rect.right > cxBitmap || cyScroll + rect.bottom > cyBitmap)
    {
      InvalidateRect(hWnd, NULL, FALSE);
    }
    else
    {

    }
    SetupScrollBars(hWnd, (WORD)cxBitmap, (WORD)cyBitmap);
  }
  else
  {
    SetScrollRange(hWnd, SB_VERT, 0, 0, TRUE);
    SetScrollRange(hWnd, SB_HORZ, 0, 0, TRUE);
  }
}

void DoPaint(HWND hWnd)
{
  HDC             hDC;
  PAINTSTRUCT     ps;
  BITMAP          bm;
  RECT            rectClient, rectDDB;
  int             xScroll, yScroll;

  hDC = BeginPaint(hWnd, &ps);
  if (!g_hBM)
  {
    SetScrollRange(hWnd, SB_VERT, 0, 0, TRUE);
    SetScrollRange(hWnd, SB_HORZ, 0, 0, TRUE);
  }
  else
  {
    GetObject(g_hBM, sizeof(BITMAP), (LPSTR)&bm);
    xScroll  = GetScrollPos(hWnd, SB_HORZ);
    yScroll  = GetScrollPos(hWnd, SB_VERT);
    SetupScrollBars(hWnd, (WORD)bm.bmWidth, (WORD)bm.bmHeight);
    GetClientRect(hWnd, &rectClient);

    rectDDB.left   = xScroll;
    rectDDB.top    = yScroll;
    rectDDB.right  = xScroll + rectClient.right - rectClient.left;
    rectDDB.bottom = yScroll + rectClient.bottom - rectClient.top;

    if (rectDDB.right > bm.bmWidth)
    {
      int dx;
      dx = bm.bmWidth - rectDDB.right;
      rectDDB.right     += dx;
      rectClient.right  += dx;
    }

    if (rectDDB.bottom > bm.bmHeight)
    {
      int dy;
      dy = bm.bmHeight - rectDDB.bottom;
      rectDDB.bottom    += dy;
      rectClient.bottom += dy;
    }

//  BitBlt(hDC,rectClient.left ,rectClient.top, rectClient.right,rectClient.bottom, g_hMemDC,0,0,SRCCOPY);
    PaintBitmap(hWnd, hDC, &rectClient, &rectDDB, g_hMemDC, g_hBM, g_hPal);
  }
  EndPaint(hWnd, &ps);
}

LRESULT CALLBACK MOBWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static int PqmRes = 0; 
  switch(uMsg)
  {
//    case WM_SIZE:
//      {
//        static BOOL InSize = 0;
//        if ( InSize == FALSE )
//        {
//          InSize = TRUE;
//          DoSize(hWnd);
//          InSize = FALSE;
//        }
//        return 0;
//      }

    case WM_HSCROLL:
    case WM_VSCROLL:
          DoScroll(hWnd, uMsg, (int)HIWORD(wParam), (int)LOWORD(wParam));
          break;

    case WM_NCPAINT:
    case WM_ERASEBKGND:
          return 0;
    case WM_PAINT:
      //    DoPaint(hWnd);
          return 0;

    case WM_CREATE:
      {
        HDC     hDC     = GetDC(hWnd);                         if (hDC   ==NULL) { return 1; }
        HDC     hMemDC  = CreateCompatibleDC(hDC);             if (hMemDC==NULL) { ReleaseDC(hWnd,hDC); return 1; }
        HDC     hPdaDC  = CreateCompatibleDC(hDC);             if (hPdaDC==NULL) { DeleteDC(hMemDC);ReleaseDC(hWnd,hDC); return 1; }
        HBITMAP hBM     = CreateCompatibleBitmap(hDC,1500,1500); if (hBM   ==NULL) { DeleteDC(hMemDC);DeleteDC(hPdaDC); ReleaseDC(hWnd,hDC); return 1; }
             
             g_hPal     = GetSystemPalette();
             g_hMemDC   = hMemDC;
             g_hBM      = hBM;
             g_brColor  = RGB(0xC0,0xC0,0xC0);
             g_hBB      = CreateSolidBrush(g_brColor);
             g_crColor  = RGB(0xFF,0xFF,0xFF);
             g_hSP      = CreatePen(PS_SOLID,1,g_crColor);

             g_bmPB     = LoadBitmap(g_hMainInstance,MAKEINTRESOURCE(IDB_PDABUSY));
        HBRUSH  hBrush  = (HBRUSH)SelectObject(hMemDC,g_hBB);
        HPEN    hPen    = (HPEN)SelectObject(hMemDC,g_hSP);
        HBITMAP hOBm    = (HBITMAP)SelectObject(hMemDC,g_hBM);
        HBITMAP hOpd    = (HBITMAP)SelectObject(hPdaDC,g_bmPB);
                          
                          Rectangle(hMemDC,0   ,0   ,1500,1500);
                          BitBlt(hMemDC,  10,  10,24*4,24*4,hPdaDC,0,0,SRCCOPY);
                          BitBlt(hMemDC,1490-24*4,1490-24*4,24*4,24*4,hPdaDC,0,0,SRCCOPY);
                          ReleaseDC(hWnd,hDC);

      }
      return 0;

    case WM_DESTROY:
      break;

    case WM_CLOSE:
      break;

    case WM_COMMAND:
      {
        HWND  hControl  = (HWND)lParam;
        DWORD dwNotify  = (DWORD)HIWORD(wParam);
        DWORD dwID      = (DWORD)LOWORD(wParam);
        if (hControl)
        {

        }
        else
        {
          switch (dwID)
          { 
            case IDM_JULIA:
              if(g_hGadgetWindow)
              {
                //do something with bm
                POINT oldPt;
                BOOL  flag = MoveToEx(g_hMemDC,0,0,&oldPt);
                
                LineTo(g_hMemDC,1000,1000);
                //UpdateWindow(hWnd);
                InvalidateRect(g_hGadgetWindow, NULL, TRUE);

              }
              return 0;  
          }
      break;
        }
      }
  }
  return DefWindowProc(hWnd, uMsg, wParam, lParam); 
}

BOOL InitGadgetWindow(HWND hParentWnd)
{ 
  HWND      hWnd;
  WNDCLASS  wc;
  RECT      rc;
  RECT      rcd;
  GetClientRect(GetDesktopWindow(),&rcd);
  GetClientRect(hParentWnd, &rc);

  DWORD     dwStyleEx = 0;      //WS_EX_APPWINDOW; 
  DWORD     dwStyle   = WS_CHILD|WS_VSCROLL|WS_HSCROLL;

  DWORD     xPos      = rc.left   + 50;
  DWORD     yPos      = rc.top    + 50;
  DWORD     nWidth    = rc.right  - 100;
  DWORD     nHeight   = rc.bottom - 100;

  g_hGadgetCursor     = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
  
  wc.style            = CS_DBLCLKS;
  wc.lpfnWndProc      = (WNDPROC)MOBWindowProc;
  wc.cbClsExtra       = 0;
  wc.cbWndExtra       = 0;
  wc.hInstance        = g_hMainInstance;
  wc.hIcon            = LoadIcon(g_hMainInstance, MAKEINTRESOURCE(IDR_MAIN_ICON));
  wc.hCursor          = g_hGadgetCursor;
  wc.hbrBackground    = (HBRUSH)GetStockObject(GRAY_BRUSH);
  wc.lpszMenuName     = NULL;
  wc.lpszClassName    = g_szGadgetClass;
  if (!RegisterClass(&wc))
  {
    g_nExitCode = -5;
    return FALSE;
  }

  hWnd = CreateWindowEx(dwStyleEx,
                        g_szGadgetClass,
                        g_szGadgetTitle,
                        dwStyle,
                        xPos,
                        yPos,
                        nWidth,
                        nHeight,
                        hParentWnd,
                        (HMENU)1,
                        g_hMainInstance,
                        NULL);
  g_hGadgetWindow = hWnd;
  if (!hWnd)
  {
    g_nExitCode = -6;
    return FALSE;
  }
  ShowWindow(hWnd, SW_SHOWDEFAULT);
  UpdateWindow(hWnd);
  return TRUE;
}

LRESULT CALLBACK PDAWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static int PqmRes = 0; 
  switch(uMsg)
  {
    case WM_CREATE:
      return (InitGadgetWindow(hWnd)==TRUE) ? 0 : -1;  

    case WM_DESTROY:
      PostQuitMessage(PqmRes);
      break;

    case WM_PAINT:
      if ( GetUpdateRect(hWnd,NULL,FALSE) == TRUE )
      {

        //HDC             hDC;
        //PAINTSTRUCT     ps;
        //hDC = BeginPaint(hWnd, &ps);
        if(g_hGadgetWindow)
        {
          DoPaint(g_hGadgetWindow);
        }
        //EndPaint(hWnd, &ps);
      }
      else
      {
        
      }
        return 0;

    case WM_CLOSE:
      if ( Closable(0) )
      {
        DestroyWindow(hWnd);
      }
      break;

    case WM_COMMAND:
      {
        HWND  hControl  = (HWND)lParam;
        DWORD dwNotify  = (DWORD)HIWORD(wParam);
        DWORD dwID      = (DWORD)LOWORD(wParam);
        if (hControl)
        {

        }
        else
        {
          switch (dwID)
          { 
            case IDM_EXIT:
              Closable(3);  //force closable
              SendMessage(hWnd, WM_CLOSE, 0, 0);
              return 0;  
            case IDM_JULIA:
              if(g_hGadgetWindow)
              {
                SendMessage(g_hGadgetWindow, WM_COMMAND, IDM_JULIA, 0);
              }
              return 0;  
          }
        }
      }
      break;
  }
  return DefWindowProc(hWnd, uMsg, wParam, lParam); 
}


void OnIdle(HWND hWindow)
{
/*
  HRESULT       hResult;
  LPD3DWindow   lpd3dWindow;
  lpd3dWindow = (LPD3DWindow)GetWindowLong(hWindow, GWL_USERDATA);
  if (! lpd3dWindow) return;
  if ((!lpd3dWindow->isActive()) ||
      ( lpd3dWindow->isPaused()) || 
      (!lpd3dWindow->isValid())  || (g_fMinimized)) return;
  hResult = lpd3dWindow->DrawFrame();
  if (FAILED(hResult)) return;
  */
  return;
}

void RunMain(void)
{
  MSG msg;
  if ((!g_hMainInstance) || (!g_hMainWindow))
  {
    g_nExitCode = -4;
    return;
  }
  g_hMainAccel = LoadAccelerators(g_hMainInstance, MAKEINTRESOURCE(IDR_MAIN_ACCEL));
  while(TRUE)
  {
    while(TRUE)
    {
      if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      {
        break;
      }
      if (msg.message == WM_QUIT)
      {
        g_nExitCode = msg.wParam;
        return;
      }
      if (g_hMainAccel)
      {
        if (!TranslateAccelerator(g_hMainWindow, g_hMainAccel, &msg))
        {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }
      }
      else
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
    OnIdle(g_hMainWindow);
  }
}

BOOL InitMainWindow(void)
{ 
  HWND      hWnd;
  WNDCLASS  wc;

  DWORD     dwStyleEx = 0;      //WS_EX_APPWINDOW; 
  DWORD     dwStyle   = WS_OVERLAPPEDWINDOW;  

  DWORD     xPos      = CW_USEDEFAULT;
  DWORD     yPos      = CW_USEDEFAULT;
  DWORD     nWidth    = CW_USEDEFAULT;
  DWORD     nHeight   = CW_USEDEFAULT;

  g_hMainCursor       = LoadCursor((HINSTANCE)NULL, IDC_ARROW);

  g_hMenu             = LoadMenu((HINSTANCE)NULL, MAKEINTRESOURCE(IDR_MAIN_MENU));
  
  wc.style            = CS_DBLCLKS;
  wc.lpfnWndProc      = (WNDPROC)PDAWindowProc;
  wc.cbClsExtra       = 0;
  wc.cbWndExtra       = 0;
  wc.hInstance        = g_hMainInstance;
  wc.hIcon            = LoadIcon(g_hMainInstance, MAKEINTRESOURCE(IDR_MAIN_ICON));
  wc.hCursor          = g_hMainCursor;
  wc.hbrBackground    = (HBRUSH)GetStockObject(GRAY_BRUSH);
  wc.lpszMenuName     = MAKEINTRESOURCE(IDR_MAIN_MENU);
  wc.lpszClassName    = g_szMainClass;
  if (! RegisterClass(&wc))
  {
    g_nExitCode = -2;
    return FALSE;
  }

  hWnd = CreateWindowEx(dwStyleEx,
                        g_szMainClass,
                        g_szMainTitle,
                        dwStyle,
                        xPos,
                        yPos,
                        nWidth,
                        nHeight,
                        GetDesktopWindow(),
                        g_hMenu,
                        g_hMainInstance,
                        NULL);
  g_hMainWindow = hWnd;
  if (!hWnd)
  {
    g_nExitCode = -3;
    return FALSE;
  }
  ShowWindow(hWnd, SW_SHOWDEFAULT);
  UpdateWindow(hWnd);
  return TRUE;
}

BOOL CheckPreviousApp(void)
{
  HWND  hwndFind;
  HWND  hwndLast;
  HWND  hwndForeGround;
  DWORD dwFindID;
  DWORD dwForeGroundID;

  hwndFind = FindWindow(g_szMainClass, g_szMainTitle);
  if (hwndFind)
  {
    hwndForeGround = GetForegroundWindow();
    dwForeGroundID = GetWindowThreadProcessId(hwndForeGround, NULL);
    dwFindID       = GetWindowThreadProcessId(hwndFind, NULL);
    if ((dwFindID != dwForeGroundID) || (IsIconic(hwndFind)))
    {
      hwndLast = GetLastActivePopup(hwndFind);
      if (IsIconic(hwndLast))
      {
        ShowWindow(hwndLast, SW_RESTORE);
      }              
      BringWindowToTop(hwndLast);
      SetForegroundWindow(hwndLast);
    }
    g_nExitCode = -1;
    return FALSE;
  }
  return TRUE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
  g_hMainInstance = hInstance;

  if ( !CheckPreviousApp() ) return g_nExitCode;
  if ( !InitMainWindow() ) return g_nExitCode;
  RunMain();
  return g_nExitCode;
}

