//****************************************************************************
//  Copyright (c) 2025  Daniel D Miller
//  pimage_mgr - Plus42 Image Manager
//  parse_layout_file.cpp - parse the .layout file, into appropriate data list
//
//  Written by:  Dan Miller
//****************************************************************************
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>  //  atoi
#include <tchar.h>

//********************************************************************************
//Display: 58,120 4 6 9EA48D 242A26
//********************************************************************************
static int scale_display(TCHAR *inpstr, FILE *outfd)
{
   static bool entry_shown = false ;
   TCHAR outstr[MAX_LINE_LEN+1] ;
   uint outlen ;
   uint xnum, ynum ;
   TCHAR *hd ;
   hd = next_field(inpstr);
   if (hd == NULL) { puts("PARSE ERROR"); return 1 ; }
   outlen = (uint) hd - (uint) inpstr ;
   _tcsncpy(outstr, inpstr, outlen);   //  copy intro data to output

   //  get x0, y0
   xnum = (uint) atoi(hd) ;
   xnum = scale_x(xnum, SCALE_ROUND) ;
   
   hd = _tcschr(hd, ',');
   if (hd == NULL) { puts("PARSE ERROR"); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) atoi(hd) ;
   ynum = scale_y(ynum, SCALE_ROUND) ;
   
   sprintf(outstr+outlen, "%u,%u ", xnum, ynum);
   outlen = _tcslen(outstr);  //  used for appending later data
   
   //  go to magnification fields
   hd = next_field(hd);
   if (hd == NULL) { puts("PARSE ERROR"); return 1 ; }
   
   xnum = (uint) atoi(hd) ;
   // xnum = scale_x(xnum, SCALE_ROUND) ;
   // xnum = scale_x(xnum, SCALE_FLOOR) ;
   double xnumd = scale_xd(xnum) ;
   
   hd = next_field(hd);
   if (hd == NULL) { puts("PARSE ERROR"); return 1 ; }
   ynum = (uint) atoi(hd) ;
   ynum = scale_y(ynum, SCALE_ROUND) ;
   
   hd = next_field(hd);
   sprintf(outstr+outlen, "%.2f %u %s", xnumd, ynum, hd);
   outlen = _tcslen(outstr);  //lint !e438
   
   // printf("D2: [%s] %u, hd: [%s]\n", outstr, outlen, hd);
   // fputs(inpstr, outfd);
   if (!entry_shown) {
      printf("Display in:  [%s]\n", inpstr);
      printf("Display out: [%s]\n", outstr);
      entry_shown = true ;
   }
   fprintf(outfd, "%s\n", outstr);
   return 0 ;
}

//********************************************************************************
//Annunciator: 1 60,90,30,26 1330,94
//********************************************************************************
static int scale_annunciator(TCHAR *inpstr, FILE *outfd)
{
   static bool entry_shown = false ;
   TCHAR outstr[MAX_LINE_LEN+1] ;
   uint outlen ;
   uint xnum, ynum ;
   TCHAR *hd ;
   // puts("found Annunciator");
   hd = next_field(inpstr);
   if (hd == NULL) { puts("PARSE ERROR"); return 1 ; }
   hd = next_field(hd);
   if (hd == NULL) { puts("PARSE ERROR"); return 1 ; }
   outlen = (uint) hd - (uint) inpstr ;
   _tcsncpy(outstr, inpstr, outlen);   //  copy intro data to output
   
   //  get x0, y0
   xnum = (uint) atoi(hd) ;
   xnum = scale_x(xnum, SCALE_ROUND) ;
   
   hd = _tcschr(hd, ',');
   if (hd == NULL) { puts("PARSE ERROR"); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) atoi(hd) ;
   ynum = scale_y(ynum, SCALE_ROUND) ;
   
   sprintf(outstr+outlen, "%u,%u,", xnum, ynum);
   outlen = _tcslen(outstr);  //  used for appending later data
   hd = _tcschr(hd, ',');
   if (hd == NULL) { puts("PARSE ERROR"); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   
   //  get dx, dy
   xnum = (uint) atoi(hd) ;
   xnum = scale_x(xnum, SCALE_ROUND) ;
   
   hd = _tcschr(hd, ',');
   if (hd == NULL) { puts("PARSE ERROR"); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) atoi(hd) ;
   ynum = scale_y(ynum, SCALE_ROUND) ;
   hd = next_field(hd) ;
   sprintf(outstr+outlen, "%u,%u ", xnum, ynum);
   outlen = _tcslen(outstr);  //  used for appending later data
   
   //  get x0, y0 for active-state bitmap
   xnum = (uint) atoi(hd) ;
   xnum = scale_x(xnum, SCALE_ROUND) ;
   
   hd = _tcschr(hd, ',');
   if (hd == NULL) { puts("PARSE ERROR"); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) atoi(hd) ;
   ynum = scale_y(ynum, SCALE_ROUND) ;
   sprintf(outstr+outlen, "%u,%u", xnum, ynum);
   outlen = _tcslen(outstr);  //lint !e438
   
   if (!entry_shown) {
      printf("Annun: in:  [%s]\n", inpstr);
      printf("Annun: out: [%s]\n", outstr);
      entry_shown = true ;
   }
   fprintf(outfd, "%s\n", outstr);
   return 0 ;
}

//********************************************************************************
//Key: 2 117,450,102,106 127,478,82,58 1389,478
//  Anything tweaked in Key: must be tweaked the same in AltBkgd:
//********************************************************************************
static int scale_key(TCHAR *inpstr, FILE *outfd)
{
   static bool entry_shown = false ;
   TCHAR outstr[MAX_LINE_LEN+1] ;
   uint outlen ;
   uint xnum, ynum ;
   TCHAR *hd ;
   // puts("found Annunciator");
   hd = next_field(inpstr);
   if (hd == NULL) { puts("PARSE ERROR"); return 1 ; }
   hd = next_field(hd);
   if (hd == NULL) { puts("PARSE ERROR"); return 1 ; }
   outlen = (uint) hd - (uint) inpstr ;
   _tcsncpy(outstr, inpstr, outlen);   //  copy intro data to output
   
   //******************************************************************
   //  get x0, y0 (sensitive rectangle)
   xnum = (uint) atoi(hd) ;
   xnum = scale_x(xnum, SCALE_ROUND) ;
   
   hd = _tcschr(hd, ',');
   if (hd == NULL) { puts("PARSE ERROR"); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) atoi(hd) ;
   ynum = scale_y(ynum, SCALE_ROUND) ;
   
   sprintf(outstr+outlen, "%u,%u,", xnum, ynum);
   outlen = _tcslen(outstr);  //  used for appending later data
   hd = _tcschr(hd, ',');
   if (hd == NULL) { puts("PARSE ERROR"); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   // hd++ ;
   
   //  get dx, dy
   xnum = (uint) atoi(hd) ;
   xnum = scale_x(xnum, SCALE_ROUND) ;
   
   hd = _tcschr(hd, ',');
   if (hd == NULL) { puts("PARSE ERROR"); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) atoi(hd) ;
   ynum = scale_y(ynum, SCALE_ROUND) ;
   hd = next_field(hd) ;
   sprintf(outstr+outlen, "%u,%u ", xnum, ynum);
   outlen = _tcslen(outstr);  //  used for appending later data
   
   //******************************************************************
   //  get x0, y0 (display rectangle)
   xnum = (uint) atoi(hd) ;
   xnum = scale_x(xnum, SCALE_ROUND) ;
   
   hd = _tcschr(hd, ',');
   if (hd == NULL) { puts("PARSE ERROR"); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) atoi(hd) ;
   ynum = scale_y(ynum, SCALE_ROUND) ;
   
   sprintf(outstr+outlen, "%u,%u,", xnum, ynum);
   outlen = _tcslen(outstr);  //  used for appending later data
   hd = _tcschr(hd, ',');
   if (hd == NULL) { puts("PARSE ERROR"); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   // hd++ ;
   
   //  get dx, dy
   xnum = (uint) atoi(hd) ;
   xnum = scale_x(xnum, SCALE_ROUND) ;
   
   hd = _tcschr(hd, ',');
   if (hd == NULL) { puts("PARSE ERROR"); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) atoi(hd) ;
   ynum = scale_y(ynum, SCALE_ROUND) ;
   hd = next_field(hd) ;
   sprintf(outstr+outlen, "%u,%u ", xnum, ynum);
   outlen = _tcslen(outstr);  //  used for appending later data
   
   //  get x0, y0 for active-state bitmap
   xnum = (uint) atoi(hd) ;
   xnum = scale_x(xnum, SCALE_ROUND) ;
   
   hd = _tcschr(hd, ',');
   if (hd == NULL) { puts("PARSE ERROR"); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) atoi(hd) ;
   ynum = scale_y(ynum, SCALE_ROUND) ;
   sprintf(outstr+outlen, "%u,%u", xnum, ynum);
   outlen = _tcslen(outstr);  //lint !e438
   
   // puts("found Key");
   // fprintf(outfd, "%s\n", inpstr);
   if (!entry_shown) {
      printf("Key: in:  [%s]\n", inpstr);
      printf("Key: out: [%s]\n", outstr);
      entry_shown = true ;
   }
   fprintf(outfd, "%s\n", outstr);
   return 0 ;
}

//********************************************************************************
//AltBkgd: 1 1294,2,192,84 864,196
//  Anything tweaked in Key: must be tweaked the same in AltBkgd:
//********************************************************************************
static int scale_altbkgd(TCHAR *inpstr, FILE *outfd)
{
   // puts("found AltBkgd");
   // fprintf(outfd, "%s\n", inpstr);
   static bool entry_shown = false ;
   TCHAR outstr[MAX_LINE_LEN+1] ;
   uint outlen ;
   uint xnum, ynum ;
   TCHAR *hd ;
   // puts("found Annunciator");
   hd = next_field(inpstr);
   if (hd == NULL) { puts("PARSE ERROR 1"); return 1 ; }
   hd = next_field(hd);
   if (hd == NULL) { puts("PARSE ERROR 2"); return 1 ; }
   outlen = (uint) hd - (uint) inpstr ;
   _tcsncpy(outstr, inpstr, outlen);   //  copy intro data to output
   
   //******************************************************************
   //  get x0, y0 
   xnum = (uint) atoi(hd) ;
   xnum = scale_x(xnum, SCALE_ROUND) ;
   
   hd = _tcschr(hd, ',');
   if (hd == NULL) { puts("PARSE ERROR 3"); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) atoi(hd) ;
   ynum = scale_y(ynum, SCALE_ROUND) ;
   
   sprintf(outstr+outlen, "%u,%u,", xnum, ynum);
   outlen = _tcslen(outstr);  //  used for appending later data
   hd = _tcschr(hd, ',');
   if (hd == NULL) { puts("PARSE ERROR 4"); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   
   //  get dx, dy
   xnum = (uint) atoi(hd) ;
   xnum = scale_x(xnum, SCALE_ROUND) ;
   
   hd = _tcschr(hd, ',');
   if (hd == NULL) { puts("PARSE ERROR 5"); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) atoi(hd) ;
   ynum = scale_y(ynum, SCALE_ROUND) ;
   hd = next_field(hd) ;
   sprintf(outstr+outlen, "%u,%u ", xnum, ynum);
   outlen = _tcslen(outstr);  //  used for appending later data
   
   //  get x0, y0 for active-state bitmap
   xnum = (uint) atoi(hd) ;
   xnum = scale_x(xnum, SCALE_ROUND) ;
   
   hd = _tcschr(hd, ',');
   if (hd == NULL) { puts("PARSE ERROR 6"); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) atoi(hd) ;
   ynum = scale_y(ynum, SCALE_ROUND) ;
   sprintf(outstr+outlen, "%u,%u", xnum, ynum);
   outlen = _tcslen(outstr);  //lint !e438
   
   if (!entry_shown) {
      printf("altbg: in:  [%s]\n", inpstr);
      printf("altbg: out: [%s]\n", outstr);
      entry_shown = true ;
   }
   fprintf(outfd, "%s\n", outstr);
   return 0 ;
}

//********************************************************************************
//AltKey: 1 14 1298,386
//********************************************************************************
static int scale_altkey(TCHAR *inpstr, FILE *outfd)
{
   // puts("found AltKey");
   static bool entry_shown = false ;
   TCHAR outstr[MAX_LINE_LEN+1] ;
   uint outlen ;
   uint xnum, ynum ;
   TCHAR *hd ;
   // puts("found Annunciator");
   hd = next_field(inpstr);
   if (hd == NULL) { puts("PARSE ERROR 1"); return 1 ; }
   hd = next_field(hd);
   if (hd == NULL) { puts("PARSE ERROR 2"); return 1 ; }
   hd = next_field(hd);
   if (hd == NULL) { puts("PARSE ERROR 3"); return 1 ; }
   outlen = (uint) hd - (uint) inpstr ;
   _tcsncpy(outstr, inpstr, outlen);   //  copy intro data to output
   
   //  get x0, y0 for active-state bitmap
   xnum = (uint) atoi(hd) ;
   xnum = scale_x(xnum, SCALE_ROUND) ;
   
   hd = _tcschr(hd, ',');
   if (hd == NULL) { puts("PARSE ERROR 4"); return 1 ; }
   hd = skip_spaces_and_commas(hd);
   ynum = (uint) atoi(hd) ;
   ynum = scale_y(ynum, SCALE_ROUND) ;
   sprintf(outstr+outlen, "%u,%u", xnum, ynum);
   outlen = _tcslen(outstr);  //lint !e438
   
   if (!entry_shown) {
      printf("altkey: in:  [%s]\n", inpstr);
      printf("altkey: out: [%s]\n", outstr);
      entry_shown = true ;
   }
   fprintf(outfd, "%s\n", outstr);
   return 0 ;
}

//********************************************************************************
static int scale_layout_values(TCHAR *dest_file, TCHAR *source_file)
{
   int result ;
   FILE *infd = fopen(source_file, "rt");
   if (infd == NULL) {
      printf("%s: cannot open for reading\n", source_file);
      return 1 ;
   }
   FILE *outfd = fopen(dest_file, "wt");
   if (outfd == NULL) {
      printf("%s: cannot open file for writing\n", dest_file);
      return 1 ;
   }
   
   TCHAR inpstr[MAX_LINE_LEN+1] ;
   while (fgets(inpstr, MAX_LINE_LEN, infd) != NULL) {
      strip_newlines(inpstr);
      
      //*******************************************************************
      //Display: 58,120 4 6 9EA48D 242A26
      if (_tcsncmp(inpstr, "Display:", 8) == 0) {
         result = scale_display(inpstr, outfd);
         if (result != 0) {
            break ;
         }
      }

      //*******************************************************************
      //Annunciator: 1 60,90,30,26 1330,94
      else if (_tcsncmp(inpstr, "Annunciator:", 12) == 0) {
         result = scale_annunciator(inpstr, outfd);
         if (result != 0) {
            break ;
         }
      }

      //*******************************************************************
      //Key: 2 117,450,102,106 127,478,82,58 1389,478
      else if (_tcsncmp(inpstr, "Key:", 4) == 0) {
         result = scale_key(inpstr, outfd);
         if (result != 0) {
            break ;
         }
      }

      //*******************************************************************
      //AltBkgd: 1 1294,2,192,84 864,196
      else if (_tcsncmp(inpstr, "AltBkgd:", 8) == 0) {
         result = scale_altbkgd(inpstr, outfd);
         if (result != 0) {
            break ;
         }
      }

      //*******************************************************************
      //AltKey: 1 14 1298,386
      else if (_tcsncmp(inpstr, "AltKey:", 7) == 0) {
         result = scale_altkey(inpstr, outfd);
         if (result != 0) {
            break ;
         }
      }
      else {
         fprintf(outfd, "%s\n", inpstr);
      }
   }
   
   fclose(infd);
   fclose(outfd);
   return 0 ;
}

