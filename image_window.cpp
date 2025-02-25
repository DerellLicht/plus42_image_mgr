//*****************************************************************
//  copy rainbow code from gstuff application
//                                           
//  Written by:   Daniel D. Miller           
//                                           
//  Last Update:  03/10/03 11:34             
//                                           
//  compile with: g++ -Wall -O3 -s rainbow.cpp -o rainbow.exe -lgdi32
//*****************************************************************
//  RAINBOWV.CPP:  A rainbow simulator.                            
//  From "Astronomical Computing"                                  
//  Sky & Telescope Magazine, February 1991                        
//  Original program was in BASIC and EGA 4-color video mode.      
//                                                                 
//  05/27/98
//    Converted to C and VGA/16-color by Dan Miller.               
//  08/09/02 16:11
//    Converted into Win32 graphics program for more colors
//*****************************************************************


static char szAppName[] = "rainbow" ;
static char szAppDesc[] = "draw simulated rainbow" ;

#include <windows.h>
#include <stdlib.h>
#include <math.h>

#ifdef  USE_JPG_FILE
#include "jpeg_read.h"
#endif
#include "rainbow.h"

#ifdef USE_BGND_IMAGE
// #include "dib.h"
char *img_name = (char *) "wfall.bmp" ;
char *imj_name = (char *) "wfall.jpg" ;
#endif
//  comment this line out to sleep on message queue
#define CONTINUOUS_REDRAW 1

//  comment this line out to draw black window background 
// #define USE_SYS_BG_COLOR  1

//***********************************************************************
int cxClient = 0 ;
int cyClient = 0 ;

int force_redraw = 0 ;
unsigned xbase, xdiff, ybase, ydiff ;

HINSTANCE g_hinst = 0 ;

char tempstr[128] ;
static void display_current_operation(HWND hwnd);

/************************************************************************/
char *get_system_message(void)
{
   static char msg[261] ;
   int slen ;

   LPVOID lpMsgBuf;
   FormatMessage( 
      FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM | 
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      GetLastError(),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
      (LPTSTR) &lpMsgBuf,
      0, 0);
   // Process any inserts in lpMsgBuf.
   // ...
   // Display the string.
   strncpy(msg, (char *) lpMsgBuf, 260) ;

   // Free the buffer.
   LocalFree( lpMsgBuf );

   //  trim the newline off the message before copying it...
   slen = strlen(msg) ;
   if (msg[slen-1] == 10  ||  msg[slen-1] == 10) {
      msg[slen-1] = 0 ;
   }
   return msg;
}

//***********************************************************************
#ifdef USE_BGND_IMAGE
LPSTR pBits = 0 ;
BITMAPINFO bmi ;

static void read_bitmap(HDC hdc, char *image_name)
{
   int result ;
   HBITMAP hbm ;
   // hbm = (HBITMAP) LoadBitmap(g_hinst, MAKEINTRESOURCE(IDB_BALL)
#ifdef USE_BMP_FILE
   hbm = (HBITMAP) LoadImage(g_hinst, image_name, IMAGE_BITMAP, 0, 0, 
               LR_LOADFROMFILE | LR_CREATEDIBSECTION) ;
#elif USE_JPG_FILE
   hbm = (HBITMAP) MakeImage(imj_name) ;
#else
   hbm = 0 ;
#endif
   if (hbm == NULL) {
      wsprintf(tempstr, "LoadImage: %s\n", get_system_message()) ;
      OutputDebugString(tempstr) ;
      return ;
   } 

   //  call GetDIBits() with NULL buffer, to get required buffer size
   bmi.bmiHeader.biSize = sizeof(BITMAPINFO) ;
   bmi.bmiHeader.biBitCount = 0 ;
   result = GetDIBits(hdc, hbm, 0, bmi.bmiHeader.biHeight, NULL, &bmi, DIB_RGB_COLORS) ;
   if (result == 0) {
      wsprintf(tempstr, "GetDIBits: %s\n", get_system_message()) ;
      OutputDebugString(tempstr) ;
   }
   //  allocate required buffer space
   // wsprintf(tempstr, "allocated %u, required %u\n", dwFileSize, bmi.bmiHeader.biSizeImage) ;
   // OutputDebugString(tempstr) ;
   pBits = (LPSTR) malloc (bmi.bmiHeader.biSizeImage) ;
   if (!pBits) {
      wsprintf(tempstr, "out of memory [%s] [%u]\n", img_name, bmi.bmiHeader.biSizeImage) ;
      OutputDebugString(tempstr) ;
      return ;
   }

   //  call GetDIBits() with buffer, to read actual data
   result = GetDIBits(hdc, hbm, 0, bmi.bmiHeader.biHeight, (LPVOID) pBits, &bmi, DIB_RGB_COLORS) ;
   if (result == 0) {
      wsprintf(tempstr, "GetDIBits: %s\n", get_system_message()) ;
      OutputDebugString(tempstr) ;
   }
   // wsprintf(tempstr, "image: width=%u, height=%u\n",
   //    bmi.bmiHeader.biWidth,
   //    bmi.bmiHeader.biHeight) ;
   // OutputDebugString(tempstr) ;

}

//***********************************************************************
static void paint_bitmap(HDC hdc)
{
   if (pBits != 0) {
      SetStretchBltMode (hdc, COLORONCOLOR) ;
      if (StretchDIBits(
         hdc,              // handle to device context
         0,                // x-coordinate of upper-left corner of dest. rectangle
         0,                // y-coordinate of upper-left corner of dest. rectangle
         cxClient,         // width of destination rectangle
         cyClient,         // height of destination rectangle
         0,                // x-coordinate of upper-left corner of source rectangle
         0,                // y-coordinate of upper-left corner of source rectangle
         // bm.bmWidth,       // width of source rectangle
         // bm.bmHeight,      // height of source rectangle
         bmi.bmiHeader.biWidth,       // width of source rectangle
         bmi.bmiHeader.biHeight,      // height of source rectangle
         (void *) pBits,   // address of bitmap bits
         &bmi,             // address of bitmap data
         DIB_RGB_COLORS,   // usage flags
         SRCCOPY           // raster operation code
      ) == (int) GDI_ERROR) {
         
      // BITMAP    bm;
      // GetObject(hbm, sizeof(BITMAP), (LPSTR)&bm);
      //  try StretchDiBits() instead??  (may also support jpg & png)
      // HDC hdcBits = CreateCompatibleDC(hdc);
      // SelectObject(hdcBits,hbm);
      // if (!StretchBlt(
      //    hdc,      // handle to destination device context
      //    0, // x-coordinate of upper-left corner of dest. rectangle
      //    0, // y-coordinate of upper-left corner of dest. rectangle
      //    cxClient,   // width of destination rectangle
      //    cyClient,  // height of destination rectangle
      //    hdcBits,       // handle to source device context
      //    0,  // x-coordinate of upper-left corner of source rectangle
      //    0,  // y-coordinate of upper-left corner of source rectangle
      //    bm.bmWidth,    // width of source rectangle
      //    bm.bmHeight,   // height of source rectangle
      //    SRCCOPY       // raster operation code
      // )) {
      
      // if (!BitBlt(hdc,0,0,bm.bmWidth,bm.bmHeight,hdcBits,0,0,SRCCOPY)) {
         wsprintf(tempstr, "BitBlt/StretchDIBits: %s\n", get_system_message()) ;
         OutputDebugString(tempstr) ;
      } 
   }
}
#endif

//***********************************************************************
LRESULT CALLBACK WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
   HDC    hdc ;
   PAINTSTRUCT ps;

   switch (iMsg) {
    
   case WM_CREATE:
      cxClient = 0 ;
      cyClient = 0 ;
      //  read our data file into memory
#ifdef USE_BGND_IMAGE
      hdc = GetDC (hwnd) ;
      read_bitmap(hdc, img_name) ;
      ReleaseDC (hwnd, hdc) ;
#endif
      return 0 ;

   case WM_SIZE:
      cxClient = LOWORD (lParam) ;
      cyClient = HIWORD (lParam) ;

      xbase = cxClient / 2 ;     
      xdiff = (cxClient-50) / 2 ;
      ybase = cyClient * 7 / 8 ; 
      ydiff = cyClient * 5 / 8 ; 

#ifdef USE_BGND_IMAGE
      force_redraw = 1 ;
      // hdc = GetDC (hwnd) ;
      // // draw_bitmap(hdc, img_name, 0, 0) ;
      // paint_bitmap(hdc) ;
      // ReleaseDC (hwnd, hdc) ;
#endif
      return 0 ;

#ifdef USE_BGND_IMAGE
   case WM_ERASEBKGND:  //  try to avoid blank redraw
      return 0 ;  

   case WM_SETFOCUS:
      force_redraw = 1 ;
      return 0 ;
#endif

   case WM_PAINT:
      hdc = BeginPaint (hwnd, &ps) ;
#ifdef USE_BGND_IMAGE
      if (force_redraw) {
         paint_bitmap(hdc) ;
         force_redraw = 0 ;
      }
#endif
      display_current_operation(hwnd) ;
      EndPaint (hwnd, &ps) ;
      return 0 ;
    
   case WM_KEYDOWN :
   case WM_SYSKEYDOWN :
        if (wParam == 27) { //  look for escape key
#ifdef USE_BGND_IMAGE
            free(pBits) ;
#endif
            PostQuitMessage (0) ;
        }
        return 0 ;

   case WM_DESTROY:
#ifdef USE_BGND_IMAGE
      free(pBits) ;
#endif
      PostQuitMessage (0) ;
      return 0 ;
   }

   return DefWindowProc (hwnd, iMsg, wParam, lParam) ;
}

/************************************************************************/
#undef  USE_PIXEL_OR
// #define  USE_PIXEL_OR

void rainbow_plot_pixel(HDC hdc, int pcolor, double TH)
{
   double f1 ;
   unsigned XP, YP ;

   TH = fabs(TH) ;
   if (TH > 60.0)
      return ;
   f1 = TH / 60.0 ;

   XP = (unsigned) (xbase + xdiff * (f1 *     (X / B))) ;
   YP = (unsigned) (ybase - ydiff * (f1 * fabs(Y / B))) ;
#ifndef USE_PIXEL_OR
   SetPixel(hdc, XP, YP, rainbow_index[pcolor]) ;
#else
   //  reality check - using the preceding write method, you get an
   //  inner region which is a colorful mosaic of constantly-changing
   //  primary colors.  In a *real* rainbow, what you see is an
   //  inner region which is an iridescent white.  The reason for 
   //  this is that the various colors are mixed together, essentially
   //  performing a visual boolean-OR of all the raindrop reflections.
   //  This effect can be simulated in this program by ORing each new
   //  pixel with the existing pixel at the target location:
   SetPixel(hdc, XP, YP, (rainbow_index[pcolor] | GetPixel(hdc, XP, YP))) ;
#endif
}

/************************************************************************/
//  this function plots one primary and one secondary pixel
/************************************************************************/
void display_current_operation(HWND hwnd)
// void DrawRainbow(HWND hwnd)
{
   HDC hdc ;
   int pcolor ;
   double N, I, R, T1, T2, RS, RP, RB, RC, I1, I2 ;

   if (cxClient == 0 || cyClient == 0)
        return ;

   // sprintf(tempstr, "cx=%u, cy=%u\n", cxClient, cyClient) ;
   // MessageBox(hwnd, tempstr, "status", MB_OK) ;

   // 30 REM  RANDOM IMPACT PARAMETER
   X = -1.0 + 2 * random_part() ;
   Y = -1.0 + 2 * random_part() ;
   B = (double) sqrt(X * X + Y * Y) ; //  square root
   // 50 IF B >= 1 THEN 30
   if (B >= 1.0) {
      return ;
   }

   hdc = GetDC (hwnd) ;
   //  select random color and calculate index of refraction
   pcolor = random_int(6) ; //  select 1 thru 6
   N = 1.33 + 0.01 * (double) (pcolor) ;

   // 70 REM  COMPUTE ANGLES
   I  = asin(B) ;     //  angle of incidence
   R  = asin(B / N) ; //  angle of refraction

   //  calculate observation angles for 
   //  primary (T1) and secondary (T2) rainbows.
   T1 = (4 * R - 2 * I) * RADS2DEGS ;
   T2 = (6 * R - 2 * I) * RADS2DEGS - 180 ;

   // 95 REM  INTENSITY FACTORS
   // RS = (sin(I - R) / sin(I + R)) ^ 2 ;
   RS = (double) (sin(I - R) / sin(I + R)) ;
   RS *= RS ;
   RP = (double) (tan(I - R) / tan(I + R)) ;
   RP *= RP ;
   RB = (1 - RP) * (1 - RP) ;
   RC = (1 - RS) * (1 - RS) ;
   I1 = (     RS * RC +      RP + RB) / 2 ;
   I2 = (RS * RS * RC + RP * RP * RB) / 2 ;

   //  Greenler's method: rather than computing intensities
   //  for individual points, we randomly throw away some
   //  computed points in the low-intensity regions.
   if (I1 >= .04 * random_part())
      rainbow_plot_pixel(hdc, pcolor, T1) ;

   if (I2 >= .02 * random_part())
      rainbow_plot_pixel(hdc, pcolor, T2) ;

   ReleaseDC (hwnd, hdc) ;
}

//***********************************************************************
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                  PSTR szCmdLine, int iCmdShow)
   {
   HWND        hwnd ;
   MSG         msg ;
   WNDCLASSEX  wndclass ;

   g_hinst = hInstance ;

   wndclass.cbSize        = sizeof (wndclass) ;
   wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
   wndclass.lpfnWndProc   = WndProc ;
   wndclass.cbClsExtra    = 0 ;
   wndclass.cbWndExtra    = 0 ;
   wndclass.hInstance     = hInstance ;
   // wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION) ;
   // wndclass.hIconSm       = LoadIcon (NULL, IDI_APPLICATION) ;
   wndclass.hIcon         = LoadIcon (g_hinst, MAKEINTRESOURCE(RBICON));
   wndclass.hIconSm       = LoadIcon (g_hinst, MAKEINTRESOURCE(RBICON));
   wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
#ifdef   USE_SYS_BG_COLOR  
   wndclass.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1) ;
#else
   wndclass.hbrBackground = (HBRUSH) GetStockObject (BLACK_BRUSH) ;
#endif
   wndclass.lpszMenuName  = NULL ;
   wndclass.lpszClassName = szAppName ;

   RegisterClassEx (&wndclass) ;

   hwnd = CreateWindow (szAppName, szAppDesc,
                        WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT,
                        CW_USEDEFAULT, CW_USEDEFAULT,
                        NULL, NULL, hInstance, NULL) ;

   ShowWindow (hwnd, iCmdShow) ;
   UpdateWindow (hwnd) ;

   while (GetMessage (&msg, NULL, 0, 0)) {
      TranslateMessage (&msg) ;
      DispatchMessage (&msg) ;
   }
   return msg.wParam ;
}

