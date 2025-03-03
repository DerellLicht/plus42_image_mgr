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
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

// #include "version.h"
#include "resource.h"
#include "common.h"
#include "commonw.h"
#include "pimage_mgr.h"
// #include "statbar.h"
#include "winmsgs.h"
#include "wthread.h"
#include "lodepng.h"
#include "lode_png.h"

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

static LodePng *refImage = NULL ;
static std::vector<unsigned char> image; //the raw pixels
static unsigned width = 0, height = 0 ;
static unsigned cli_width = 0, cli_height = 0 ;
//lint -esym(844, ref_image_thread)
static TCHAR *refImageFile = NULL ;
static TCHAR ref_image_file[MAX_PATH_LEN]  = _T("") ;

//***********************************************************************
static void resize_window_corrected(HWND hwnd)
{
   //  unfortunately, this resize operation, which calls SetWindowPos(),
   // sets the *outside* window area, not the internal client area...
   resize_window(hwnd, width, height);
   
   //  so we have to calculate the error and correct it here
   RECT myRect ;
   GetClientRect(hwnd, &myRect) ;
   uint cxClient = (myRect.right - myRect.left) ;
   uint cyClient = (myRect.bottom - myRect.top) ;
   // termout("Ref Image window client area: %ux%u\n", cxClient, cyClient);
   cli_width = width + (width - cxClient) ;
   cli_height = height + (height - cyClient) ;
   // termout("Ref Image window tweaked size: %ux%u\n", cli_width, cli_height);
   
   //  now re-resize with adjusted values
   resize_window(hwnd, cli_width, cli_height);
}
      
/************************************************************************/
static void Box(HWND hwnd, int x0, int y0, int x1, int y1, COLORREF Color)
{
   HDC hdc = GetDC (hwnd) ;
   HPEN hPen = CreatePen(PS_SOLID, 1, Color) ;
   SelectObject(hdc, hPen) ;

   MoveToEx(hdc, x0, y0, NULL) ;
   LineTo  (hdc, x1, y0) ;
   LineTo  (hdc, x1, y1) ;
   LineTo  (hdc, x0, y1) ;
   LineTo  (hdc, x0, y0) ;

   SelectObject(hdc, GetStockObject(BLACK_PEN)) ;  //  deselect my pen
   DeleteObject (hPen) ;
   ReleaseDC (hwnd, hdc) ;
}

//***********************************************************************
static void do_init_dialog(HWND hwnd)
{
   SetWindowText(hwnd, ref_image_file) ;

   hwndRef = hwnd ;
   SetClassLong(hwnd, GCL_HICON,   (LONG) LoadIcon(g_hinst, (LPCTSTR)IDI_PLUS42IM));
   SetClassLong(hwnd, GCL_HICONSM, (LONG) LoadIcon(g_hinst, (LPCTSTR)IDI_PLUS42IM));
   
   resize_window_corrected(hwnd);
   //  Disable the CLOSE button on title bar;
   //  Window should be closed by command from main dialog
   EnableMenuItem(GetSystemMenu(hwnd, FALSE), SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

   center_window(hwnd, -1, 650) ;
   
   //  this does, indeed, draw the png image...
   HDC hdc = GetDC(hwnd) ;
   refImage->render_bitmap(hdc, 0, 0, 0) ;
   ReleaseDC(hwnd, hdc) ;
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
      case 295:  //  WM_CHANGEUISTATE
      case WM_NCMOUSEMOVE:
      case WM_NCHITTEST:
      case WM_SETCURSOR:
      case WM_ERASEBKGND:
      case WM_TIMER:
      case WM_NOTIFY:
      case WM_COMMAND:  //  prints its own msgs below
         break;
      default:
         syslog("TOP [%s]\n", lookup_winmsg_name(iMsg)) ;
         break;
      }
   }

   switch(iMsg) {
   case WM_INITDIALOG:
      {
      refImageFile = (TCHAR *) lParam ;
      _tcscpy(ref_image_file, refImageFile);
      unsigned error = lodepng::decode(image, width, height, refImageFile, LCT_RGB, 8);
      if (error) {
         // std::cout << "error " << error << ": " << lodepng_error_text(error) << std::endl;
         syslog("decodeWithState: [%u] %s\n", error, lodepng_error_text(error)) ;
         // syslog("decodeWithState: %u\n", error) ;
         break;
      }
      termout("Reference Image size: %ux%u\n", width, height);
      refImage = new LodePng(refImageFile);
      do_init_dialog(hwnd) ;
      }
      return TRUE;

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
         //    queryout("Terminal keyboard shortcuts") ;
         //    infoout("Alt-s = send command (i.e., print command in terminal)") ;
         //    infoout("Alt-h = show this help screen") ;
         //    infoout("Alt-c = Close this program") ;
         //    break;
            
         case IDB_CLOSE:
            PostMessageA(hwnd, WM_CLOSE, 0, 0);
            break;
         } //lint !e744  switch target
         return true;
      } //lint !e744  switch cmd
      break;
      }  //lint !e438 !e10  end local context
      
   case WM_DRAW_BOX:
      {
      draw_box_msg_t *box_data = (draw_box_msg_t *) wParam ;
      if (box_data == NULL) {
         syslog("Draw Box: pointer is NULL\n");
      }
      else {
// typedef struct draw_box_msg_s {
//    uint box_count ;
//    uint box_data->x0 ;
//    uint box_data->y0 ;
//    uint box_data->dx ;
//    uint box_data->dy ;
//    COLORREF box_data->cref ;
// } draw_box_msg_t ;
         Box(hwnd, box_data->x0,                
                   box_data->y0, 
                   box_data->x0 + box_data->dx, 
                   box_data->y0 + box_data->dy, 
                   box_data->cref);
      }
      }
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
      syslog("CreateDialog: %s\n", get_system_message()) ;
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
      syslog("delete ref image thread\n") ;
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

