# RadialEnvMap
A technical experiment in Radial/Equirectangular environment mapping. 

This style of environment mapping has the advantage of only needing a single 2D texture
and not having issues with seam filtering. 

However, expensive ALU math is needed to perform this style of texture mapping. 

![Screenshot](RadialEnvMap.jpg?raw=true "Demo screenshot")

##Author##
This demo was made by Damian Trebilco based on a demo by Emil Persson, aka Humus.


##Controls##

 Space - Toggles frame rate counter

 F1    - Opens the settings/rendering options dialog. 

 ESC   - Exit mouse look / close F1 menu / Exit application

 Left mouse - Enter mouse look mode

 WASD  - Move around (can be configued from F1 menu) 


##Legal stuff##

This demo is freeware and may be used by anyone for any purpose
and may be distributed freely to anyone using any distribution
media or distribution method as long as this file is included.


##Compiling code##

The provided code was compiled with Visual Studio 2008 and should include all needed files.
Note that the FrameWork3 files have been changed slightly from the Humus framework code.

  
___
Do not conform any longer to the pattern of this world, but be transformed by the renewing of your mind.
Then you will be able to test and approve what God's will is - his good, pleasing and perfect will. (Romans 12:2 NIV)

