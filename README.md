# Naga_KeypadMapper
This little linux xorg daemon allows you to map the side keypad of the Razer Naga series mice via a configuration file called `mapping_xx.txt` under `$HOME/.naga/` 
Requires: `xdotool` and an X server environment to work.

Currently tested with:  
- Razer Naga Epic Chroma in CentOS 7  
- Razer Naga Epic (pre-2014 version) in Ubuntu 14.04, 15.04, 15.10  
- Razer Naga (RZ01-0028) (thanks to khornem) in Ubuntu 14.04  
- Razer Naga 2014 (thanks to Destroyer) in Ubuntu 15.04, 15.10  
- Razer Naga Molten (thanks to noobxgockel) in Linux Mint 17.02  
- Razer Chroma (thanks to felipeacsi) in Manjaro
- Razer Naga 2012 (RZ01-0058) (thanks to mrlinuxfish, brianfreytag) in Arch Linux, Ubuntu 16.04
- Razer Naga Chroma (thanks to ipsod) in Linux Mint KDE 18.1

This daemon does not, in any case modify any system file nor property of any device. So the process is totally reversible just by deleting the files and at most rebooting. 

CAUTION, in this alpha version the run option wont work for text environment commands, like for example `top`.
As an alpha version, it is very prone to bugs and other sorts of failure. I release this project without any sort of warranty, so use under your own responsibility.

## CONFIGURATION
The configuration file `mapping_xx.txt` has the following syntax:

    <keynumber> - <option>=<action>
    
    <keynumber> is a number between 1-14 representing the 12 keys of the naga's keypad + two on the top of the naga.

    <option>
    Switch mapping: chmap
    Key (holds the key as long as the button is pressed, or presses+releases if inside a macro) or shortcut: key
	Toggle a key (first press will mimic a key being pressed, the second will release it): toggle
    Running system commands: run, run2(runs the command at key press and key release)	
    Mouse click: click 
    Switching workspace relatively: workspace_r
    Switching workspace absolutly: workspace
    Position mouse cursor: position
    Add a delay between actions : delay
	Peform a media action: media

    <action>
    For chmap: path to a new mapping file 
    For key and toggle: is the custom key mapping, might be a single key like A or a combination like ctrl+t (following xdotool's syntax)
    For run and run2: a system command like gedit or a custom script or bash line like bash /usr/local/bin/custom.bash
    For click: number of the mouse button, see table below
    For workspace_r: positive or negative number e.g. +2 (go two workspaces forward) -1 (previous)
    For workspace: min 0, max `xdotool get_num_desktops`-1
    For position: x,y which are the relative position in pixel from the left upper corner of the display
    For delay: delay in milliseconds

### MOUSE FUNCTION (click)
Button number | Info
------------ | -------------
1 | left button
2 | middle button (pressing the scroll wheel)
3 | right button
4 | turn scroll wheel up
5 | turn scroll wheel down
6 | push scroll wheel left (some mouse only)
7 | push scroll wheel right (some mouse only)
8 | 4th button (aka backward button)
9 | 5th button (aka forward button)
### KEYBOARD FUNCTION (key)
For mapping a key from keyboard you need to look up your key e.g. here: http://cgit.freedesktop.org/xorg/proto/x11proto/plain/keysymdef.h . You need to exclude the beginning `XK_` so for example caps lock would be `Caps_Lock`. 
If you want to test your shortcut you can use `xdotool key --window getactivewindow KEYorSHORTCUT` . If no error appears the shortcut works. **Keep in mind this not only tests but also executes the shortcut.**
### NOTES
If the `$HOME/.naga/mapping_01.txt` file is missing the daemon won't start (the program will NOT autocreate this file, the install.sh script will copy example files though).

For a given button action multiple actions may be defined. They will be executed sequentially (this is called a macro).

An example `mapping_xx.txt` configuration file is the following:

    #There must be no blank lines at the beginning of the file, yeah lazy parsing. Comments are accepted though
    1 - key=ctrl+t
    2 - toggle=A
    3 - click=8
    4 - key=C
    5 - click=9
    6 - workspace_r=1
    7 - workspace_r=-1
    8 - key=G
    9 - position=331,7
    9 - click=1
    9 - delay=100
    9 - position=343,72
    9 - click=1
    10 - run=gedit
    11 - key=H
    12 - key=Return
    13 - workspace=0
    14 - chmap=mapping_02.txt


If you want to dig more into configuration, you might find these tools useful: `xinput`, `evtest`

Keep in mind that any non existing functionality can be created through the "run" option, at the end of the day naga just calls xdotools, which can be done from a script.  
## INSTALLATION

KeypadMapper does not need any dependencies besides having installed `xdotool` http://www.semicomplete.com/projects/xdotool/  (in the oficial ubuntu, fedora, centOS, etc repositories) and g++

Change `src/naga.cpp` to adapt the installation to another device, using different inputs and/or different key codes than the Naga Epic, 2014, Molten or Chroma. For Example, Epic Chroma is compatible with Epic (they have the same buttons), so you would only have to add an additional line to the devices vector.

Run `bash install.sh` .
This will compile the source and copy the necessary files (see `install.sh` for more info).  
It will prompt you for your password, as it uses sudo to copy some files.
 

## USAGE

**Install with ``` bash install.sh```**  
This will copy the necessary files and start the daemon. After running this you should have mapping_01.txt working.  

#### In depth
The installation process automatically executes the daemon in the background and set it to start at boot for you. But you can still run it manually as follows:

`nagastart.sh` does the below process automatically:

1) Inits the mapper by calling: `$./naga` 

2) In order to get rid of the original bindings it disables the keypad using xinput as follows:

    $ xinput set-int-prop [id] "Device Enabled" 8 0

where [id] is the id number of the keypad returned by $ xinput.

4) You may have to also run 

    $ xinput set-button-map [id2] 1 2 3 4 5 6 7 11 10 8 9 13 14 15

where [id2] is the id number of the pointer device returned by `xinput` - in case of naga 2014 you also have to check which of those two has more than 7 numbers by typing `xinput get-button-map [id2]`. Although this seems to be unnecesary in some systems (i.e CentOS 7)

This lasts until the x server is restarted (`nagastart.sh` is aware of this), but you can enable it back to completely restore the changes by changing the last 0 to a 1 in 2).

## UNINSTALLATION

To uninstall you just need to run ```$bash uninstall.sh```.




