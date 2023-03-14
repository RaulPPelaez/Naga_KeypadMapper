# Razer Mouse Mapper for Linux
..also can accept other devices by adding the files. Contact me to add devices.

!!! ONE LINE INSTALL !!! :

`wget https://codeload.github.com/lostallmymoney/Razer_Mouse_Linux/zip/refs/heads/master -q -O Razer_Mouse_Linux.zip && unzip -f Razer_Mouse_Linux.zip && rm Razer_Mouse_Linux.zip && cd Razer_Mouse_Linux-master && sh install.sh && echo "If the install was successful you may remove this folder."`

(requires unzip)

Run sh install.sh from the directory to install.

If there is an error about config files just copy it to your /home/.naga/

Run in root if there is an error accessing the device, also look up naga debug it'll give all the info you need most of the time.

Map razer naga devices keys with the config file : `keyMap.txt` under `$HOME/.naga/`

Requires: `xdotool`, `xinput` and `g++`

Optionnal and probably already installed : `policykit-1`

Probably works with :
- Razer Naga Epic Chroma in CentOS 7
- Razer Naga Epic (pre-2014 version) in Ubuntu 14.04, 15.04, 15.10
- Razer Naga (RZ01-0028) (thanks to khornem) in Ubuntu 14.04
- Razer Naga Molten (thanks to noobxgockel) in Linux Mint 17.02
- Razer Chroma (thanks to felipeacsi) in Manjaro
- Razer Naga 2012 (RZ01-0058) (thanks to mrlinuxfish, brianfreytag) in Arch Linux, Ubuntu 16.04
- Razer Naga Chroma (thanks to ipsod) in Linux Mint KDE 18.1
- Razer Naga Trinity (thanks to haringsrob and ws141)
- Razer Pro Wireless (thanks to Stibax)

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
		-chmap : Changes the keymap for another config inside `keymap.txt` in ~/.naga .
		-champRelease : Changes the keymap on key release.
		-keyPress : Runs <command> in xdotool so the possible keys are on google ( i didn't find a list of all the keys ) .
			The <command> is run in : **xdotool keydown** or/and **keyup --window getactivewindow <command>** so it's already 'framed' by the program.
			By example to play/pause music you can put **key=XF86AudioPlay**.
		-sleep and sleepRelease :
			Sleeps
		-string and stringRelease :
			Writes a string, doesn't use xdotool so xdotool keys won't work
			
		-key : does keyPress at press and keyRelease at release
		-keyPress :
		-keyRelease :
			The xdotool key is released when the key is released.
			There seems to be a list of keys on https://cgit.freedesktop.org/xorg/proto/x11proto/plain/keysymdef.h but you need to remove **XK_** and they're not all there so google them if you want to be sure.
		-run : Runs the command <command> on key press with setsid before the command.
		-run2 : Runs the command without setsid.
		-runRelease : Runs a bash command when the button is released .
		-runRelease2 : Runs the command without setsid when button released.
		-setWorkspace : Runs <command> in **xdotool set_desktop <command>** .
		-mousePosition : Runs <command> in **xdotool mousemove <command>** .
		-keyClick : Press a key once when the button is pressed .
		-keyClickRelease : Press a key once when the button is released .
		
		#Use theses to press/unpress special chars that won't work using xdotool
		-specialPressOnPress : Supports 1 char only...
		-specialPressOnRelease : 1 char
		-specialReleaseOnPress : 1 char
		-specialReleaseOnRelease : 1 char

    		<command> is what is going to be used based on the option.

		To test any <command> run it in the command cited above.

		"configEnd" Marks the end of <configName>.
		
		For a mouseclick run xdotool click <command>) (Can put numbers from 1 to 9 and options such as *--window etc).

You may have as many configs as you want in the keyMap.txt file, just make sure to give them different names and include defaultConfig.

[Link for Keys](https://cgit.freedesktop.org/xorg/proto/x11proto/plain/keysymdef.h)



### NOTES

To reload the config run the command :
	naga start

which will restart the daemon

If ran as root you should edit the file in /root/.naga.

!!!!!!!!!!!!!!
If the `$HOME/.naga/keyMap.txt` file is missing the daemon won't start (the program will NOT autocreate this file, the install.sh script will copy an example file though).

For a key multiple actions may be defined. They will then be executed sequentially at the key press.

An example `keyMap.txt` configuration file is the following:

    #Comments should be accepted
    config=defaultConfig
    1 - key=XF86AudioPlay
    3 - chmap=420configEnemyBlazerWoW
    4 - run=notify-send 'Button # 4' 'Pressed'
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

Since autorun is a bit complicated for all the distros you can simply add nagastart.desktop or a script executing naga start to your startup folder/configuration.
(Might have to run chmod +x on the .desktop)

If you ever were to login as root then unlog and log as another user the daemon might ask you for your password to kill the rooted daemon. If the device still doesn't work after root access, unplugging and replugging the device then starting the daemon will surely work.

## Debugging

The commands are :

	`naga stop` or `naga kill` //stops the daemon.
	`naga start <optional:selectedMapping>` //restart the daemon if there is one running and starts a hidden daemon with the optional key mapping passed as the default config.
	`naga debug <optional:selectedMapping>` //restart the daemon if there is one running and starts one in the console for debugging with the optional key mapping passed as the default config.
	`naga uninstall` //uninstall the daemon tool
	`naga killroot` //kill root daemon if launched as root
	`naga` //gives help

For all the double dashed ocd people the commands also works with 2 dashes.

#### In depth


1) In order to get rid of the original bindings it disables the keypad using xinput as follows:

    $ xinput set-int-prop [id] "Device Enabled" 8 0

where [id] is the id number of the keypad returned by $ xinput.

2) You may have to also run

    $ xinput set-button-map [id2] 1 2 3 4 5 6 7 11 10 8 9 13 14 15

where [id2] is the id number of the pointer device returned by `xinput` - in case of naga 2014 you also have to check which of those two has more than 7 numbers by typing `xinput get-button-map [id2]`. Although this seems to be unnecessary in some systems (i.e CentOS 7)

## UNINSTALLATION

To uninstall you need to run `naga uninstall` .

