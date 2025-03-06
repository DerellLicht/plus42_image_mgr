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
3. After selecting a layout file, click the <code>Open Image</code> button.  
   Okay, this is a practice page for gdiplus operations... it'll go away in a while.  
   Just click the <code>Draw Image</code> button...
4. This will display the reference .gif image for the skin that you selected earlier.
5. Last but not least... click the <code>Draw Box</code> three times...
   This will highlight the defined fields around 'Key: 2' fields ...  
   [ NOTE: This is using hard-coded values, and only works for LandscapeRight skin. ]
6. and that's it for now... more to come!   
   
That's all that's working at this point...
   
<hr>

The distribution site for this program is [My Home Page](https://derelllicht.42web.io/pimage_mgr.html)
