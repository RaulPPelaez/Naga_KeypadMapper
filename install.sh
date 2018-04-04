#!/bin/bash
echo "--------------------------------------\nOnly run with root (or sudo su) if you want to use it on the root account.\nChanging user will need to either copy the .naga folder\nor reinstall on their account.\n--------------------------------------"
sudo killall naga
echo "Checking requirements..."
# Xdotool installed check
command -v xdotool >/dev/null 2>&1 || { echo >&2 "I require xdotool but it's not installed! Aborting."; exit 1; }

echo "Compiling code..."
# Compilation
cd src
g++ -O3 -std=c++11 naga.cpp -o naga

if [ ! -f ./naga ]; then
	echo "Error at compile! Ensure you have gcc installed Aborting"
	exit 1
fi

echo "Create config files"
# Configuration
sudo mv naga /usr/local/bin/
sudo chmod 755 /usr/local/bin/naga

cd ..

sudo cp nagastart.sh /usr/local/bin/
sudo chmod 755 /usr/local/bin/nagastart.sh

echo "Please add (naga.desktop OR nagastart.sh) to be executed when your window manager starts."

mkdir -p ~/.naga
cp -n mapping_*.txt ~/.naga/
sudo chown -R $(whoami):$(id -gn $(whoami)) ~/.naga/

echo "Creating udev rule..."

echo 'KERNEL=="event[0-9]*",SUBSYSTEM=="input",GROUP="razer",MODE="640"' > /tmp/80-naga.rules

sudo mv /tmp/80-naga.rules /etc/udev/rules.d/80-naga.rules
sudo groupadd -f razer
sudo gpasswd -a "$(whoami)" razer

echo "Starting daemon..."
# Run
nohup sh /usr/local/bin/nagastart.sh >/dev/null 2>&1 &
