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

#include "common.h"
#include "pimage_mgr.h"

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
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   
   kltemp->draw_area.x0 = xnum ;
   kltemp->draw_area.y0 = ynum ;
   
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   
   //  get dx, dy
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   
   kltemp->draw_area.dx = xnum ;
   kltemp->draw_area.dy = ynum ;
   
   //  get x0, y0 for active-state bitmap
   hd = next_field(hd) ;
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, ',');
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
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;

   kltemp->touch_area.x0 = xnum ;
   kltemp->touch_area.y0 = ynum ;
   
   //  get dx, dy
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;

   kltemp->touch_area.dx = xnum ;
   kltemp->touch_area.dy = ynum ;
   
   //******************************************************************
   //  get x0, y0 (display rectangle)
   hd = next_field(hd) ;
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   
   kltemp->draw_area.x0 = xnum ;
   kltemp->draw_area.y0 = ynum ;
   
   //  get dx, dy
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, ',');
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
   hd = _tcschr(hd, ',');
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
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 3")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   
   kltemp->draw_area.x0 = xnum ;
   kltemp->draw_area.y0 = ynum ;
   
   //  get dx, dy
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 4")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 5")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;

   kltemp->draw_area.dx = xnum ;
   kltemp->draw_area.dy = ynum ;
   
   //  get x0, y0 for active-state bitmap
   hd = next_field(hd) ;
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, ',');
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
   hd = _tcschr(hd, ',');
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
int parse_layout_values(TCHAR *tlayout_file)
{
   int result ;
   FILE *infd = _tfopen(tlayout_file, _T("rt"));
   if (infd == NULL) {
      put_color_term_msg(TERM_ERROR, _T("%s: cannot open for reading\n"), tlayout_file);
      return 1 ;
   }
   // termout(_T("Entering parse_layout_values()"));
   // uint lcount = 0 ;
   TCHAR inpstr[MAX_LINE_LEN+1] ;
   bool done = false ;
   while (_fgetts(inpstr, MAX_LINE_LEN, infd) != NULL) {
      strip_newlines(inpstr);
      // update_counter_field(lcount++);  //  DEBUG
      
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
      // else {
      //    _ftprintf(outfd, _T("%s\n"), inpstr);
      // }
      if (done) {
         break ;
      }
   }
   
   fclose(infd);
   enable_load_layout_button(false);
   show_layout_info(true);
   // termout(_T("Leaving parse_layout_values(), line count: %u"), lcount);
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
static const COLORREF BGR_TOUCH   = 0x00C000 ;  //  0.75 green
static const COLORREF BGR_DRAW    = 0x0000C0 ;  //  0.75 red
static const COLORREF BGR_SELECT  = 0x00FFFF ;  //  yellow
static const COLORREF BGR_ADRAW   = 0xFFFF00 ;  //  cyan
static const COLORREF BGR_ASELECT = 0x007FFF ;  //  orange
static const COLORREF BGR_ALTKEY  = 0xFFFFFF ;  //  white

void draw_object_boxes(HWND hwnd)
{
   key_layout_data_p kltemp;
   for (kltemp = top; kltemp != NULL; kltemp = kltemp->next) {
      switch(kltemp->lftype) {
      case LAYOUT_ANNUN:
         Box(hwnd, 
            kltemp->draw_area.x0, 
            kltemp->draw_area.y0,
            kltemp->draw_area.x0 + kltemp->draw_area.dx, 
            kltemp->draw_area.y0 + kltemp->draw_area.dy,
            BGR_DRAW);
         Box(hwnd, 
            kltemp->selected_area.x0, 
            kltemp->selected_area.y0,
            kltemp->selected_area.x0 + kltemp->selected_area.dx, 
            kltemp->selected_area.y0 + kltemp->selected_area.dy,
            BGR_SELECT);
         break ;
      
      case LAYOUT_KEY:
         Box(hwnd, 
            kltemp->touch_area.x0, 
            kltemp->touch_area.y0,
            kltemp->touch_area.x0 + kltemp->touch_area.dx, 
            kltemp->touch_area.y0 + kltemp->touch_area.dy,
            BGR_TOUCH);
         Box(hwnd, 
            kltemp->draw_area.x0, 
            kltemp->draw_area.y0,
            kltemp->draw_area.x0 + kltemp->draw_area.dx, 
            kltemp->draw_area.y0 + kltemp->draw_area.dy,
            BGR_DRAW);
         Box(hwnd, 
            kltemp->selected_area.x0, 
            kltemp->selected_area.y0,
            kltemp->selected_area.x0 + kltemp->selected_area.dx, 
            kltemp->selected_area.y0 + kltemp->selected_area.dy,
            BGR_SELECT);
         break ;
         
      case LAYOUT_ALT_BG:
         Box(hwnd, 
            kltemp->draw_area.x0, 
            kltemp->draw_area.y0,
            kltemp->draw_area.x0 + kltemp->draw_area.dx, 
            kltemp->draw_area.y0 + kltemp->draw_area.dy,
            BGR_ADRAW);
         Box(hwnd, 
            kltemp->selected_area.x0, 
            kltemp->selected_area.y0,
            kltemp->selected_area.x0 + kltemp->selected_area.dx, 
            kltemp->selected_area.y0 + kltemp->selected_area.dy,
            BGR_ASELECT);
         break ;
         
      case LAYOUT_ALT_KEY:
         Box(hwnd, 
            kltemp->selected_area.x0, 
            kltemp->selected_area.y0,
            kltemp->selected_area.x0 + kltemp->selected_area.dx, 
            kltemp->selected_area.y0 + kltemp->selected_area.dy,
            BGR_ALTKEY);
         break ;
         
      default:
         break ;
      }
   }
}
