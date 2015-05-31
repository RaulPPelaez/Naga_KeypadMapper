# Naga_KeypadMapper
This little linux xorg daemon allows you to map the side keypad of the Razer Naga series mice via a configuration file called mapping.txt under $HOME/.naga/

#INSTALLATION

It does not need any dependencies besides having installed xdotools http://www.semicomplete.com/projects/xdotool/  (in the oficial ubuntu repositories)
and g++

Just compile the code using $ g++ -O3 -o naga naga.cpp
And place it wherever you like, you can run it automatically at startup if you want.


#USAGE

Init the mapper by calling: $ ./naga /dev/input/by-id/<NAGA_KEYPAD>

where <NAGA_KEYPAD> is the name of the keypad in this folder. 
In my case:   /dev/input/by-id/usb-Razer_Razer_Naga_Epic-if01-event-kbd

In order to get rid of the original bindings you have to disable the keypad using xinput as follows:

$ xinput set-int-prop <id> "Devide Enabled" 8 0

where <id> is the id number of the keypad returned by $ xinput.

You can enable it back to completly restore the changes by changing the last 0 to a 1

