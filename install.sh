#!/bin/bash
if [ "$(whoami)" != "root" ]; then
	echo "You need to be root! Aborting"
	exit 1
fi
command -v xdotool >/dev/null 2>&1 || { echo >&2 "I require xdotool but it's not installed!  Aborting."; exit 1; }
cd src
g++ -O3 -std=c++11 naga.cpp -o naga

if [ ! -f ./naga ]; then
echo "Error at compile! Aborting"
exit 1
fi

mv naga /usr/local/bin/
sudo chmod 755 /usr/local/bin/naga
cd ..
cp naga.desktop $HOME/.config/autostart/
cp nagastart.sh /usr/local/bin/
sudo chmod 755 /usr/local/bin/nagastart.sh
mkdir -p $HOME/.naga
cp mapping_{01,02}.txt $HOME/.naga/

echo 'KERNEL=="event[0-9]*",SUBSYSTEM=="input",GROUP="razer",MODE="640"' > /etc/udev/rules.d/80-naga.rules
groupadd -f razer
gpasswd -a $SUDO_USER razer

nohup sudo -u $SUDO_USER nagastart.sh & >/dev/null
sleep 5
rm nohup.out
