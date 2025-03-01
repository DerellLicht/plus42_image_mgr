##  Plus42 Image Manager

The ultimate goal of this utility is to support creation of new Plus42 skins.  
The sequence of operations will be thus:

<ins>**data extraction**</ins>
1. read/display skin_name.gif
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
   
### Caveats
> Initially, development will take place using .png image files, not .gif.  
  This is because I'm more familiar with the former format than the latter.
  
<hr>

