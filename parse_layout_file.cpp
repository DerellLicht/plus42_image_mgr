//****************************************************************************
//  Copyright (c) 2025  Daniel D Miller
//  pimage_mgr - Plus42 Image Manager
//  parse_layout_file.cpp - parse the .layout file, into appropriate data list
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
   static bool entry_shown = false ;
   TCHAR outstr[MAX_LINE_LEN+1] ;
   uint outlen ;
   uint xnum, ynum ;
   TCHAR *hd ;
   
   key_layout_data_p kltemp = alloc_new_field(LAYOUT_ANNUN);

   // put_color_term_msg(TERM_ERROR, "found Annunciator");
   hd = next_field(inpstr);
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = next_field(hd);
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   outlen = (uint) hd - (uint) inpstr ;
   _tcsncpy(outstr, inpstr, outlen);   //  copy intro data to output
   
   //  get x0, y0
   xnum = (uint) _ttoi(hd) ;
   
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   
   put_color_term_msg(TERM_NORMAL, outstr+outlen, _T("%u,%u,"), xnum, ynum);
   outlen = _tcslen(outstr);  //  used for appending later data
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   
   //  get dx, dy
   xnum = (uint) _ttoi(hd) ;
   
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   hd = next_field(hd) ;
   put_color_term_msg(TERM_NORMAL, outstr+outlen, _T("%u,%u "), xnum, ynum);
   outlen = _tcslen(outstr);  //  used for appending later data
   
   //  get x0, y0 for active-state bitmap
   xnum = (uint) _ttoi(hd) ;
   
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   put_color_term_msg(TERM_NORMAL, outstr+outlen, _T("%u,%u"), xnum, ynum);
   outlen = _tcslen(outstr);  //lint !e438
   
   if (!entry_shown) {
      put_color_term_msg(TERM_NORMAL, _T("Annun: in:  [%s]\n"), inpstr);
      put_color_term_msg(TERM_NORMAL, _T("Annun: out: [%s]\n"), outstr);
      entry_shown = true ;
   }

   add_field_to_list(kltemp);
   return 0 ;
}

//********************************************************************************
//Key: 2 117,450,102,106 127,478,82,58 1389,478
//  Anything tweaked in Key: must be tweaked the same in AltBkgd:
//********************************************************************************
static int parse_key(TCHAR *inpstr)
{
   static bool entry_shown = false ;
   TCHAR outstr[MAX_LINE_LEN+1] ;
   uint outlen ;
   uint xnum, ynum ;
   TCHAR *hd ;

   key_layout_data_p kltemp = alloc_new_field(LAYOUT_KEY);

   // put_color_term_msg(TERM_ERROR, "found Annunciator");
   hd = next_field(inpstr);
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = next_field(hd);
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   outlen = (uint) hd - (uint) inpstr ;
   _tcsncpy(outstr, inpstr, outlen);   //  copy intro data to output
   
   //******************************************************************
   //  get x0, y0 (sensitive rectangle)
   xnum = (uint) _ttoi(hd) ;
   
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   
   put_color_term_msg(TERM_NORMAL, outstr+outlen, _T("%u,%u,"), xnum, ynum);
   outlen = _tcslen(outstr);  //  used for appending later data
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   // hd++ ;
   
   //  get dx, dy
   xnum = (uint) _ttoi(hd) ;
   
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   
   hd = next_field(hd) ;
   put_color_term_msg(TERM_NORMAL, outstr+outlen, _T("%u,%u "), xnum, ynum);
   outlen = _tcslen(outstr);  //  used for appending later data
   
   //******************************************************************
   //  get x0, y0 (display rectangle)
   xnum = (uint) _ttoi(hd) ;
   
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   
   put_color_term_msg(TERM_NORMAL, outstr+outlen, _T("%u,%u,"), xnum, ynum);
   outlen = _tcslen(outstr);  //  used for appending later data
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   // hd++ ;
   
   //  get dx, dy
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   
   hd = next_field(hd) ;
   put_color_term_msg(TERM_NORMAL, outstr+outlen, _T("%u,%u "), xnum, ynum);
   outlen = _tcslen(outstr);  //  used for appending later data
   
   //  get x0, y0 for active-state bitmap
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   
   put_color_term_msg(TERM_NORMAL, outstr+outlen, _T("%u,%u"), xnum, ynum);
   outlen = _tcslen(outstr);  //lint !e438
   
   if (!entry_shown) {
      put_color_term_msg(TERM_NORMAL, _T("Key: in:  [%s]\n"), inpstr);
      put_color_term_msg(TERM_NORMAL, _T("Key: out: [%s]\n"), outstr);
      entry_shown = true ;
   }
   add_field_to_list(kltemp);

   return 0 ;
}

//********************************************************************************
//AltBkgd: 1 1294,2,192,84 864,196
//  Anything tweaked in Key: must be tweaked the same in AltBkgd:
//********************************************************************************
static int parse_altbkgd(TCHAR *inpstr)
{
   static bool entry_shown = false ;
   TCHAR outstr[MAX_LINE_LEN+1] ;
   uint outlen ;
   uint xnum, ynum ;
   TCHAR *hd ;

   key_layout_data_p kltemp = alloc_new_field(LAYOUT_ALT_BG);

   // put_color_term_msg(TERM_ERROR, "found Annunciator");
   hd = next_field(inpstr);
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 1")); return 1 ; }
   hd = next_field(hd);
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 2")); return 1 ; }
   outlen = (uint) hd - (uint) inpstr ;
   _tcsncpy(outstr, inpstr, outlen);   //  copy intro data to output
   
   //******************************************************************
   //  get x0, y0 
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 3")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   
   put_color_term_msg(TERM_NORMAL, outstr+outlen, _T("%u,%u,"), xnum, ynum);
   outlen = _tcslen(outstr);  //  used for appending later data
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 4")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   
   //  get dx, dy
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 5")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;

   hd = next_field(hd) ;
   put_color_term_msg(TERM_NORMAL, outstr+outlen, _T("%u,%u "), xnum, ynum);
   outlen = _tcslen(outstr);  //  used for appending later data
   
   //  get x0, y0 for active-state bitmap
   xnum = (uint) _ttoi(hd) ;
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 6")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   
   put_color_term_msg(TERM_NORMAL, outstr+outlen, _T("%u,%u"), xnum, ynum);
   outlen = _tcslen(outstr);  //lint !e438
   
   if (!entry_shown) {
      put_color_term_msg(TERM_NORMAL, _T("altbg: in:  [%s]\n"), inpstr);
      put_color_term_msg(TERM_NORMAL, _T("altbg: out: [%s]\n"), outstr);
      entry_shown = true ;
   }

   add_field_to_list(kltemp);
   return 0 ;
}

//********************************************************************************
//AltKey: 1 14 1298,386
//********************************************************************************
static int parse_altkey(TCHAR *inpstr)
{
   // put_color_term_msg(TERM_ERROR, "found AltKey");
   static bool entry_shown = false ;
   TCHAR outstr[MAX_LINE_LEN+1] ;
   uint outlen ;
   uint xnum, ynum ;
   TCHAR *hd ;

   key_layout_data_p kltemp = alloc_new_field(LAYOUT_ALT_KEY);

   // put_color_term_msg(TERM_ERROR, "found Annunciator");
   hd = next_field(inpstr);
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 1")); return 1 ; }
   hd = next_field(hd);
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 2")); return 1 ; }
   hd = next_field(hd);
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 3")); return 1 ; }
   outlen = (uint) hd - (uint) inpstr ;
   _tcsncpy(outstr, inpstr, outlen);   //  copy intro data to output
   
   //  get x0, y0 for active-state bitmap
   xnum = (uint) _ttoi(hd) ;
   
   hd = _tcschr(hd, ',');
   if (hd == NULL) { put_color_term_msg(TERM_ERROR, _T("PARSE ERROR 4")); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) _ttoi(hd) ;
   put_color_term_msg(TERM_NORMAL, outstr+outlen, _T("%u,%u"), xnum, ynum);
   outlen = _tcslen(outstr);  //lint !e438
   
   if (!entry_shown) {
      put_color_term_msg(TERM_NORMAL, _T("altkey: in:  [%s]\n"), inpstr);
      put_color_term_msg(TERM_NORMAL, _T("altkey: out: [%s]\n"), outstr);
      entry_shown = true ;
   }

   add_field_to_list(kltemp);
   return 0 ;
}

//********************************************************************************
static int parse_layout_values(TCHAR *layout_file)
{
   int result ;
   FILE *infd = _tfopen(layout_file, _T("rt"));
   if (infd == NULL) {
      put_color_term_msg(TERM_ERROR, _T("%s: cannot open for reading\n"), layout_file);
      return 1 ;
   }
   TCHAR inpstr[MAX_LINE_LEN+1] ;
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
      // else {
      //    _ftprintf(outfd, _T("%s\n"), inpstr);
      // }
   }
   
   fclose(infd);
   return 0 ;
}

