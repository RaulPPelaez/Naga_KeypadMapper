# Naga_KeypadMapper
Map razer naga devices keys with the config files : `mapping_xx.txt` under `$HOME/.naga/` 

Requires: `xdotool`, `xinput` and `g++`

Probably works with :
- Razer Naga Epic Chroma in CentOS 7
- Razer Naga Epic (pre-2014 version) in Ubuntu 14.04, 15.04, 15.10
- Razer Naga (RZ01-0028) (thanks to khornem) in Ubuntu 14.04
- Razer Naga Molten (thanks to noobxgockel) in Linux Mint 17.02
- Razer Chroma (thanks to felipeacsi) in Manjaro
- Razer Naga 2012 (RZ01-0058) (thanks to mrlinuxfish, brianfreytag) in Arch Linux, Ubuntu 16.04
- Razer Naga Chroma (thanks to ipsod) in Linux Mint KDE 18.1

Works for sure with :
- Razer Naga 2014 (Debian)

This tool doesn't modify file except `$HOME/.naga/`, `/etc/udev/rules.d/80-naga.rules` and `/usr/local/bin/(naga && nagastart.sh)` so deleting the files deletes the tool.

Make sure to add the users to the group razer with the command `groupadd -f razer`.

CAUTION, in this alpha version the run option wont work for text environment commands, like for example `top`.
As an alpha version, there might be bugs.

## CONFIGURATION
The configuration file `mapping_xx.txt` has the following syntax:

    <keynumber> - <option>=<command>
    
    <keynumber> is a number between 1-14 representing the 12 keys of the naga's keypad + two on the top of the naga.

    <option> decides what is going to be applied to <command>
	
	The possible choices are :
		-chmap : Changes the keymap for the file <option> (example `keymap.txt` in ~/.naga) .
		-key : Runs <command> in xdotool so the possible keys are on google ( i didn't find a list of all the keys ) .
			The <command> is run in : **xdotool keydown** or/and **keyup --window getactivewindow <command>** so it's already 'framed'.
			By example to play/pause music you can put **key=XF86AudioPlay**.
			The xdotool key is released when the key is released.
			There seems to be a list of keys on https://cgit.freedesktop.org/xorg/proto/x11proto/plain/keysymdef.h but you need to remove **XK_** and they're not all there so google them if you want to be sure.
		-run : Runs the command <command> on key press with setsid before the command.
		-run2 : Runs the command <command> on key press and release with setsid before the command.
		-run3 : Runs the command <command> on key press without setsid before the command (might freeze the whole daemon depending on the command, I did this in case any command might not work with setsid).
		-run4 : Runs the command <command> on key press and release without setsid before the command. (might also freeze the whole daemon but 2 times lul)
		-run5 : Runs the command <command> on key release with setsid.
		-run6 : Runs the command <command> on key release without setsid(might freeze the whole daemon depending on the command).
		-click : CLick based on xdotool click option. (Basically runs **xdotool click <command>**) (Can put numbers from 1 to 9 and options such as *--window** etc).
		-workspace_r : Runs <command> in **xdotool set_desktop --relative -- <command>** .
		-workspace : Runs <command> in **xdotool set_desktop <command>** .
		-position : Runs <command> in **xdotool mousemove <command>** .
		-delay : Sleeps for <command> milliseconds .
		-toggle : Toggle key <command>.
	
		If you're not sure about the run versions use the setsid ones (run, run2, run5).

    <command> is what is going to be used based on the option.
    
	To test any <command> run it in the command cited above.

[Link for Keys](https://cgit.freedesktop.org/xorg/proto/x11proto/plain/keysymdef.h){:target="_blank"}



### NOTES
If the `$HOME/.naga/mapping_01.txt` file is missing the daemon won't start (the program will NOT autocreate this file, the install.sh script will copy example files though). 
	//The copy of the config files might not succeed in wich case you can copy them manually.

For a key multiple actions may be defined. They will be executed sequentially.

An example `mapping_xx.txt` configuration file is the following:

    #There must be no blank lines at the beginning of the file. Comments are accepted
    1 - key=XF86AudioPlay
    2 - toggle=A
    3 - chmap=mapping_420.txt
    4 - run=notify-send 'Button # 4' 'Pressed'
    4 - run5=notify-send 'Button # 4' 'Released'
    #etc


If you want to dig more into configuration, you might find these tools useful: `xinput`, `evtest`

Any non existing functionality can be created through the "run" option.

## INSTALLATION

Dependencies : `xdotool`and `xinput`

Edit `src/naga.cpp` to adapt the installation to another device, using different inputs and/or different key codes than the Naga Epic, 2014, Molten or Chroma. For Example, Epic Chroma is compatible with Epic (they have the same buttons), so you would only have to add an additional line to the devices vector.

Run `sh install.sh` .
This will compile the source and copy the necessary files (see `install.sh` for more info).
It will prompt you for your password, as it uses sudo to copy some files.

## Autorun

Since autorun is a bit complicated for all the distros you can simply add nagastart.desktop or nagastart.sh to your startup folder/configuration.
(Might have to run chmod +x on the .desktop)

## Debugging

While running as root you can use naga -restart to restart the daemon and log the output.
Be careful running this as non root as it might not kill the older daemon
The commands are :
	`naga -stop`
	`naga -restart`

By running it from a terminal you can get the "Command : " output and debug your config/fork.

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

To uninstall you need to run `sh uninstall.sh`.




