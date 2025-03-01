extern TCHAR const * const Version ;

extern uint dbg_flags ;
#define  DBG_WINMSGS    1

#define  MAX_PATH_LEN      1024

extern HINSTANCE g_hinst ;

//lint -esym(769, TERM_INFO, TERM_QUERY, TERM_ERROR)
//  indices into term_atable[]
enum {
TERM_NORMAL = 0,
TERM_INFO,
TERM_QUERY,
TERM_ERROR
} ;

#define  NUM_TERM_ATTR_ENTRIES   4

void set_term_attr(uint atidx);
int termout(const char *fmt, ...);
int put_color_term_msg(uint idx, const char *fmt, ...);

BOOL CmdAbout(HWND hwnd);
void open_image_window(TCHAR *image_file);

//***********************************************************************
//  args for center_window:
//  x=-1  center width in window
//  y=-1  center height in window
//  y > 0  set Y position to y
//***********************************************************************
void center_window(HWND hwnd, int x_pos, int y_pos);


