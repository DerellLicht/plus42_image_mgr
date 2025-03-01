extern TCHAR const * const Version ;

extern uint dbg_flags ;
#define  DBG_WINMSGS    1

#define  MAX_PATH_LEN      1024

extern HINSTANCE g_hinst ;

BOOL CmdAbout(HWND hwnd);
void open_image_window(TCHAR *image_file);

//***********************************************************************
//  args for center_window:
//  x=-1  center width in window
//  y=-1  center height in window
//  y > 0  set Y position to y
//***********************************************************************
void center_window(HWND hwnd, int x_pos, int y_pos);


