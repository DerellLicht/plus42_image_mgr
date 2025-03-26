//****************************************************************************
//  Copyright (c) 2025  Daniel D Miller
//  pimage_mgr - Plus42 Image Manager
//  parse_tlayout_file.cpp - parse the .layout file, into appropriate data list
//
//  Written by:  Dan Miller
//****************************************************************************
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>  //  _ttoi
#include <tchar.h>
#include <gdiplus.h>

#include "common.h"
#include "commonw.h"
#include "pimage_mgr.h"

using namespace Gdiplus;

//********************************************************************************
//  table of field types and associated keyboard areas
//  
//  field type    touch    draw     select
//  ==========    =====    ====     ======
//  key             X       X         X
//  annunciator             X         X
//  AltBgnd                src      dest
//  AltKey                            X
//********************************************************************************

typedef enum LAYOUT_FIELD_TYPES_e {
LAYOUT_KEY=0,
LAYOUT_ANNUN,
LAYOUT_ALT_BG,
LAYOUT_ALT_KEY
} LAYOUT_FIELD_TYPES_t;

typedef struct key_pos_s {
   uint x0 ;
   uint y0 ;
   uint dx ;
   uint dy ;
} key_pos_t ;

typedef struct key_layout_data_s {
   struct key_layout_data_s *next ;
   LAYOUT_FIELD_TYPES_t lftype ;
   uint key_norm ;
   uint key_shift ;
   key_pos_t touch_area ;
   key_pos_t draw_area ;
   key_pos_t selected_area ;
} key_layout_data_t, *key_layout_data_p ;

static key_layout_data_p top = NULL ;
static key_layout_data_p tail = NULL ;

//********************************************************************************
static key_layout_data_p alloc_new_field(LAYOUT_FIELD_TYPES_t lftype)
{
   key_layout_data_p kltemp = new key_layout_data_t ;
   ZeroMemory(kltemp, sizeof(key_layout_data_t));
   kltemp->lftype = lftype ;
   return kltemp ;
}

//********************************************************************************
static void add_field_to_list(key_layout_data_p kltemp)
{
   if (top == NULL) {
      top = kltemp ;
   }
   else {
      tail->next = kltemp ;
   }
   tail = kltemp ;
}

//********************************************************************************
static void flush_layout_field_list(void)
{
   key_layout_data_p ktemp ;
   key_layout_data_p kkill ;
   if (top == NULL) {
      return ;
   }
   uint kcount = 0 ;
   ktemp = top ;
   while (ktemp != NULL) {
      kkill = ktemp ;
      ktemp = ktemp->next ;
      free(kkill);
      kcount++ ;
   }
   top = NULL ;
   tail = NULL ;
   termout(_T("Flush layout file: %u elements deleted"), kcount);
}

//********************************************************************************
//Annunciator: 1 60,90,30,26 1330,94
//********************************************************************************
static int parse_annunciator(TCHAR *inpstr)
{
   uint xnum, ynum ;
   TCHAR *hd ;
   
   key_layout_data_p kltemp = alloc_new_field(LAYOUT_ANNUN);
   //  get annunciator index
   hd = next_field(inpstr);
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   
   xnum = (uint) _ttoi(hd) ;
   kltemp->key_norm = xnum ;
   
   //  get x0, y0
   hd = next_field(hd);
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, _T(','));
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   
   kltemp->draw_area.x0 = xnum ;
   kltemp->draw_area.y0 = ynum ;
   
   hd = _tcschr(hd, _T(','));
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   
   //  get dx, dy
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, _T(','));
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   
   kltemp->draw_area.dx = xnum ;
   kltemp->draw_area.dy = ynum ;
   
   //  get x0, y0 for active-state bitmap
   hd = next_field(hd) ;
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, _T(','));
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   kltemp->selected_area.x0 = xnum ;
   kltemp->selected_area.y0 = ynum ;
   kltemp->selected_area.dx = kltemp->draw_area.dx ;
   kltemp->selected_area.dy = kltemp->draw_area.dy ;

   add_field_to_list(kltemp);
   return 0 ;
}

//********************************************************************************
//  search for comma after current number,
//  but don't check beyond space/tab
//********************************************************************************
static TCHAR *check_comma_in_field(TCHAR *input)
{
   uint safety_count = 0 ;
   TCHAR *hd = input ;
   while (true) {
      switch (*hd) {
      //  if space/tab found, abort search with no result
      case _T(' '):
      case _T('\t'):
      case 0:
         return NULL ;
         
      case _T(','):
         return hd ;
         
      default:
         hd++ ;
         if (++safety_count > MAX_LINE_LEN) {
            return NULL ;
         }
         break ;
      }
   }
}

//********************************************************************************
//Key: 2 117,450,102,106 127,478,82,58 1389,478
//Key: 38,39   0,538,117,102  25,566,82,62 1287,566
//********************************************************************************
static int parse_key(TCHAR *inpstr)
{
   uint xnum, ynum = 0;
   TCHAR *hd ;
   TCHAR *tl ;

   key_layout_data_p kltemp = alloc_new_field(LAYOUT_KEY);

   // put_color_term_msg(TERM_ERROR, "found Annunciator");
   hd = next_field(inpstr);
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   
   xnum = (uint) _ttoi(hd) ;
   kltemp->key_norm = xnum ;
   //  next, see if there is a comma after first number, but before spaces
   tl = check_comma_in_field(hd);
   if (tl != NULL) {
      tl++ ;   //  skip past the comma
      ynum = (uint) _ttoi(tl) ;
      kltemp->key_shift = ynum ;
   }
   
   //******************************************************************
   //  get x0, y0 (sensitive rectangle)
   hd = next_field(hd);
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, _T(','));
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;

   kltemp->touch_area.x0 = xnum ;
   kltemp->touch_area.y0 = ynum ;
   
   //  get dx, dy
   hd = _tcschr(hd, _T(','));
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, _T(','));
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;

   kltemp->touch_area.dx = xnum ;
   kltemp->touch_area.dy = ynum ;
   
   //******************************************************************
   //  get x0, y0 (display rectangle)
   hd = next_field(hd) ;
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, _T(','));
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   
   kltemp->draw_area.x0 = xnum ;
   kltemp->draw_area.y0 = ynum ;
   
   //  get dx, dy
   hd = _tcschr(hd, _T(','));
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, _T(','));
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   
   kltemp->draw_area.dx = xnum ;
   kltemp->draw_area.dy = ynum ;
   // outlen += _stprintf(outstr+outlen, _T("D%u,%u,%u,%u "), 
   //    kltemp->draw_area.x0, kltemp->draw_area.y0,
   //    kltemp->draw_area.dx, kltemp->draw_area.dy);
   
   hd = next_field(hd) ;
   
   //******************************************************************
   //  get x0, y0 for active-state bitmap (selected_area)
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, _T(','));
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   
   kltemp->selected_area.x0 = xnum ;
   kltemp->selected_area.y0 = ynum ;
   kltemp->selected_area.dx = kltemp->draw_area.dx ;
   kltemp->selected_area.dy = kltemp->draw_area.dy ;
   
   add_field_to_list(kltemp);
   return 0 ;
}

//********************************************************************************
//AltBkgd: 1 1294,2,192,84 864,196
//  Anything tweaked in Key: must be tweaked the same in AltBkgd:
//********************************************************************************
static int parse_altbkgd(TCHAR *inpstr)
{
   uint xnum, ynum ;
   TCHAR *hd ;

   key_layout_data_p kltemp = alloc_new_field(LAYOUT_ALT_BG);

   hd = next_field(inpstr);
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 1")); return 1 ; }
   xnum = (uint) _ttoi(hd) ;
   kltemp->key_norm = xnum ;
   
   //******************************************************************
   //  get x0, y0 
   hd = next_field(hd);
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 2")); return 1 ; }
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, _T(','));
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 3")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   
   kltemp->draw_area.x0 = xnum ;
   kltemp->draw_area.y0 = ynum ;
   
   //  get dx, dy
   hd = _tcschr(hd, _T(','));
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 4")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, _T(','));
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 5")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;

   kltemp->draw_area.dx = xnum ;
   kltemp->draw_area.dy = ynum ;
   
   //  get x0, y0 for active-state bitmap
   hd = next_field(hd) ;
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, _T(','));
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 6")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   
   kltemp->selected_area.x0 = xnum ;
   kltemp->selected_area.y0 = ynum ;
   kltemp->selected_area.dx = kltemp->draw_area.dx ;
   kltemp->selected_area.dy = kltemp->draw_area.dy ;

   add_field_to_list(kltemp);
   return 0 ;
}

//********************************************************************************
//AltKey: 1 14 1298,386
//********************************************************************************
static int parse_altkey(TCHAR *inpstr)
{
   uint xnum, ynum ;
   TCHAR *hd ;

   key_layout_data_p kltemp = alloc_new_field(LAYOUT_ALT_KEY);

   hd = next_field(inpstr);
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 1")); return 1 ; }
   xnum = (uint) _ttoi(hd) ;
   kltemp->key_norm = xnum ;

   hd = next_field(hd);
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 2")); return 1 ; }
   ynum = (uint) _ttoi(hd) ;
   kltemp->key_shift = ynum ;
   
   hd = next_field(hd);
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 3")); return 1 ; }
   
   //  get x0, y0 for active-state bitmap
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, _T(','));
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 4")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   
   kltemp->selected_area.x0 = xnum ;
   kltemp->selected_area.y0 = ynum ;
   kltemp->selected_area.dx = 103 ;
   kltemp->selected_area.dy = 66 ;

   add_field_to_list(kltemp);
   return 0 ;
}

//********************************************************************************
// # Offset: key y0_as_int dy_as_int
// # Offset: key1;keyn y0_as_int dy_as_int
// # no spaces permitted between key1 and keyn
// Offset: 1;7 -4 +4
// Offset: 38 -5 +5
//********************************************************************************
typedef struct key_offset_s {
   int y0 ;
   int dy ;
} key_offset_t ;  //, *key_offset_p ;
#define  MAX_KEY_NUM    100
static key_offset_t key_offsets[MAX_KEY_NUM] ;

static int parse_offset(TCHAR *inpstr)
{
   uint keyn, keym, idx ;
   int y0, dy ;
   TCHAR *hd ;

   hd = next_field(inpstr);
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 1")); return 1 ; }
   keyn = (uint) _ttoi(hd) ;

   TCHAR *tl = _tcschr(hd, _T(','));
   if (tl == NULL) {
      keym = 0 ;
   }
   else {
      tl++ ;   //  skip past hyphen
      keym = (uint) _ttoi(tl) ;
   }
   
   hd = next_field(hd);
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 3")); return 1 ; }
   
   //  get x0, y0 for active-state bitmap
   y0 = (int) _ttoi(hd) ;
   hd = next_field(hd);
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 4")); return 1 ; }
   dy = (int) _ttoi(hd) ;
   
   if (keym == 0) {
      key_offsets[keyn].y0 = y0 ;
      key_offsets[keyn].dy = dy ;
      // termout(_T("offset: key %u y0: %d, dy: %d"), keyn, y0, dy);
   }
   else {
      // termout(_T("offset: keys %u-%u y0: %d, dy: %d"), keyn, keym, y0, dy);
      for (idx=keyn; idx<=keym; idx++) {
         key_offsets[idx].y0 = y0 ;
         key_offsets[idx].dy = dy ;
      }
   }
   return 0;
}

//********************************************************************************
//lint -esym(528, show_key_offsets)
static void show_key_offsets(void)
{
   uint idx ;
   termout(_T("show key_offsets table"));
   for (idx=0; idx<MAX_KEY_NUM; idx++) {
      if (key_offsets[idx].y0 != 0) {
         termout(_T("key %2u: y0: %d, dy: %d"), idx, key_offsets[idx].y0, key_offsets[idx].dy);
      }
   }
}

//********************************************************************************
static bool layout_read_done = false ;

int parse_layout_values(TCHAR *tlayout_file)
{
   int result ;
   //  reset previous displayed data
   if (layout_read_done) {
      redraw_calc_image();
      flush_layout_field_list();
      ZeroMemory((void *)&key_offsets[0], sizeof(key_offsets));
   }

   FILE *infd = _tfopen(tlayout_file, _T("rt"));
   if (infd == NULL) {
      put_color_term_msg(TERM_ERROR, _T("%s: cannot open for reading\n"), tlayout_file);
      return 1 ;
   }
   
   TCHAR inpstr[MAX_LINE_LEN+1] ;
   bool done = false ;
   while (_fgetts(inpstr, MAX_LINE_LEN, infd) != NULL) {
      strip_newlines(inpstr);
      
      //*******************************************************************
      //Annunciator: 1 60,90,30,26 1330,94
      if (_tcsncmp(inpstr, _T("Annunciator:"), 12) == 0) {
         result = parse_annunciator(inpstr);
         if (result != 0) {
            break ;
         }
      }

      //*******************************************************************
      //Key: 2 117,450,102,106 127,478,82,58 1389,478
      else if (_tcsncmp(inpstr, _T("Key:"), 4) == 0) {
         result = parse_key(inpstr);
         if (result != 0) {
            break ;
         }
      }

      //*******************************************************************
      //AltBkgd: 1 1294,2,192,84 864,196
      else if (_tcsncmp(inpstr, _T("AltBkgd:"), 8) == 0) {
         result = parse_altbkgd(inpstr);
         if (result != 0) {
            break ;
         }
      }

      //*******************************************************************
      //AltKey: 1 14 1298,386
      else if (_tcsncmp(inpstr, _T("AltKey:"), 7) == 0) {
         result = parse_altkey(inpstr);
         if (result != 0) {
            break ;
         }
      }
      else if (_tcsncmp(inpstr, _T("Offset:"), 7) == 0) {
         result = parse_offset(inpstr);
         if (result != 0) {
            break ;
         }
      }
      // else {
      //    _ftprintf(outfd, _T("%s\n"), inpstr);
      // }
      if (done) {
         break ;
      }
   }
   
   fclose(infd);

   show_layout_info(true);
   // show_key_offsets();
   layout_read_done = true ;   
   return 0 ;
}

//********************************************************************************
void show_layout_info(bool show_summary_only)
{
   key_layout_data_p kltemp;
   TCHAR outstr[MAX_LINE_LEN+1] ;
   int outlen ;
   uint counts[5] = {
      0, 0, 0, 0, 0
   };
   // termout(_T("Entering show_layout_info()"));
   for (kltemp = top; kltemp != NULL; kltemp = kltemp->next) {
      counts[kltemp->lftype]++ ;
      counts[4]++ ;  //  total count

      if (!show_summary_only) {
         outlen = 0 ;      
         switch(kltemp->lftype) {
         case LAYOUT_ANNUN:
            outlen += _stprintf(outstr+outlen, _T("annun: %u "),
               kltemp->key_norm) ;
            outlen += _stprintf(outstr+outlen, _T("D%u,%u,%u,%u "), 
               kltemp->draw_area.x0, kltemp->draw_area.y0,
               kltemp->draw_area.dx, kltemp->draw_area.dy);
            outlen += _stprintf(outstr+outlen, _T("S%u,%u,%u,%u "), 
               kltemp->selected_area.x0, kltemp->selected_area.y0,
               kltemp->selected_area.dx, kltemp->selected_area.dy);
            put_color_term_msg(TERM_NORMAL, _T("%s"), outstr);
            break ;
         
         case LAYOUT_KEY:
            outlen += _stprintf(outstr+outlen, _T("key: %u,%u "),
               kltemp->key_norm, kltemp->key_shift) ;
            outlen += _stprintf(outstr+outlen, _T("T%u,%u,%u,%u "), 
               kltemp->touch_area.x0, kltemp->touch_area.y0,
               kltemp->touch_area.dx, kltemp->touch_area.dy);
            outlen += _stprintf(outstr+outlen, _T("D%u,%u,%u,%u "), 
               kltemp->draw_area.x0, kltemp->draw_area.y0,
               kltemp->draw_area.dx, kltemp->draw_area.dy);
            outlen += _stprintf(outstr+outlen, _T("S%u,%u,%u,%u "), 
               kltemp->selected_area.x0, kltemp->selected_area.y0,
               kltemp->selected_area.dx, kltemp->selected_area.dy);
            put_color_term_msg(TERM_NORMAL, _T("%s"), outstr);
            break ;
            
         case LAYOUT_ALT_BG:
            outlen += _stprintf(outstr+outlen, _T("altbg: %u "),
               kltemp->key_norm) ;
            outlen += _stprintf(outstr+outlen, _T("D%u,%u,%u,%u "), 
               kltemp->draw_area.x0, kltemp->draw_area.y0,
               kltemp->draw_area.dx, kltemp->draw_area.dy);
            outlen += _stprintf(outstr+outlen, _T("S%u,%u,%u,%u "), 
               kltemp->selected_area.x0, kltemp->selected_area.y0,
               kltemp->selected_area.dx, kltemp->selected_area.dy);
            put_color_term_msg(TERM_NORMAL, _T("%s"), outstr);
            break ;
            
         case LAYOUT_ALT_KEY:
            outlen += _stprintf(outstr+outlen, _T("altkey: %u,%u "),
               kltemp->key_norm, kltemp->key_shift) ;
            outlen += _stprintf(outstr+outlen, _T("S%u,%u,%u,%u "), 
               kltemp->selected_area.x0, kltemp->selected_area.y0,
               kltemp->selected_area.dx, kltemp->selected_area.dy);
            put_color_term_msg(TERM_NORMAL, _T("%s"), outstr);
            break ;
            
         default:
            break ;
         }
      }
   }
   termout(_T("counts: key: %u, annun: %u, altbg: %u, altkey: %u, total: %u"),
      counts[0], counts[1], counts[2], counts[3], counts[4]);
}

//***********************************************************************************
static const COLORREF BGR_TOUCH   = 0x00C000 ;  //  green      touch/sensitive area
static const COLORREF BGR_DRAW    = 0x0000C0 ;  //  red        display/drawing area
static const COLORREF BGR_SELECT  = 0x00FFFF ;  //  yellow     active/highlighted area
static const COLORREF BGR_ADRAW   = 0xFFFF00 ;  //  cyan       AltBg display/drawing area
static const COLORREF BGR_ASELECT = 0x007FFF ;  //  orange     AltBg active/highlighted area
static const COLORREF BGR_ALTKEY  = 0xFFFFFF ;  //  white      AltKey area (unspecified size)

void draw_object_boxes(HWND hwnd)
{
   key_layout_data_p kltemp;
   for (kltemp = top; kltemp != NULL; kltemp = kltemp->next) {
      switch(kltemp->lftype) {
      case LAYOUT_ANNUN:
         Box(hwnd, kltemp->draw_area.x0, kltemp->draw_area.y0,
            kltemp->draw_area.dx, kltemp->draw_area.dy, BGR_DRAW);
         Box(hwnd, kltemp->selected_area.x0, kltemp->selected_area.y0,
            kltemp->selected_area.dx, kltemp->selected_area.dy, BGR_SELECT);
         break ;
      
      case LAYOUT_KEY:
         Box(hwnd, kltemp->touch_area.x0, 
                   kltemp->touch_area.y0 + key_offsets[kltemp->key_norm].y0,  //lint !e737
                   kltemp->touch_area.dx, 
                   kltemp->touch_area.dy + key_offsets[kltemp->key_norm].dy,  //lint !e737 
                   BGR_TOUCH);
         Box(hwnd, kltemp->draw_area.x0, kltemp->draw_area.y0,
            kltemp->draw_area.dx, kltemp->draw_area.dy, BGR_DRAW);
         Box(hwnd, kltemp->selected_area.x0, kltemp->selected_area.y0,
            kltemp->selected_area.dx, kltemp->selected_area.dy, BGR_SELECT);
         break ;
         
      case LAYOUT_ALT_BG:
         Box(hwnd, kltemp->draw_area.x0, kltemp->draw_area.y0,
            kltemp->draw_area.dx, kltemp->draw_area.dy, BGR_ADRAW);
         Box(hwnd, kltemp->selected_area.x0, kltemp->selected_area.y0,
            kltemp->selected_area.dx, kltemp->selected_area.dy, BGR_ASELECT);
         break ;
         
      case LAYOUT_ALT_KEY:
         Box(hwnd, kltemp->selected_area.x0, kltemp->selected_area.y0,
            kltemp->selected_area.dx, kltemp->selected_area.dy, BGR_ALTKEY);
         break ;
         
      default:
         break ;
      }
   }
}

//***********************************************************************************
static const COLORREF KEYNUM = 0xFFFFFF ;  //  white      AltKey area (unspecified size)
static TCHAR font_name_message[LF_FULLFACESIZE] = _T("Times New Roman") ;

void show_key_numbers(HWND hwnd)
{
   TCHAR outstr[10] ;
   uint slen ;
   HFONT hfont = build_font(font_name_message, 24, EZ_ATTR_NORMAL) ;
   if (hfont == 0) {
      syslog(_T("build_font: %s\n"), get_system_message()) ;
   } else {
      PostMessage(hwnd, WM_SETFONT, (WPARAM) hfont, (LPARAM) true) ;
   }
   HDC hdc = GetDC (hwnd) ;
   SelectObject (hdc, hfont) ;
   // Clear_Window(hdc, 0);
   SetBkMode(hdc, TRANSPARENT) ;
   SetTextColor(hdc, KEYNUM) ;
   
   key_layout_data_p kltemp;
   for (kltemp = top; kltemp != NULL; kltemp = kltemp->next) {
      switch(kltemp->lftype) {
      case LAYOUT_KEY:
         //  show kltemp->key_norm
         slen = _stprintf(outstr, _T("%u"), kltemp->key_norm);
         TextOut (hdc, kltemp->touch_area.x0+2, kltemp->touch_area.y0+2, outstr, slen);
         // Box(hwnd, kltemp->touch_area.x0, kltemp->touch_area.y0,
         //    kltemp->touch_area.dx, kltemp->touch_area.dy, BGR_TOUCHx);
         break ;
         
      case LAYOUT_ANNUN:
      case LAYOUT_ALT_BG:
      case LAYOUT_ALT_KEY:
      default:
         break ;
      }
   }
   DeleteObject (SelectObject (hdc, GetStockObject (SYSTEM_FONT)));
   ReleaseDC (hwnd, hdc) ;
}
