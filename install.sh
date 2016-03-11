#!/bin/bash
# Root access check
if [ "$(whoami)" != "root" ]; then
	echo "You need to be root! Aborting"
	exit 1
fi
# Xdotool installed check
command -v xdotool >/dev/null 2>&1 || { echo >&2 "I require xdotool but it's not installed! Aborting."; exit 1; }

# Compilation
cd src
g++ -O3 -std=c++11 naga.cpp -o naga

if [ ! -f ./naga ]; then
	echo "Error at compile! Aborting"
	exit 1
fi

# Configuration
mv naga /usr/local/bin/
chmod 755 /usr/local/bin/naga

cd ..
HOME=$( getent passwd "$SUDO_USER" | cut -d: -f6 )

cp nagastart.sh /usr/local/bin/
chmod 755 /usr/local/bin/nagastart.sh

#cp naga.desktop "$HOME"/.config/autostart/
if ! grep -Fxq "bash /usr/local/bin/nagastart.sh &" "$HOME"/.profile; then
	echo "bash /usr/local/bin/nagastart.sh &" >> "$HOME"/.profile
fi

mkdir -p "$HOME"/.naga
cp mapping_{01,02,03}.txt "$HOME"/.naga/
chown -R ${SUDO_USER}:${SUDO_USER} "$HOME"/.naga/

echo 'KERNEL=="event[0-9]*",SUBSYSTEM=="input",GROUP="razer",MODE="640"' > /etc/udev/rules.d/80-naga.rules
groupadd -f razer
gpasswd -a "$SUDO_USER" razer

# Run
nohup sudo -u $SUDO_USER nagastart.sh >/dev/null 2>&1 &
