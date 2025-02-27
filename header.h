extern TCHAR const * const Version ;

extern uint dbg_flags ;
#define  DBG_WINMSGS    1

#define  MAX_PATH_LEN      1024

extern HINSTANCE g_hinst ;

BOOL CmdAbout(HWND hwnd);
void open_image_window(TCHAR *image_file);
//lint -esym(759, status_message)
void status_message(char *msgstr);
void status_message(uint idx, char *msgstr);


