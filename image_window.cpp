 //****************************************************************************
//  Produced and Directed by:  Dan Miller
//****************************************************************************
#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include "resource.h"
#include "common.h"
#include "commonw.h"
#include "pimage_mgr.h"
#include "winmsgs.h"

static TCHAR szEditName[] = _T("SendBfr") ;
// static char szEditName[] = "EditPad" ;

static LRESULT CALLBACK SendFileProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

CThread *ref_image_thread = NULL ;
//****************************************************************************
//  this should be done only once, at program startup
//****************************************************************************
LRESULT register_edit_window(void)
{
   // WNDCLASS wndclass = {0} ;
   WNDCLASS wndclass ;
   memset((char *) &wndclass, 0, sizeof(WNDCLASS)) ;

   wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
   wndclass.lpfnWndProc   = SendFileProc ;
   wndclass.cbClsExtra    = 0 ;
   wndclass.cbWndExtra    = 0 ;
   wndclass.hInstance     = g_hinst ;
   wndclass.hIcon         = LoadIcon (g_hinst, MAKEINTRESOURCE(IDI_ANAICON));
   wndclass.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1) ;
   wndclass.lpszMenuName  = MAKEINTRESOURCE(IDM_POPPAD2) ;
   wndclass.lpszClassName = szEditName ;

   if (!RegisterClass(&wndclass)) {
      LRESULT result = (LRESULT) GetLastError() ;
      syslog("RegisterClass: %s", get_system_message(result)) ;
      return result ;
   }
   return 0;
}

//******************************************************************
//  read lines from cmd_fname and stuff all resulting data
//  into the edit window.
//******************************************************************
static int read_command_file(monitor_object_p this_device, char *cmd_fname)
{
   char inpbfr[MAX_RESP_BFR+1] ;
   // HWND hwndEdit = GetDlgItem(hDlgWnd, IDC_EDIT_CE) ;
   FILE *fd = fopen(cmd_fname, "rt") ;
   if (fd == 0)
      return (int) GetLastError() ;

   unsigned lcount = 0 ;
   while (fgets(inpbfr, MAX_RESP_BFR, fd) != 0) {
      lcount++ ;
      //  convert whatever newlines are found, into CR/LF
      int found = 0 ;
      char *hd = inpbfr ;
      while (*hd != 0) {
         if (*hd == CR  ||  *hd == LF) {
            *hd++ = CR ;
            *hd++ = LF ;
            *hd = 0 ;
            found = 1 ;
            break;
         }
         hd++ ;
      }
      if (!found) {
         syslog("line %u: no EOL found, discarding data\n", lcount) ;
         continue;
      }

      //  stuff text into edit buffer
      int ndx = GetWindowTextLengthA(this_device->hwndEdit);
      SendMessageA(this_device->hwndEdit, EM_SETSEL, (WPARAM) ndx, (LPARAM) ndx) ;
      SendMessageA(this_device->hwndEdit, EM_REPLACESEL, 0, (LPARAM) ((LPSTR) inpbfr));
   }
   fclose(fd) ;
   return 0;
}

//******************************************************************
//  We can no longer copy lines into input_bfr[],
//  because send_anacommand() is going to do that.
//  We will have to buffer individual lines internally!!
//******************************************************************
static void send_commands(monitor_object_p this_device)
{
   char sbfr[1024] ;
   uint sidx = 0 ;
   //****************************************************
   //  extract command list from editor window
   //****************************************************
   int ndx = GetWindowTextLengthA (this_device->hwndEdit);
   char *bfr = (char *) GlobalAlloc(GMEM_FIXED, ndx+1) ;
   if (bfr == NULL) {
      syslog("GlobalAlloc: %s\n", get_system_message()) ;
      return ;
   }
   unsigned blen = GetWindowTextA (this_device->hwndEdit, bfr, ndx+1);
   // bfr[ndx+1] = 0 ;
   bfr[blen] = 0 ;
   // syslog("read %u of %u bytes\n", blen, ndx) ;
   // hex_dump((u8 *) bfr, ndx) ;

   //****************************************************
   //  write commands to ODU
   //****************************************************
   char rbfr[MAX_RESP_BFR] ;
   int result ;
   // reset_input_bfr(this_device) ;
   sbfr[0] = 0 ;
   unsigned lcount = 0 ;
   char *src = bfr ;
   begin_critical_section();
   while (LOOP_FOREVER) {
      if (*src == 0) {
         //  send any unterminated last line that we find
         if (sidx > 0) {
            txout(this_device, sbfr);
            result = send_anacommand(this_device, sbfr, rbfr) ;
            if (result < 0) {
               errout(this_device, "%s: %s", sbfr, show_error(result));
               break;
            }
            rxout(this_device, rbfr);
            lcount++ ;
         }
         break;
      } else
      if (*src == CR) {
         src++ ;   //  copy CR
         src++ ;   //  copy LF
         txout(this_device, sbfr);
         result = send_anacommand(this_device, sbfr, rbfr) ;
         if (result < 0) {
            errout(this_device, "%s: %s", sbfr, show_error(result));
            break;
         }
         rxout(this_device, rbfr);
         lcount++ ;
         sidx = 0 ;
         sbfr[sidx] = 0 ;
      } else
      {
         sbfr[sidx++] = *src++ ;
         sbfr[sidx] = 0 ;
      }
   }
   leave_serial_critical_section();

   GlobalFree(bfr) ;
   // this_device->input_bfr[0] = 0 ;
   // this_device->input_bfr_len = 0 ;
   termout(this_device, "Sent %u lines to ODU", lcount) ;
}

//****************************************************************************
static LRESULT CALLBACK SendFileProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   // int iSelect, iEnable ;
   int result ;
   // char command_filename[MAX_PATH] = "commands.txt" ;

   monitor_object_p this_device ;
   if (message == WM_CREATE) {
      this_device = (monitor_object_p) (((LPCREATESTRUCT) lParam)->lpCreateParams);
      // syslog("EditProc: process WM_CREATE, this_device=0x%08X\n", (unsigned) this_device) ;
      this_device->hwndOduEditor = hwnd ;
      // this_device->editwin_open = 1 ;
   } else {
      this_device = find_object_from_hwndOduEditor(hwnd) ;
      if (this_device == 0) {
         switch (message) {
         //  list messages to be ignored
         case WM_GETMINMAXINFO:
         case WM_NCCREATE:
         case WM_NCCALCSIZE:
            break;
         default:
            syslog("EditProc: cannot find parent object, message=%s\n",
               lookup_winmsg_name(message)) ;
            break;
         }

         return DefWindowProc(hwnd, message, wParam, lParam) ;
      }
   }

   //***************************************************
   //  debug: log all windows messages
   //***************************************************
   // this_device->winmessages = true ;
   if (dbg_flags & DBG_WINMSGS) {
      switch (message) {
      //  list messages to be ignored
      case WM_NCHITTEST:
      case WM_SETCURSOR:
      case WM_MOUSEMOVE:
      case WM_NCMOUSEMOVE:
      case WM_COMMAND:
         break;
      default:
         syslog("ED: [%s]\n", lookup_winmsg_name(message)) ;
         break;
      }
   }

   //********************************************************************
   //  Windows message handler for this dialog
   //********************************************************************
   switch (message) {
   case WM_CREATE:
      {
//       TCHAR msgstr[81] ;
//       wsprintf(msgstr, _T("ODU Command Editor [M%u: %s]"), this_device->index,
//          ascii2unicode(get_sernum(this_device))) ;
//       // 00000:  4F 00 44 00 55 00 20 00 43 00 6F 00 6D 00 6D 00  | O.D.U. .C.o.m.m. |
//       // 00010:  61 00 6E 00 64 00 20 00 45 00 64 00 69 00 74 00  | a.n.d. .E.d.i.t. |
//       // 00020:  6F 00 72 00 20 00 5B 00 4D 00 32 00 3A 00 20 00  | o.r. .[.M.2.:. . |
//       // 00030:  30 00 32 00 32 00 32 00 34 00 36 00 5D 00 00 00  | 0.2.2.2.4.6.]... |
//       // hex_dump((u8 *) msgstr, 64) ;
//       SetWindowText(hwnd, msgstr) ;

      TCHAR msgstr[81] ;
      wsprintf(msgstr, _T("ODU Command Editor [M%u: %s]"), this_device->index,
         // get_sernum(this_device)) ;
         ascii2unicode(get_sernum(this_device))) ;
      SetWindowText(hwnd, msgstr) ;
      // SetWindowText(hwnd, _T("Test")) ;

      //  create the internal edit window
      this_device->hwndEdit = CreateWindow(_T("edit"), NULL,
                          WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL |
                          WS_BORDER | ES_LEFT | ES_MULTILINE |
                          ES_AUTOHSCROLL | ES_AUTOVSCROLL,
                          0, 0, 0, 0, hwnd, (HMENU) IDC_EDITWIN,
                          ((LPCREATESTRUCT) lParam)->hInstance, NULL) ;
      }
      return 0 ;

   case WM_SIZE:
      MoveWindow(this_device->hwndEdit, 0, 0, LOWORD (lParam), HIWORD (lParam), true) ;
      return 0 ;

   case WM_SETFOCUS:
      SetFocus(this_device->hwndEdit) ;
      return 0 ;

   case WM_COMMAND:
      switch(LOWORD(wParam)) {
      case IDC_SEND2ODU: //  send to ODU
         send_commands(this_device) ;
         DestroyWindow(hwnd);
         return 0 ;

      case IDC_LOADFILE:  //  "Send Buffer" button, read/upload lines from a file
         if (select_text_file(hwnd, upload_file_name)) {  //  was command_filename
            result = read_command_file(this_device, upload_file_name);
            save_cfg_file();  //  save upload_file_name to config file
            
            if (result != 0) {
               syslog("%s: %s", upload_file_name, get_system_message(result)) ;
            }
         } 
         else {
            syslog("select_text_file: %s", get_system_message()) ;
         }
         
         return 0 ;
      }  //lint !e744
      break ;

   case WM_CLOSE:
      DestroyWindow (hwnd) ;
      return 0 ;

   case WM_DESTROY:
      // syslog("EditWin: WM_DESTROY\n") ;
      // this_device->editwin_open = 0 ;
      ref_image_thread = NULL ;
      PostQuitMessage(0) ;
      return 0 ;
   }  //lint !e744
   return DefWindowProc(hwnd, message, wParam, lParam) ;
}

//****************************************************************************
static DWORD WINAPI fRefImageThread(LPVOID iValue)
{
   HWND hwnd = CreateWindow(szEditName, szEditName,
        WS_OVERLAPPEDWINDOW,
        GetSystemMetrics (SM_CXSCREEN) / 4,
        GetSystemMetrics (SM_CYSCREEN) / 4,
        800, 600,
        // GetSystemMetrics (SM_CXSCREEN) / 2,
        // GetSystemMetrics (SM_CYSCREEN) / 2,
        NULL, NULL, g_hinst, iValue) ;

   ShowWindow(hwnd, SW_SHOW) ;
   UpdateWindow(hwnd) ;
   // hAccel = LoadAccelerators (hInstance, szEditName) ;

   MSG      msg ;
//  interesting...  If I enable USE_GETMESSAGE_HWND, and use hwnd in GetMessage(),
//  then PostQuitMessage() does not terminate the message loop!!
//  Actually, this problem only occurs if the message handler is a Window.
//  Dialogs terminate just fine in this case.
// #ifdef  USE_GETMESSAGE_HWND
//    while (GetMessageA (&msg, hwnd, 0, 0)) {
// #else
   while (GetMessage(&msg, NULL, 0, 0)) {
// #endif
      TranslateMessage (&msg) ;
      DispatchMessage (&msg) ;
   }
   // syslog("EditWin: thread exiting\n") ;
   return msg.wParam ;
}

//****************************************************************************
void open_edit_window(void)
{
   if (ref_image_thread == NULL) 
      ref_image_thread = new CThread(fRefImageThread, (LPVOID) NULL, NULL) ;
}

