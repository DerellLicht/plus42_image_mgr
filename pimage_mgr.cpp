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
#include <objidl.h>
#include <gdiplus.h>
#include <htmlhelp.h>

using namespace Gdiplus;

#include "version.h"
#include "resource.h"
#include "common.h"
#include "commonw.h"
#include "pimage_mgr.h"
#include "cterminal.h"  // MAX_TERM_CHARS
#include "terminal.h"
#include "winmsgs.h"

static TCHAR const * const Version = _T("Plus42 Image Manager, Version " VerNum " ") ;
//lint -esym(715, lParam)
//lint -esym(818, szCmdLine, hPrevInstance)  could be declared as pointing to const

// static char szAppName[] = "pimage_mgr";

//***********************************************************************
HINSTANCE g_hinst = 0;

static HWND hwndMain          = NULL ;
static HWND hwndSelectSkin    = NULL ;
static HWND hwndLoadLayout    = NULL ;
static HWND hwndShowLayout    = NULL ;
static HWND hwndDrawBoxes     = NULL ;
static HWND hwndShowKeys      = NULL ;
static HWND hwndCounter       = NULL ;
static HWND hwndShowImageList = NULL ;

//lint -esym(843, dbg_flags)  could be declared as const
uint dbg_flags = 0
   // | DBG_WINMSGS
   ;

// static HWND hToolTip ;  /* Tooltip handle */

TCHAR layout_file[MAX_PATH_LEN] = _T("skin.layout") ;
static TCHAR skin_name[MAX_PATH_LEN]   = _T("") ;
static TCHAR image_file[MAX_PATH_LEN]  = _T("") ;

//*************************************************************
#ifndef PATH_MAX
#define  PATH_MAX    1024
#endif

static TCHAR chmname[PATH_MAX] = _T("");

static void find_chm_location(void)
{
   LRESULT result = derive_filename_from_exec(chmname, (TCHAR *) _T(".chm")) ; //lint !e1773 const
   if (result != 0) {
      put_color_term_msg(TERM_ERROR, _T("find_chm_loc: %s\n"), get_system_message());
   }
   else {
      put_color_term_msg(TERM_INFO, _T("%s"), chmname);
   }
}

//*************************************************************
static void view_help_screen(HWND hwnd)
{
   find_chm_location() ;
   
   // syslog("help=[%s]", chmname) ;
   //  MinGw gives a couple of indecipherable linker warnings about this:
   // Warning: .drectve `-defaultlib:uuid.lib ' unrecognized
   // Warning: .drectve `-defaultlib:uuid.lib ' unrecognized   
   //  But ignoring them doesn't seem to hurt anything...
   HtmlHelp(hwnd, chmname, HH_DISPLAY_TOPIC, 0L);
   return ;
}

//***********************************************************************
// LodePng pngSprites("tiles32.png", SPRITE_HEIGHT, SPRITE_WIDTH) ;
// LodePng pngTiles  ("images.png",  IMAGE_WIDTH,   IMAGE_HEIGHT) ;

//***********************************************************************
//lint -esym(714, enable_load_layout_button)
//lint -esym(759, enable_load_layout_button)
//lint -esym(765, enable_load_layout_button)
void enable_load_layout_button(bool state)
{
   EnableWindow(hwndLoadLayout, state);
}

//***********************************************************************
//lint -esym(714, enable_show_layout_button)
//lint -esym(759, enable_show_layout_button)
//lint -esym(765, enable_show_layout_button)
void enable_show_layout_button(bool state)
{
   EnableWindow(hwndShowLayout, state);
}

//***********************************************************************
void enable_draw_boxes_button(bool state)
{
   EnableWindow(hwndDrawBoxes, state);
}

//***********************************************************************
void enable_show_keys_button(bool state)
{
   EnableWindow(hwndShowKeys, state);
}

//***********************************************************************
void enable_show_image_list(bool state)
{
   EnableWindow(hwndShowImageList, state);
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
//  args for center_window:
//  x=-1  center width in window
//  y=-1  center height in window
//  y > 0  set Y position to y
//***********************************************************************
//lint -esym(714, center_window)
//lint -esym(759, center_window)
//lint -esym(765, center_window)
//lint -esym(578, y0)
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
      syslog(_T("set_term_attr: invalid index %u\n"), atidx) ;
      return ;
   }
   term_set_attr(term_atable[atidx].fgnd, term_atable[atidx].bgnd) ;
}

//********************************************************************
//  This outputs to terminal in default colors
//********************************************************************
int termout(const TCHAR *fmt, ...)
{
   TCHAR consoleBuffer[MAX_TERM_CHARS + 1];
   va_list al; //lint !e522

   va_start(al, fmt);   //lint !e1055 !e530
   _vstprintf(consoleBuffer, fmt, al);   //lint !e64
   set_term_attr(TERM_NORMAL);
   term_put(consoleBuffer);
   va_end(al);
   return 1;
}

//********************************************************************
//  this *cannot* be called with a color attribute;
//  it must be called with an index into term_atable[] !!
//********************************************************************
int put_color_term_msg(uint idx, const TCHAR *fmt, ...)
{
   TCHAR consoleBuffer[MAX_TERM_CHARS + 1];
   va_list al; //lint !e522

   va_start(al, fmt);   //lint !e1055 !e530
   _vstprintf(consoleBuffer, fmt, al);   //lint !e64
   set_term_attr(idx) ;
   term_put(consoleBuffer);
   va_end(al);
   return 1;
}

//***********************************************************************
static void do_init_dialog(HWND hwnd)
{
   TCHAR msgstr[81] ;
   wsprintf(msgstr, _T("%s"), Version) ;
   SetWindowText(hwnd, msgstr) ;

   SetClassLong(hwnd, GCL_HICON,   (LONG) LoadIcon(g_hinst, (LPCTSTR)IDI_PLUS42IM));
   SetClassLong(hwnd, GCL_HICONSM, (LONG) LoadIcon(g_hinst, (LPCTSTR)IDI_PLUS42IM));

   hwndMain = hwnd ;
   hwndSelectSkin    = GetDlgItem(hwnd, IDB_SKIN_SELECT);
   hwndDrawBoxes     = GetDlgItem(hwnd, IDB_DRAW_BOXES);
   hwndShowKeys      = GetDlgItem(hwnd, IDB_SHOW_KEYS);
   hwndLoadLayout    = GetDlgItem(hwnd, IDB_LOAD_LAYOUT);
   hwndShowLayout    = GetDlgItem(hwnd, IDB_SHOW_LAYOUT);
   hwndCounter       = GetDlgItem(hwnd, IDC_COUNTER);
   hwndShowImageList = GetDlgItem(hwnd, IDB_OPEN_ILIST);
   
   // EnableWindow(hwndOpen, false);
   EnableWindow(hwndLoadLayout, false);
   EnableWindow(hwndShowLayout, false);
   EnableWindow(hwndDrawBoxes,  false);
   EnableWindow(hwndShowKeys,   false);
   EnableWindow(hwndShowImageList,   false);

   // setup_main_menu(hwnd) ;
   // set_up_working_spaces(hwnd) ; //  do this *before* tooltips !!
   //***************************************************************************
   //  add tooltips and bitmaps
   //***************************************************************************
   // create_and_add_tooltips(hwnd, 150, 100, 10000, main_tooltips);

   //****************************************************************
   //  create/configure terminal
   //****************************************************************
   center_window(hwnd, -1, 100) ;
   setup_terminal_window(hwnd, 0, IDB_SKIN_SELECT, IDC_TERMINAL);
   set_local_terminal_colors() ;
   put_color_term_msg(TERM_INFO, _T("terminal size: columns=%u, rows=%u"),
      term_get_columns(), term_get_rows());
}

//***********************************************************************
//lint -esym(714, update_counter_field)
//lint -esym(759, update_counter_field)
//lint -esym(765, update_counter_field)
void update_counter_field(uint counter)
{
   TCHAR msgstr[41] ;
   _stprintf(msgstr, _T(" %u"), counter) ;
   SetWindowText(hwndCounter, msgstr) ;
}

//***********************************************************************
//lint -esym(551, draw_msg)
static LRESULT CALLBACK WinProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
   TCHAR msgstr[81] ;
   
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
         syslog(_T("TOP [%s]\n"), lookup_winmsg_name(iMsg)) ;
         break;
      }
   }

   switch(iMsg) {
   case WM_INITDIALOG:
      do_init_dialog(hwnd) ;
      return TRUE;

   case WM_NOTIFY:
      return term_notify(hwnd, lParam) ;

   case WM_COMMAND:
      {  //  create local context
      DWORD cmd = HIWORD (wParam) ;
      DWORD target = LOWORD(wParam) ;

      switch (cmd) {
      case FVIRTKEY:  //  keyboard accelerators: WARNING: same code as CBN_SELCHANGE !!
         //  fall through to BM_CLICKED, which uses same targets
      case BN_CLICKED:
         switch(target) {
         
         //  "Start Here" button         
         case IDB_SKIN_SELECT:
            {  // create local context
            _tcscpy(layout_file, _T("skin.layout")) ;
            if (select_file(hwnd, layout_file, _T("layout"))) {
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
               _stprintf(msgstr, _T(" %s"), skin_name) ;
               SetWindowText(GetDlgItem(hwnd, IDC_SKIN_NAME), msgstr) ;
               
               //  generate image filename
               _tcscpy(image_file, layout_file);
               p = _tcschr(image_file, _T('.'));
               if (p == NULL) {
                  goto error_path;
               }
               _tcscpy(p, _T(".gif"));
               termout(_T("%s"), image_file);
               
               EnableWindow(hwndSelectSkin, false);
               EnableWindow(hwndLoadLayout, true);
               open_image_window(image_file);
            } 
            else {
error_path:
               layout_file[0] = 0 ; //  make layout filename invalid
               DWORD glerr = GetLastError();
               if (glerr == 0) {
                  termout(_T("select_file: aborted by user")) ;
               }
               else {
                  put_color_term_msg(TERM_ERROR, _T("select_file: %s"), get_system_message()) ;
               }
            }
            }  // end local context
            return 0 ;
            
         case IDB_LOAD_LAYOUT:
            PostMessage(hwndRef, WM_LOAD_LAYOUT, (WPARAM) NULL, (LPARAM) NULL);
            break ;
         
         case IDB_SHOW_LAYOUT:
            PostMessage(hwndRef, WM_SHOW_LAYOUT, (WPARAM) NULL, (LPARAM) NULL);
            break ;
         
         case IDB_DRAW_BOXES:
            // Key: 2 117,450,102,106 127,478,82,58 1389,478
            PostMessage(hwndRef, WM_DRAW_BOX, (WPARAM) NULL, (LPARAM) NULL);
            break ;

         case IDB_SHOW_KEYS:
            // Key: 2 117,450,102,106 127,478,82,58 1389,478
            PostMessage(hwndRef, WM_SHOW_KEYNUMS, (WPARAM) NULL, (LPARAM) NULL);
            break ;

         case IDB_OPEN_ILIST:
            open_image_list_window();
            break ;

         case IDB_HELP:
            // put_color_term_msg(TERM_INFO, _T("Help file is not yet created"));
            view_help_screen(hwnd); 
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
      stop_image_list_thread();
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

   GdiplusStartupInput gdiplusStartupInput;
   ULONG_PTR           gdiplusToken;
   
   load_exec_filename() ;  //  get our executable name
   // set_ini_filename();
   
   // Initialize GDI+.
   GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
   
   // hdlTopLevel = OpenProcess(PROCESS_ALL_ACCESS, false, _getpid()) ;
   HWND hwnd = CreateDialog(g_hinst, MAKEINTRESOURCE(IDD_MAIN_DIALOG), NULL, (DLGPROC) WinProc) ;
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
   GdiplusShutdown(gdiplusToken);

   return (int) Msg.wParam ;
}  //lint !e715

