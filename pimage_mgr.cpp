//****************************************************************************
//  Copyright (c) 2025  Daniel D Miller
//  pimage_mgr - Plus42 Image Manager
//
//  Written by:  Dan Miller
//****************************************************************************

#include "version.h"
static char const * const Version = "Plus42 Image Manager, Version " VerNum " " ;

//lint -esym(767, _WIN32_WINNT)
#define  _WIN32_WINNT   0x0501
#include <windows.h>
#include <stdio.h>   //  vsprintf, sprintf, which supports %f

#include "resource.h"
#include "common.h"
// #include "commonw.h"
#include "header.h"
#include "statbar.h"
#include "winmsgs.h"
#include "lode_png.h"

//lint -esym(715, lParam)
//lint -esym(818, szCmdLine, hPrevInstance)  could be declared as pointing to const

// static char szAppName[] = "pimage_mgr";

//lint -esym(714, dbg_flags)
//lint -esym(759, dbg_flags)
//lint -esym(765, dbg_flags)

//***********************************************************************
HINSTANCE g_hinst = 0;

static HWND hwndMain ;
static HMENU hMainMenu = NULL ;

//lint -esym(843, dbg_flags)  could be declared as const

//lint -esym(714, status_message)
//lint -esym(765, status_message)
static uint dbg_flags = 0
   // | DBG_WINMSGS
   ;

static uint cxClient = 0 ;
static uint cyClient = 0 ;

static CStatusBar *MainStatusBar = NULL;
// static HWND hToolTip ;  /* Tooltip handle */

//***********************************************************************
// LodePng pngSprites("tiles32.png", SPRITE_HEIGHT, SPRITE_WIDTH) ;
// LodePng pngTiles  ("images.png",  IMAGE_WIDTH,   IMAGE_HEIGHT) ;

//*******************************************************************
void status_message(char *msgstr)
{
   MainStatusBar->show_message(msgstr);
}

//*******************************************************************
void status_message(uint idx, char *msgstr)
{
   MainStatusBar->show_message(idx, msgstr);
}

//***********************************************************************
static uint screen_width  = 0 ;
static uint screen_height = 0 ;

static void ww_get_monitor_dimens(HWND hwnd)
{
   HMONITOR currentMonitor;      // Handle to monitor where fullscreen should go
   MONITORINFO mi;               // Info of that monitor
   currentMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
   mi.cbSize = sizeof(MONITORINFO);
   if (GetMonitorInfo(currentMonitor, &mi) != FALSE) {
      screen_width  = mi.rcMonitor.right  - mi.rcMonitor.left ;
      screen_height = mi.rcMonitor.bottom - mi.rcMonitor.top ;
   }
   // curr_dpi = GetScreenDPI() ;
}

//***********************************************************************
static void center_window(void)
{
   ww_get_monitor_dimens(hwndMain);
   
   RECT myRect ;
   GetWindowRect(hwndMain, &myRect) ;
   // GetClientRect(hwnd, &myRect) ;
   uint dialog_width = (myRect.right - myRect.left) ;
   uint dialog_height = (myRect.bottom - myRect.top) ;

   uint x0 = (screen_width  - dialog_width ) / 2 ;
   uint y0 = (screen_height - dialog_height) / 2 ;

   SetWindowPos(hwndMain, HWND_TOP, x0, y0, 0, 0, SWP_NOSIZE) ;
}

//***********************************************************************
static void setup_main_menu(HWND hwnd)
{
   hMainMenu = LoadMenu(g_hinst, MAKEINTRESOURCE(IDM_MAINMENU));
   SetMenu(hwnd, hMainMenu);
   // update_summary_options_menu() ;   //  initial setup
}

//***********************************************************************
static void do_init_dialog(HWND hwnd)
{
   char msgstr[81] ;
   // hwndTopLevel = hwnd ;   //  do I need this?
   wsprintfA(msgstr, "%s", Version) ;
   SetWindowTextA(hwnd, msgstr) ;

   SetClassLongA(hwnd, GCL_HICON,   (LONG) LoadIcon(g_hinst, (LPCTSTR)IDI_PLUS42IM));
   SetClassLongA(hwnd, GCL_HICONSM, (LONG) LoadIcon(g_hinst, (LPCTSTR)IDI_PLUS42IM));

   hwndMain = hwnd ;

   setup_main_menu(hwnd) ;
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
   cxClient = (myRect.right - myRect.left) ;
   cyClient = (myRect.bottom - myRect.top) ;

   center_window() ;
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
static LRESULT CALLBACK TermProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
   //***************************************************
   //  debug: log all windows messages
   //***************************************************
   if (dbg_flags & 1) {
      switch (iMsg) {
      //  list messages to be ignored
      case WM_CTLCOLORBTN:
      case WM_CTLCOLORSTATIC:
      case WM_CTLCOLOREDIT:
      case WM_CTLCOLORDLG:
      case WM_MOUSEMOVE:
      case 295:  //  WM_CHANGEUISTATE
      case WM_NCMOUSEMOVE:
      case WM_NCMOUSELEAVE:
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
      do_init_dialog(hwnd) ;
      // wpOrigMainProc = (WNDPROC) SetWindowLongPtr(hwnd, GWL_WNDPROC, (LONG) MainSubclassProc); 
      return TRUE;

   //***********************************************************************************************
   //  04/16/14 - unfortunately, I cannot use WM_SIZE, nor any other message, to draw my graphics,
   //  because some other message occurs later and over-writes my work...
   //***********************************************************************************************
   // case WM_SIZE:
   //    if (wParam == SIZE_RESTORED) {
   //       // syslog("WM_SIZE\n") ;
   //       redraw_in_progress = true ;
   //    } 
      //********************************************************************************************
      //  The last operations in the dialog redraw, are subclassed WM_CTLCOLORSTATIC messages.
      //  So, to determine when it is all done, I need to somehow recognize when these are done,
      //  and then update our graphics objects.
      //********************************************************************************************
   //    return TRUE;

   //  this occurs during program startup
   case WM_ERASEBKGND:
      // syslog("WM_ERASEBKGND\n") ;
      break;

   case WM_COMMAND:
      {  //  create local context
      DWORD cmd = HIWORD (wParam) ;
      DWORD target = LOWORD(wParam) ;

      switch (cmd) {
      case FVIRTKEY:  //  keyboard accelerators: WARNING: same code as CBN_SELCHANGE !!
         //  fall through to BM_CLICKED, which uses same targets
      case BN_CLICKED:
         switch(target) {
         
         case IDB_OPTIONS:
            break ;
            
         case IDM_HELP:
            break;
            
         case IDD_ABOUT : CmdAbout(hwnd); break ;
         
         case IDM_CLOSE:
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
//lint -esym(1784, WinMain)
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
   {
   g_hinst = hInstance;

   // hdlTopLevel = OpenProcess(PROCESS_ALL_ACCESS, false, _getpid()) ;
   HWND hwnd = CreateDialog(g_hinst, MAKEINTRESOURCE(IDD_MAIN_DIALOG), NULL, (DLGPROC) TermProc) ;
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
}  //lint !e715

