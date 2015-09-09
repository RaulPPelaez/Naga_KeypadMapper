# Naga_KeypadMapper
This little linux xorg daemon allows you to map the side keypad of the Razer Naga series mice via a configuration file called `mapping_xx.txt` under `$HOME/.naga/` 
Requires: `xdotool` and an X server enviroment to work.

Currently tested with:
-Razer Naga Epic (pre-2014 version) in Ubuntu 14.04,15.04
-Razer Naga 2014 (thanks to Destroyer) in Ubuntu 15.04
-Razer Naga Molten (thanks to noobxgockel) in Linux Mint 17.02

This daemon does not, in any case modify any system file nor propertie of any device. So the process is totally reversible just by deleting the files and at most rebooting. 

CAUTION, in this alpha version the run option wont work for text enviroment commands, like for example `top`.
As an alpha version, it is very prone to bugs and other sorts of faliure. I release this project without any sort of warranty, so use under your own responsability.

#CONFIGURATION
For Naga 2014 or Naga molten just change the param in nagastart.sh to `naga 2014` or `naga molten` .

The configuration file mapping_xx.txt has the following syntax:

    <keynumber> - <option>=<action>
    
    <keynumber> is a number between 1-14 representing the 12 keys of the naga's keypad + two on the top of the naga.

    <option>
    For switch mapping: chmap
    For key or shortcut: key
    For running system commands: run
    For mouse click: click 
    For switching workspace relatively: workspace_r
    For switching workspace absolutly: workspace

    <action>
    For chmap: path to a new mapping file 
    For key: is the custom key mapping, might be a single key like A or a combination like ctrl+t (following xdotool's syntax)
    For run: a system command like gedit or a custom script like bash /usr/local/bin/custom.bash
    For click: number of the mouse button, see table below
    For workspace_r: positive or negative number e.g. +2 (go two workspaces forward) -1 (previous)
    For workspace: min 0, max `xdotool get_num_desktops`-1


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

If the $HOME/.naga/mapping_xx.txt file is missing the daemon won't start: (the program will NOT autocreate this file, the install.sh script will copy an example files though)

An example mapping_xx.txt configuration file is the following:

    #There must be no blank lines at the beggining of the file, yeah lazy parsing. Comments are accepted though
    1 - key=ctrl+t
    2 - key=A
    3 - click=8
    4 - key=C
    5 - click=9
    6 - workspace_r=1
    7 - workspace_r=-1
    8 - key=G
    9 - key=alt+F4
    10 - run=gedit
    11 - key=H
    12 - key=Return
    13 - workspace=0
    14 - chmap=mapping_02.txt


If you want to dig more into configuration, you might find these tools useful: `xinput`, `evtest`

#INSTALLATION

KeypadMapper does not need any dependencies besides having installed `xdotool` http://www.semicomplete.com/projects/xdotool/  (in the oficial ubuntu repositories) and g++

Change nagastart.sh to adapt the installation to another device. You will also have to change a couple of lines in the source code if the device is not listed there, using different inputs and/or different key codes than the Naga Epic, 2014 or Molten - more information in src/naga.cpp.

Run `sudo bash install.sh` .
This will compile the source and copy the necesary files (see install.sh for more info)

 

#USAGE
The instalation process automatically executes the daemon in the background and set it to start at boot for you. But you can still run it manually as follows:

nagastart.sh does the below process automatically:

1) Inits the mapper by calling: `$./naga epic`, `$./naga 2014` or `$./naga molten`

2) In order to get rid of the original bindings it disables the keypad using xinput as follows:

    $ xinput set-int-prop [id] "Device Enabled" 8 0

where [id] is the id number of the keypad returned by $ xinput.

4) You have to also run 

    $ xinput set-button-map [id2] 1 2 3 4 5 6 7 11 10 8 9 13 14 15

where [id2] is the id number of the pointer device returned by $ xinput - in case of naga 2014 you also have to check which of those two has more than 7 numbers by typing $ xinput get-button-map [id2]

This lasts until the x server is restarted (nagastart.sh is aware of this), but you can enable it back to completly restore the changes by changing the last 0 to a 1.

#UNINSTALATION

You just have to delete the files created:

    $ sudo rm /usr/local/bin/naga* /etc/udev/rules.d/80-naga.rules ~/.config/autostart/naga.desktop
    $ sudo rm -r ~/.naga
    $ sudo gpasswd -d yourUserName razer