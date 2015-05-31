# Naga_KeypadMapper
This little linux xorg daemon allows you to map the side keypad of the Razer Naga series mice via a configuration file called mapping.txt under $HOME/.naga/

More info and mapping.txt syntax in README

Be adviced that I release this project without any sort of warranty. So use under your own responsability.

This daemon does not, in any case modify any system file nor propertie of any device. So the process is totally reversible just by deleting the files. It is absolutely non invasive


#INSTALLATION

It does not need any dependencies besides having installed xdotool http://www.semicomplete.com/projects/xdotool/  (in the oficial ubuntu repositories)
and g++

Just run install.sh as sudo.
This will compile the source and copy the necesary files (see install.sh for more info)


#USAGE
The instalation process automatically executes the daemon in the background and set it to start at boot for you. But you can still run it manually as follows:

nagastart.sh does the below process automatically:

naga executable has to be called as sudo or have the s bit up with chmod u+s at least.

Init the mapper by calling: $./naga /dev/input/by-id/[NAGA_KEYPAD]
I dont know how to overcome the need for sudo privileges, if you know let me know please!

where [NAGA_KEYPAD] is the name of the keypad in this folder. 
In my case:   /dev/input/by-id/usb-Razer_Razer_Naga_Epic-if01-event-kbd

In order to get rid of the original bindings you have to disable the keypad using xinput as follows:

$ xinput set-int-prop [id] "Device Enabled" 8 0

where [id] is the id number of the keypad returned by $ xinput.

This last until the x server is restarted (nagastart.sh is aware of this), but you can enable it back to completly restore the changes by changing the last 0 to a 1.

#UNNINSTALATION

You just have to delete the files created:

$sudo rm /usr/local/bin/naga ~/.config/startup/naga.desktop /usr/local/bin/nagastart.sh
