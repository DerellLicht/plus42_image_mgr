##  Plus42 Image Manager

The ultimate goal of this utility is to support creation of new Plus42 skins.  
The sequence of operations will be thus:

<ins>**data extraction**</ins>
1. read/display skin_name (in .png format)
2. read skin_name.layout, and build a list of key, annunciator, and AltBkgd fields for the skin
3. copy the key fields and write them to an ImageList,
   along with a lookup table of length/width for each field
4. save the ImageList to a file

<ins>**Display ImageList contents**</ins>
1. display the fields of the ImageList, along with relevant index

<ins>**Build the new skin**</ins>
1. manually create a skin-format file describing indices and positions of images
   within new image
2. Let program generate a new .layout file for the new skin
    
<hr>

Usage instructions (development version)  

1. Click the button labeled <code>Start here</code>
2. Select a .layout file from some existing skin  
   You *may* need to do some folder-cruising to find such skins,
   but once you get there, it will remember the location for future runs.  
   
   This will display the reference .gif image for the skin that you selected earlier.
3. Click <code>Load Layout File</code> button to load the file [base skin name].layout.  
   A summary of fields decoded will be displayed in the terminal.  
   
4a. Click <code>Show Layout Data</code> button to display the fields read from the .layout file.  
4b. Click <code>Draw Frames</code> button to highlight the data fields which have been  
    extracted from the .layout data.  
    
    That is all that is completed at this point.
    
Colors used in <code>Draw Frames</code>  
```
BGR_TOUCH   = 0x00C000 ;  //  green      touch/sensitive area
BGR_TOUCHx  = 0xFF00FF ;  //  magenta    touch/sensitive area (alternate color)
BGR_DRAW    = 0x0000C0 ;  //  red        display/drawing area
BGR_SELECT  = 0x00FFFF ;  //  yellow     active/highlighted area
BGR_ADRAW   = 0xFFFF00 ;  //  cyan       AltBg display/drawing area
BGR_ASELECT = 0x007FFF ;  //  orange     AltBg active/highlighted area
BGR_ALTKEY  = 0xFFFFFF ;  //  white      AltKey area (unspecified size)
```
   
<hr>

The distribution site for this program is [My Home Page](https://derelllicht.42web.io/pimage_mgr.html)
