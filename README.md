# GTPackCap
ENet packet capturing software intended to use with Growtopia.
NOTE: Works on Windows only!

### Warning: This program will output all of your packets into files, INCLUDING YOUR PERSONAL LOGIN DETAILS! Please remove the packet files after inspection.

## Building (from scratch)
1. Place enetrepeater.cpp in a new Visual Studio project.
2. Add enet.lib, ws2_32.lib and winmm.lib to the additional libraries.
3. Add enet to the Additional include directories.
4. Build the project.
If you did everything correctly, you should now have enetrepeater.exe somewhere in your project folders.
## Usage

1. Open enetrepeater.exe
2. Open Growtopia and login.
3. Once you have sent/received the packets you need, just log out of Growtopia.
4. Close the application by pressing Ctrl+C

To inspect the packets you may need some external program such as a hex editor.
