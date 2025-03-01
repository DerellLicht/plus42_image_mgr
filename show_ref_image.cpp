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
// #include "commonw.h"
#include "pimage_mgr.h"
#include "statbar.h"
#include "winmsgs.h"
#include "wthread.h"
#include "lodepng.h"
#include "lode_png.h"

//lint -esym(551, hwndRef, refImage)
//lint -esym(844, hwndRef, refImage)

//  comment this line out to draw black window background 
// #define USE_SYS_BG_COLOR  1

static CThread *ref_image_thread = NULL ;
//***********************************************************************
// static int cxClient = 0 ;
// static int cyClient = 0 ;

// unsigned xbase, xdiff, ybase, ydiff ;

static HWND hwndRef = NULL ;

// static TCHAR tempstr[128] ;

static CStatusBar *MainStatusBar = NULL;

static LodePng *refImage = NULL ;
static std::vector<unsigned char> image; //the raw pixels
static unsigned width = 0, height = 0 ;
//lint -esym(844, ref_image_thread)
static TCHAR *refImageFile = NULL ;

//***********************************************************************
static void do_init_dialog(HWND hwnd)
{
   TCHAR msgstr[81] ;
   // hwndTopLevel = hwnd ;   //  do I need this?
   _stprintf(msgstr, "%s", Version) ;
   SetWindowText(hwnd, msgstr) ;

   SetClassLong(hwnd, GCL_HICON,   (LONG) LoadIcon(g_hinst, (LPCTSTR)IDI_PLUS42IM));
   SetClassLong(hwnd, GCL_HICONSM, (LONG) LoadIcon(g_hinst, (LPCTSTR)IDI_PLUS42IM));

   hwndRef = hwnd ;

   // set_up_working_spaces(hwnd) ; //  do this *before* tooltips !!
   //***************************************************************************
   //  add tooltips and bitmaps
   //***************************************************************************
   // create_and_add_tooltips(hwnd, 150, 100, 10000, main_tooltips);

   // RECT rWindow;
   // unsigned stTop ;
   RECT myRect ;
   // GetWindowRect(hwnd, &myRect) ;
   GetClientRect(hwnd, &myRect) ;
   int cxClient = (myRect.right - myRect.left) ;
   int cyClient = (myRect.bottom - myRect.top) ;

   // center_window() ;
   //****************************************************************
   //  create/configure status bar
   //****************************************************************
   MainStatusBar = new CStatusBar(hwnd) ;
   MainStatusBar->MoveToBottom(cxClient, cyClient) ;
   //  re-position status-bar parts
   {
   int sbparts[3];
   sbparts[0] = (int) (6 * cxClient / 10) ;
   sbparts[1] = (int) (8 * cxClient / 10) ;
   sbparts[2] = -1;
   MainStatusBar->SetParts(3, &sbparts[0]);
   }
   
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
      unsigned error = lodepng::decode(image, width, height, refImageFile, LCT_RGB, 8);
      if (error) {
         // std::cout << "error " << error << ": " << lodepng_error_text(error) << std::endl;
         syslog("decodeWithState: [%u] %s\n", error, lodepng_error_text(error)) ;
         // syslog("decodeWithState: %u\n", error) ;
         break;
      }
      syslog("refImage size: %ux%u\n", width, height);
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
void open_image_window(TCHAR *image_file)
{
   if (ref_image_thread == NULL) {
      ref_image_thread = new CThread(fRefImageThread, (LPVOID) image_file, NULL) ;
   }
}

