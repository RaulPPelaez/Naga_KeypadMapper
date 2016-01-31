#!/bin/bash
# Root access check
if [ "$(whoami)" != "root" ]; then
	echo "You need to be root! Aborting"
	exit 1
fi
# Xdotool installed check
command -v xdotool >/dev/null 2>&1 || { echo >&2 "I require xdotool but it's not installed! Aborting."; exit 1; }

# Naga detection
if [[ -e /dev/input/by-id/usb-Razer_Razer_Naga_Epic-if01-event-kbd ]]; then
	version=epic
elif [[ -e /dev/input/by-id/usb-Razer_Razer_Naga_2014-if02-event-kbd ]]; then
	version=2014
elif [[ -e /dev/input/by-id/usb-Razer_Razer_Naga-if01-event-kbd ]]; then
	version=molten
elif [[ -e /dev/input/by-id/usb-Razer_Razer_Naga_Epic_Chroma-if01-event-kbd ]]; then
	version=chroma
else 
	echo "Naga not connected or using unsupported model. Please check src/naga.cpp and nagastart.sh"
fi

# Compilation
cd src
g++ -O3 -std=c++11 naga.cpp -o naga

if [ ! -f ./naga ]; then
echo "Error at compile! Aborting"
exit 1
fi

# Configuration
mv naga /usr/local/bin/
sudo chmod 755 /usr/local/bin/naga

cd ..
cp naga.desktop $HOME/.config/autostart/
echo "naga $version" >> nagastart.sh
cp nagastart.sh /usr/local/bin/
sudo chmod 755 /usr/local/bin/nagastart.sh
mkdir -p $HOME/.naga
cp mapping_{01,02}.txt $HOME/.naga/

echo 'KERNEL=="event[0-9]*",SUBSYSTEM=="input",GROUP="razer",MODE="640"' > /etc/udev/rules.d/80-naga.rules
groupadd -f razer
gpasswd -a $SUDO_USER razer

# Run
nohup sudo -u $SUDO_USER nagastart.sh & >/dev/null
sleep 5
rm nohup.out
