//****************************************************************************
//  Copyright (c) 2025  Daniel D Miller
//  pimage_mgr - Plus42 Image Manager
//
//  Written by:  Dan Miller
//****************************************************************************

//lint -esym(767, _WIN32_WINNT)
#define  _WIN32_WINNT   0x0501
#include <windows.h>
#include <stdio.h>   //  vsprintf, sprintf, which supports %f
#include <tchar.h>

#include "version.h"
#include "resource.h"
#include "common.h"
#include "commonw.h"
#include "pimage_mgr.h"
#include "cterminal.h"  // MAX_TERM_CHARS
#include "terminal.h"
#include "winmsgs.h"

static TCHAR const * const Version = "Plus42 Image Manager, Version " VerNum " " ;
//lint -esym(715, lParam)
//lint -esym(818, szCmdLine, hPrevInstance)  could be declared as pointing to const

// static char szAppName[] = "pimage_mgr";

//  taken from CTerminal.h
//  if a function needs a longer line than this, they should
//  define their own termout() derivative function(s)
// #define  MAX_TERM_CHARS    1024

//***********************************************************************
HINSTANCE g_hinst = 0;

static HWND hwndMain = NULL ;
static HWND hwndOpen = NULL ;
// static HMENU hMainMenu = NULL ;

//lint -esym(843, dbg_flags)  could be declared as const
uint dbg_flags = 0
   // | DBG_WINMSGS
   ;

// static HWND hToolTip ;  /* Tooltip handle */

static TCHAR layout_file[MAX_PATH_LEN] = _T("skin.layout") ;
static TCHAR skin_name[MAX_PATH_LEN]   = _T("") ;
static TCHAR image_file[MAX_PATH_LEN]  = _T("") ;

//  external functions

//  show_ref_image.cpp
extern void stop_ref_image_thread(void);

//***********************************************************************
// LodePng pngSprites("tiles32.png", SPRITE_HEIGHT, SPRITE_WIDTH) ;
// LodePng pngTiles  ("images.png",  IMAGE_WIDTH,   IMAGE_HEIGHT) ;

//*******************************************************************
// void status_message(char *msgstr)
// {
//    MainStatusBar->show_message(msgstr);
// }

//*******************************************************************
// void status_message(uint idx, char *msgstr)
// {
//    MainStatusBar->show_message(idx, msgstr);
// }

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
//  args for center_window:
//  x=-1  center width in window
//  y=-1  center height in window
//  y > 0  set Y position to y
//***********************************************************************
//lint -esym(714, center_window)
//lint -esym(759, center_window)
//lint -esym(765, center_window)
void center_window(HWND hwnd, int x_pos, int y_pos)
{
   if (screen_width == 0) {
      ww_get_monitor_dimens(hwndMain);
   }
   
   RECT myRect ;
   GetWindowRect(hwnd, &myRect) ;
   // GetClientRect(hwnd, &myRect) ;
   uint dialog_width = (myRect.right - myRect.left) ;
   uint dialog_height = (myRect.bottom - myRect.top) ;

   uint x0 = (screen_width  - dialog_width ) / 2 ;
   uint y0 = (screen_height - dialog_height) / 2 ;
   
   if (x_pos >= 0) {
      x0 = x_pos ;
   }
   if (y_pos >= 0) {
      y0 = y_pos ;
   }
   SetWindowPos(hwnd, HWND_TOP, x0, y0, 0, 0, SWP_NOSIZE) ;
}

//****************************************************************************
//lint -esym(756, attrib_table_t)
typedef struct attrib_table_s {
   COLORREF fgnd ;
   COLORREF bgnd ;
} attrib_table_t ;

static attrib_table_t term_atable[NUM_TERM_ATTR_ENTRIES] = {
   { WIN_CYAN,    WIN_BLACK },   // TERM_NORMAL 
   { WIN_BCYAN,   WIN_GREY },    // TERM_INFO
   { WIN_YELLOW,  WIN_BLUE },    // TERM_QUERY
   { WIN_BRED,    WIN_BLUE }     // TERM_ERROR
} ;

//****************************************************************************
static void set_local_terminal_colors(void)
{
   COLORREF std_bgnd = GetSysColor(COLOR_WINDOW) ;
   term_atable[TERM_NORMAL].fgnd = GetSysColor(COLOR_WINDOWTEXT) ;
   term_atable[TERM_NORMAL].bgnd = std_bgnd ;
}

//********************************************************************
//lint -esym(714, set_term_attr, termout, put_color_term_msg)
//lint -esym(759, set_term_attr, termout, put_color_term_msg)
//lint -esym(765, set_term_attr, termout, put_color_term_msg)
void set_term_attr(uint atidx)
{
   if (atidx >= NUM_TERM_ATTR_ENTRIES) {
      syslog("set_term_attr: invalid index %u\n", atidx) ;
      return ;
   }
   term_set_attr(term_atable[atidx].fgnd, term_atable[atidx].bgnd) ;
}

//********************************************************************
//  This outputs to terminal in default colors
//********************************************************************
int termout(const char *fmt, ...)
{
   char consoleBuffer[MAX_TERM_CHARS + 1];
   va_list al; //lint !e522

   va_start(al, fmt);   //lint !e1055 !e530
   vsprintf(consoleBuffer, fmt, al);   //lint !e64
   set_term_attr(TERM_NORMAL);
   term_put(consoleBuffer);
   va_end(al);
   return 1;
}

//********************************************************************
//  this *cannot* be called with a color attribute;
//  it must be called with an index into term_atable[] !!
//********************************************************************
int put_color_term_msg(uint idx, const char *fmt, ...)
{
   char consoleBuffer[MAX_TERM_CHARS + 1];
   va_list al; //lint !e522

   va_start(al, fmt);   //lint !e1055 !e530
   vsprintf(consoleBuffer, fmt, al);   //lint !e64
   set_term_attr(idx) ;
   term_put(consoleBuffer);
   va_end(al);
   return 1;
}

//***********************************************************************
static void do_init_dialog(HWND hwnd)
{
   // hwndTopLevel = hwnd ;   //  do I need this?
   char msgstr[81] ;
   wsprintfA(msgstr, "%s", Version) ;
   SetWindowText(hwnd, msgstr) ;

   SetClassLong(hwnd, GCL_HICON,   (LONG) LoadIcon(g_hinst, (LPCTSTR)IDI_PLUS42IM));
   SetClassLong(hwnd, GCL_HICONSM, (LONG) LoadIcon(g_hinst, (LPCTSTR)IDI_PLUS42IM));

   hwndMain = hwnd ;
   hwndOpen = GetDlgItem(hwnd, IDB_SKIN_OPEN);
   
   EnableWindow(hwndOpen, false);

   // setup_main_menu(hwnd) ;
   // set_up_working_spaces(hwnd) ; //  do this *before* tooltips !!
   //***************************************************************************
   //  add tooltips and bitmaps
   //***************************************************************************
   // create_and_add_tooltips(hwnd, 150, 100, 10000, main_tooltips);

   // RECT myRect ;
   // // GetWindowRect(hwnd, &myRect) ;
   // GetClientRect(hwnd, &myRect) ;
   // uint cxClient = (myRect.right - myRect.left) ;
   // uint cyClient = (myRect.bottom - myRect.top) ;

   center_window(hwnd, -1, 100) ;
   //****************************************************************
   //  create/configure status bar
   //****************************************************************
   // MainStatusBar = new CStatusBar(hwnd) ;
   // MainStatusBar->MoveToBottom(cxClient, cyClient) ;
   // //  re-position status-bar parts
   // {
   // int sbparts[3];
   // sbparts[0] = (int) (6 * cxClient / 10) ;
   // sbparts[1] = (int) (8 * cxClient / 10) ;
   // sbparts[2] = -1;
   // MainStatusBar->SetParts(3, &sbparts[0]);
   // }

   //****************************************************************
   //  create/configure terminal
   //****************************************************************
   // setup_terminal_window(hwnd, MainStatusBar->height(), IDB_SKIN_OPEN);
   setup_terminal_window(hwnd, 0, IDB_SKIN_OPEN, IDC_TERMINAL);
   set_local_terminal_colors() ;
   put_color_term_msg(TERM_INFO, "terminal size: columns=%u, rows=%u",
      term_get_columns(), term_get_rows());
      
   // put_color_term_msg(TERM_ERROR, "Testing ERROR report function");
}

//***********************************************************************
static LRESULT CALLBACK WinProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
   char msgstr[81] ;
   
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
      return TRUE;

   case WM_NOTIFY:
      return term_notify(hwnd, lParam) ;

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
   // case WM_ERASEBKGND:
   //    // syslog("WM_ERASEBKGND\n") ;
   //    break;

   case WM_COMMAND:
      {  //  create local context
      DWORD cmd = HIWORD (wParam) ;
      DWORD target = LOWORD(wParam) ;

      switch (cmd) {
      case FVIRTKEY:  //  keyboard accelerators: WARNING: same code as CBN_SELCHANGE !!
         //  fall through to BM_CLICKED, which uses same targets
      case BN_CLICKED:
         switch(target) {
         
         case IDB_SKIN_SELECT:
            {  // create local context
            _tcscpy(layout_file, _T("skin.layout")) ;
            if (select_file(hwnd, layout_file, "layout")) {
               TCHAR *p = _tcsrchr(layout_file, _T('\\'));
               if (p == NULL) {
                  goto error_path;
               }
               p++ ;
               _tcscpy(skin_name, p);
               p = _tcschr(skin_name, _T('.'));
               if (p == NULL) {
                  goto error_path;
               }
               *p = 0 ;
               sprintf(msgstr, " %s", skin_name) ;
               SetWindowText(GetDlgItem(hwnd, IDC_SKIN_NAME), msgstr) ;
               
               //  generate image filename
               _tcscpy(image_file, layout_file);
               p = _tcschr(image_file, _T('.'));
               if (p == NULL) {
                  goto error_path;
               }
               strcpy(p, _T(".png"));
               termout("%s", image_file);
               
               EnableWindow(hwndOpen, true);
            } else {
error_path:
               layout_file[0] = 0 ; //  make layout filename invalid
               termout("select_file: %s", get_system_message()) ;
            }
            }  // end local context
            return 0 ;
            
         case IDB_SKIN_OPEN:
            open_image_window(image_file);
            break ;
            
         case IDB_HELP:
            break;
            
         case IDD_ABOUT : 
            CmdAbout(hwnd); 
            break ;
         
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
      stop_ref_image_thread();
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
   HWND hwnd = CreateDialog(g_hinst, MAKEINTRESOURCE(IDD_MAIN_DIALOG), NULL, (DLGPROC) WinProc) ;
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

