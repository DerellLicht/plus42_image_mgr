//*****************************************************************
//  Display reference image in window
//                                           
//  Written by:   Daniel D. Miller           
//                                           
//  Last Update:  02/27/25
//*****************************************************************

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <objidl.h>
#include <gdiplus.h>

using namespace Gdiplus;

// #include "version.h"
#include "resource.h"
#include "common.h"
#include "commonw.h"
#include "pimage_mgr.h"
#include "winmsgs.h"
#include "wthread.h"

//lint -esym(551, hwndRef, refImage)
//lint -esym(844, hwndRef, refImage)

//  comment this line out to draw black window background 
// #define USE_SYS_BG_COLOR  1

static CThread const *ref_image_thread = NULL ;
//***********************************************************************
// static int cxClient = 0 ;
// static int cyClient = 0 ;

// unsigned xbase, xdiff, ybase, ydiff ;

HWND hwndRef = NULL ;

// static TCHAR tempstr[128] ;

// static CStatusBar *MainStatusBar = NULL;

// static LodePng *refImage = NULL ;
// static std::vector<unsigned char> image; //the raw pixels
static unsigned width = 0, height = 0 ;
static unsigned cli_width = 0, cli_height = 0 ;
//lint -esym(844, ref_image_thread)
static TCHAR ref_image_file[MAX_PATH_LEN]  = _T("") ;
static Image *ref_image = NULL;

//***********************************************************************
static void resize_window_corrected(HWND hwnd)
{
   //  unfortunately, this resize operation, which calls SetWindowPos(),
   // sets the *outside* window area, not the internal client area...
   resize_window(hwnd, width, height);
   
   //  so we have to calculate the error and correct it here
   RECT myRect ;
   GetClientRect(hwnd, &myRect) ;
   uint cxClient = (myRect.right  - myRect.left) ;
   uint cyClient = (myRect.bottom - myRect.top) ;
   // termout("Ref Image window client area: %ux%u\n", cxClient, cyClient);
   cli_width  = width  + (width  - cxClient) ;
   cli_height = height + (height - cyClient) ;
   // termout("Ref Image window tweaked size: %ux%u\n", cli_width, cli_height);
   
   //  now re-resize with adjusted values
   resize_window(hwnd, cli_width, cli_height);
}
      
//***********************************************************************
static void do_init_dialog(HWND hwnd)
{
   SetWindowText(hwnd, ref_image_file) ;

   hwndRef = hwnd ;
   SetClassLong(hwnd, GCL_HICON,   (LONG) LoadIcon(g_hinst, (LPCTSTR)IDI_PLUS42IM));
   SetClassLong(hwnd, GCL_HICONSM, (LONG) LoadIcon(g_hinst, (LPCTSTR)IDI_PLUS42IM));
   
   //  Disable the CLOSE button on title bar;
   //  Window should be closed by command from main dialog
   EnableMenuItem(GetSystemMenu(hwnd, FALSE), SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

   center_window(hwnd, -1, 650) ;
   
   //  this does, indeed, draw the png image...
//    HDC hdc = GetDC(hwnd) ;
//    refImage->render_bitmap(hdc, 0, 0, 0) ;
//    ReleaseDC(hwnd, hdc) ;
}

//***********************************************************************************
#undef  DRAW_TEST_IMAGES   

static VOID OnPaint(HDC hdc)
{
   Graphics    graphics(hdc);
   
#ifdef  DRAW_TEST_IMAGES   
   Image *binclock_image = NULL;
   SolidBrush  brush(Color(255, 0, 0, 255));
   FontFamily  fontFamily(L"Times New Roman");
   Font        font(&fontFamily, 24.0, FontStyleRegular, UnitPixel); //lint !e747 !e641
   PointF      pointF(10.0f, 20.0f);
   
   graphics.DrawString(L"gdiplus practice page", -1, &font, pointF, &brush);
   
   Pen      pen(Color(255, 0, 0, 255));
   graphics.DrawLine(&pen, 220, 220, 250, 100);
   
   SolidBrush redBrush(Color::Red);            
   Pen greenPen(Color::Green, 2.0); //lint !e747
   graphics.FillRectangle(&redBrush, 120, 120, 100, 100);
   graphics.DrawEllipse(&greenPen, 300, 50, 80, 80);   
   graphics.DrawEllipse(&greenPen, 300, 200, 80, 180);
   
   // Image image(L"binclock.gif");
   // graphics.DrawImage(&image, 30, 500);
   
   binclock_image = new Image(L"binclock.gif");
   // termout(_T("image size: %u x %u"), binclock_image->GetWidth(), binclock_image->GetHeight()); //lint !e864
   graphics.DrawImage(binclock_image, 420, 200);
#else   
   graphics.DrawImage(ref_image, 0, 0);
#endif     
}

/************************************************************************/
//lint -esym(578, y0, y1)
void Box(HWND hwnd, int x0, int y0, int dx, int dy, COLORREF rColor)
{
   HDC hdc = GetDC (hwnd) ;
   ul2uc_t uconv ;
   uconv.ul = (uint) rColor ;
   // uconv.uc[3] = 255 ;
   
   Graphics    graphics(hdc);
   
   // Pen greenPen(Color::Green, 2.0); //lint !e747
   // Pen      pen(Color(255, 0, 0, 255), 2.0);
   // Pen Pen(Color(uconv.uc[3], uconv.uc[0], uconv.uc[1], uconv.uc[2] ), 2.0); //lint !e747
   // Pen pen(Color(uconv.uc[3], uconv.uc[0], uconv.uc[1], uconv.uc[2])); //lint !e747
   Pen pen(Color(uconv.uc[0], uconv.uc[1], uconv.uc[2])); //lint !e747
   graphics.DrawRectangle(&pen, x0, y0, dx, dy);
   ReleaseDC (hwnd, hdc) ;
}

//***********************************************************************
static LRESULT CALLBACK RefImageProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
   //***************************************************
   //  debug: log all windows messages
   //***************************************************
   if (dbg_flags & DBG_WINMSGS) {
      switch (iMsg) {
      //  list messages to be ignored
      case WM_CTLCOLORBTN:
      case WM_CTLCOLORSTATIC:
      case WM_CTLCOLOREDIT:
      case WM_CTLCOLORDLG:
      case WM_MOUSEMOVE:
      case WM_NCMOUSEMOVE:
      case WM_NCHITTEST:
      case WM_SETCURSOR:
      case WM_ERASEBKGND:
      case WM_TIMER:
      case WM_NOTIFY:
      case WM_COMMAND:  //  prints its own msgs below
         break;
      default:
         syslog(_T("Ref [%s]\n"), lookup_winmsg_name(iMsg)) ;
         break;
      }
   }

   switch(iMsg) {
   case WM_INITDIALOG:
      {
      TCHAR *refImageFile = (TCHAR *) lParam ;
      _tcscpy(ref_image_file, refImageFile);
      termout(_T("target file: %s"), ref_image_file);
      ref_image = new Image(ref_image_file); //lint !e1025
      width  = ref_image->GetWidth();
      height = ref_image->GetHeight();
      termout(_T("ref image size: %u x %u"), width, height);
   
      do_init_dialog(hwnd) ;
      resize_window_corrected(hwnd);
      center_window(hwnd, -1, 650) ;
      UpdateWindow(hwnd);
      }
      return TRUE;

   case WM_PAINT:
      {
      PAINTSTRUCT  ps;
      HDC hdc = BeginPaint(hwnd, &ps);
      OnPaint(hdc);
      EndPaint(hwnd, &ps);
      }
      return 0;
      
   case WM_COMMAND:
      {  //  create local context
      DWORD cmd = HIWORD (wParam) ;
      DWORD target = LOWORD(wParam) ;

      switch (cmd) {
      case FVIRTKEY:  //  keyboard accelerators: WARNING: same code as CBN_SELCHANGE !!
         //  fall through to BM_CLICKED, which uses same targets
      case BN_CLICKED:
         switch(target) {
            
         // case IDB_HELP:
         //    break;
            
         case IDB_CLOSE:
            PostMessageA(hwnd, WM_CLOSE, 0, 0);
            break;
         } //lint !e744  switch target
         return true;
      } //lint !e744  switch cmd
      break;
      }  //lint !e438 !e10  end local context
      
   case WM_DRAW_IMAGE:
      {
      // UpdateWindow(hwnd);  //  this does *NOT* generate WM_PAINT
      HDC hdc = GetDC(hwnd)      ;
      // OnPaint(hdc);
      Graphics    graphics(hdc);
      graphics.DrawImage(ref_image, 0, 0);
      ReleaseDC(hwnd, hdc);
      }
      break ;
      
   case WM_LOAD_LAYOUT:
      //  send message to another message queue, to do this...
      //  we are freezing our message queue 
      parse_layout_values(layout_file);
      enable_show_layout_button(true);
      enable_draw_boxes_button(true);
      enable_show_keys_button(true);
      break ;

   case WM_SHOW_LAYOUT:
      //  send message to another message queue, to do this...
      //  we are freezing our message queue 
      show_layout_info(false);
      break ;
      
   case WM_DRAW_BOX:
      draw_object_boxes(hwnd);
      break ;
      
   case WM_SHOW_KEYNUMS:
      show_key_numbers(hwnd);
      break ;

   //********************************************************************
   //  application shutdown handlers
   //********************************************************************
   case WM_CLOSE:
      DestroyWindow(hwnd);
      break;

   case WM_DESTROY:
      PostQuitMessage(0);
      break;

   // default:
   //    return false;
   }  //lint !e744  switch(iMsg) 

   return false;
}

//***********************************************************************
static DWORD WINAPI fRefImageThread(LPVOID iValue)
   {
   // hdlTopLevel = OpenProcess(PROCESS_ALL_ACCESS, false, _getpid()) ;
   HWND hwnd = CreateDialogParam(g_hinst, 
      MAKEINTRESOURCE(IDD_INPUT_IMAGE_DIALOG), NULL, (DLGPROC) RefImageProc, (LPARAM) iValue) ;
   if (hwnd == NULL) {
      syslog(_T("CreateDialog: %s\n"), get_system_message()) ;
      return 0;
   }
   // HACCEL hAccel = LoadAccelerators(g_hinst, MAKEINTRESOURCE(IDR_ACCELERATOR1));  
   // [2920] hInstance=4194304, 4194304, 4194304
   // syslog("hInstance=%u, %u, %u\n", 
   //    hInstance, 
   //    GetWindowLong(hwnd, GWL_HINSTANCE),
   //    GetModuleHandle(NULL)
   //    );

   MSG Msg;
   while(GetMessage(&Msg, NULL,0,0)) {
      // if(!TranslateAccelerator(hwnd, hAccel, &Msg)  &&  !IsDialogMessage(hwnd, &Msg)) {
      if(!IsDialogMessage(hwnd, &Msg)) {
          TranslateMessage(&Msg);
          DispatchMessage(&Msg);
      }
   }

   return (int) Msg.wParam ;
}

//****************************************************************************

static void close_ref_image_thread(LPVOID iValue)
{
   SendMessage(hwndRef, WM_CLOSE, 0, 0) ;
}  //lint !e715

//  called during WM_CLOSE in main dialog
void stop_ref_image_thread(void)
{
   if (ref_image_thread != NULL) {
      syslog(_T("delete ref image thread\n")) ;
      delete ref_image_thread ;
      ref_image_thread = NULL ;
   }
}

void open_image_window(TCHAR *image_file)
{
   if (ref_image_thread == NULL) {
      ref_image_thread = new CThread(fRefImageThread, (LPVOID) image_file, close_ref_image_thread) ;
   }
}

