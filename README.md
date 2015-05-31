# Naga_KeypadMapper
This little linux xorg daemon allows you to map the side keypad of the Razer Naga series mice via a configuration file called mapping.txt under $HOME/.naga/

The configuration file mapping.txt has the following syntax:

<keynumber> - <option>=<action>

<keynumber> is a number between 1-12 representing the 12 keys of the naga's keypad

<option> is either key (for remapping keys) or run (for running a system command, or a custom script through an alias i.e.)

<action> 
  For key: is the custom key mapping, might be a single key like A or a combination like ctrl+t (following xdotool's syntax)
  For run: a system command like top or a custom script like bash /usr/local/bin/custom.bash
  

if the mapping.txt file is missing the default mapping.txt corresponds to the numbers 0-11 assigned to the buttons: (the program will NOT autocreate this file)

An example mapping.txt configuration file is the following:
--------------------------
#There must be no blank lines at the beggining of the file, yeah lazy parsing. Comments are accepted though
1 - key=ctrl+t
2 - key=A
3 - key=B
4 - key=C
5 - key=D
6 - key=E
7 - key=F
8 - key=G
9 - key=alt+F4
10 - run=gedit
11 - key=H
12 - key=I
------------------------------
(Do not include the ---- in the actual file) 

CAUTION, in this first alpha version the run option wont work for text enviroment commands, like for example top.

INSTALLATION
It does not need any dependencies besides having installed xtools (in the oficial ubuntu repositories) and g++

Just compile the code using $ g++ -O3 -o naga naga.cpp
And place it wherever you like, you can run it automatically at startup if you want.


USAGE

Init the mapper by calling: $ ./naga /dev/input/by-id/<NAGA_KEYPAD>

where <NAGA_KEYPAD> is the name of the keypad in this folder. 
In my case:   /dev/input/by-id/usb-Razer_Razer_Naga_Epic-if01-event-kbd

In order to get rid of the original bindings you have to disable the keypad using xinput as follows:

$ xinput set-int-prop <id> "Devide Enabled" 8 0

where <id> is the id number of the keypad returned by $ xinput.

You can enable it back to completly restore the changes by changing the last 0 to a 1



