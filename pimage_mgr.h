extern uint dbg_flags ;
#define  DBG_WINMSGS    1

extern HINSTANCE g_hinst ;

//  message handler handle for RefImage window
extern HWND hwndRef ;

extern TCHAR layout_file[MAX_PATH_LEN] ;

//lint -esym(757, WM_SET_INPUT_CURSOR)
static const UINT WM_DRAW_BOX     = (WM_USER + 101) ;
static const UINT WM_DRAW_IMAGE   = (WM_USER + 102) ;
static const UINT WM_LOAD_LAYOUT  = (WM_USER + 103) ;
static const UINT WM_SHOW_LAYOUT  = (WM_USER + 104) ;
static const UINT WM_SHOW_KEYNUMS = (WM_USER + 105) ;
// static const UINT WM_SHOW_KEYNUMS = (WM_USER + 105) ;

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
int  termout(const TCHAR *fmt, ...);
int  put_color_term_msg(uint idx, const TCHAR *fmt, ...);

BOOL CmdAbout(HWND hwnd);
void update_counter_field(uint counter);

//***********************************************************************
//  args for center_window:
//  x=-1  center width in window
//  y=-1  center height in window
//  y > 0  set Y position to y
//***********************************************************************
void center_window(HWND hwnd, int x_pos, int y_pos);
void enable_load_layout_button(bool state);
void enable_show_layout_button(bool state);
void enable_draw_boxes_button(bool state);
void enable_show_keys_button(bool state);
void enable_show_image_list(bool state);

//  show_ref_image.cpp
void open_image_window(TCHAR *image_file);
void stop_ref_image_thread(void);
void redraw_calc_image(void);

//  parse_layout_file.cpp
int  parse_layout_values(TCHAR *layout_file);
void show_layout_info(bool show_summary_only);
void draw_object_boxes(HWND hwnd);
void show_key_numbers(HWND hwnd);

//  show_image_list.cpp
void open_image_list_window(void);
void stop_image_list_thread(void);

