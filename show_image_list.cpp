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

// temp lint declarations, development in progress
//lint -esym(843, width, height)
//  comment this line out to draw black window background 
// #define USE_SYS_BG_COLOR  1

static CThread const *image_list_thread = NULL ;
//***********************************************************************
static HWND hwndImageList = NULL ;

// static LodePng *refImage = NULL ;
// static std::vector<unsigned char> image; //the raw pixels
static unsigned width = 0, height = 0 ;
static unsigned cli_width = 0, cli_height = 0 ;
//lint -esym(844, image_list_thread)
// static TCHAR image_list_file[MAX_PATH_LEN]  = _T("") ;
// static Image *image_list = NULL;
static TCHAR const image_list_title[MAX_PATH_LEN]  = _T("Image List workspace") ;

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
   SetWindowText(hwnd, image_list_title) ;

   hwndImageList = hwnd ;
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
// #define USE_ONPAINT
#undef USE_ONPAINT

#ifdef USE_ONPAINT
static VOID OnPaint(HDC hdc)
{
   Graphics    graphics(hdc);
   graphics.DrawImage(image_list, 0, 0);
}

//***********************************************************************************
void redraw_image_list_element(void)
{
   HDC hdc = GetDC(hwndImageList);
   OnPaint(hdc);
   ReleaseDC(hwndImageList, hdc);
}
#endif

//***********************************************************************
static LRESULT CALLBACK ImageListProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
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
      // TCHAR *refImageFile = (TCHAR *) lParam ;
      // _tcscpy(image_list_file, refImageFile);
      // termout(_T("target file: %s"), image_list_file);
      // image_list = new Image(image_list_file); //lint !e1025
      // image_list = new gdi_plus(image_list_file) ;
      // width  = image_list->img_width();
      // height = image_list->img_height();
      // termout(_T("image list size: %u x %u"), width, height);
   
      do_init_dialog(hwnd) ;
      resize_window_corrected(hwnd);
      center_window(hwnd, 100, 710) ;
#ifdef USE_ONPAINT
      UpdateWindow(hwnd);
#endif      
      }
      return TRUE;

#ifdef USE_ONPAINT
   case WM_PAINT:
      {
      PAINTSTRUCT  ps;
      HDC hdc = BeginPaint(hwnd, &ps);
      OnPaint(hdc);
      EndPaint(hwnd, &ps);
      }
      return 0;
#endif      
      
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
#ifdef USE_ONPAINT
      // UpdateWindow(hwnd);  //  this does *NOT* generate WM_PAINT
      HDC hdc = GetDC(hwnd)      ;
      // OnPaint(hdc);
      Graphics    graphics(hdc);
      graphics.DrawImage(image_list, 0, 0);
      ReleaseDC(hwnd, hdc);
#endif      
      }
      break ;
      
   case WM_LOAD_LAYOUT:
      //  send message to another message queue, to do this...
      //  we are freezing our message queue 
      parse_layout_values(layout_file);
      // enable_show_layout_button(true);
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
static DWORD WINAPI fImageListThread(LPVOID iValue)
   {
   // hdlTopLevel = OpenProcess(PROCESS_ALL_ACCESS, false, _getpid()) ;
   HWND hwnd = CreateDialogParam(g_hinst, 
      MAKEINTRESOURCE(IDD_IMAGE_LIST_DIALOG), NULL, (DLGPROC) ImageListProc, (LPARAM) iValue) ;
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

static void close_image_list_thread(LPVOID iValue)
{
   SendMessage(hwndImageList, WM_CLOSE, 0, 0) ;
}  //lint !e715

//  called during WM_CLOSE in main dialog
void stop_image_list_thread(void)
{
   if (image_list_thread != NULL) {
      syslog(_T("delete image list thread\n")) ;
      delete image_list_thread ;
      image_list_thread = NULL ;
   }
}

void open_image_list_window(void)
{
   if (image_list_thread == NULL) {
      image_list_thread = new CThread(fImageListThread, (LPVOID) NULL, close_image_list_thread) ;
   }
}

