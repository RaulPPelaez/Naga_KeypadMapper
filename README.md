# Razer Key Mapper for Linux
..also can accept other devices they're simply untested. Contact me for adding devices.

Map razer naga devices keys with the config file : `keyMap.txt` under `$HOME/.naga/` 

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

This tool doesn't modify any files except `$HOME/.naga/`, `/etc/udev/rules.d/80-naga.rules` and `/usr/local/bin/(naga && nagaXinputStart.sh)`, so deleting the files deletes the tool.

Make sure to add the users to the group razer with the command `sudo gpasswd -a "$(whoami)" razer` if you create a new user.

CAUTION, in this alpha version the run option wont work for text environment commands, like for example `top`.
As an alpha version, there might be bugs.

## CONFIGURATION
The configuration file `keyMap.txt` has the following syntax:

    "config="<configName> set the name of the following config. The initial loaded config will always be <configName> = defaultConfig .

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

		"configEnd" Marks the end of <configName>.

You can have as many configs as you want in the keyMap.txt file, just make sure to give them different names and include defaultConfig.

[Link for Keys](https://cgit.freedesktop.org/xorg/proto/x11proto/plain/keysymdef.h)



### NOTES

To reload the config you can simply reference itself with :

    config=defaultConfig
    1 - chmap=defaultConfig
    configEnd

or swap to another config and comeback.

If the `$HOME/.naga/keyMap.txt` file is missing the daemon won't start (the program will NOT autocreate this file, the install.sh script will copy an example file though).

For a key multiple actions may be defined. They will then be executed sequentially at the key press.

An example `keyMap.txt` configuration file is the following:

    #Comments are accepted
    config=defaultConfig
    1 - key=XF86AudioPlay
    2 - toggle=A
    3 - chmap=420configEnemyBlazerWoW
    4 - run=notify-send 'Button # 4' 'Pressed'
    4 - run5=notify-send 'Button # 4' 'Released'
    #etc
    configEnd

    config=420configEnemyBlazerWoW
    1 - run=sh ~/hacks.sh
    2 - chmap=defaultConfig
    #etc
    configEnd

If you want to dig more into configuration, you might find these tools useful: `xinput`, `evtest`

Any non existing functionality can be created through the "run" option.

## INSTALLATION

Dependencies : `xdotool`, `xinput` and `g++`

Edit `src/naga.cpp` to adapt the installation to another device, using different inputs and/or different key codes than the Naga Epic, 2014, Molten or Chroma. For Example, Epic Chroma is compatible with Epic (they have the same buttons), so you would only have to add an additional line to the devices vector.

Run `sh install.sh` .
This will compile the source and copy the necessary files (see `install.sh` for more info).
It will prompt you for your password, as it uses sudo to copy some files.
The config files are copied to all the users (even root) homes.

## Autorun

Since autorun is a bit complicated for all the distros you can simply add nagastart.desktop or a script executing naga -start to your startup folder/configuration.
(Might have to run chmod +x on the .desktop)

## Debugging

The commands are :

	`naga -stop` //stops the daemon.
	`naga -start` //restart the daemon if there is one running and starts a hidden daemon.
	`naga -debug` //restart the daemon if there is one running and starts one in the console for debugging.
	`naga` //gives help

For all the double dashed ocd people the commands also works with 2 dashes.

#### In depth


1) In order to get rid of the original bindings it disables the keypad using xinput as follows:

    $ xinput set-int-prop [id] "Device Enabled" 8 0

where [id] is the id number of the keypad returned by $ xinput.

2) You may have to also run 

    $ xinput set-button-map [id2] 1 2 3 4 5 6 7 11 10 8 9 13 14 15

where [id2] is the id number of the pointer device returned by `xinput` - in case of naga 2014 you also have to check which of those two has more than 7 numbers by typing `xinput get-button-map [id2]`. Although this seems to be unnecesary in some systems (i.e CentOS 7)

## UNINSTALLATION

To uninstall you need to run `sh uninstall.sh`.




