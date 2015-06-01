#!/bin/bash
if [ "$(whoami)" != "root" ]; then
	echo "You need to be root! Aborting"
	exit 1
fi
command -v xdotool >/dev/null 2>&1 || { echo >&2 "I require xdotool but it's not installed!  Aborting."; exit 1; }
cd src
g++ -O3 naga.cpp -o naga

if [ ! -f ./naga ]; then
echo "Error at compile! Aborting"
exit 1
fi

mv naga /usr/local/bin/
sudo chmod u+s /usr/local/bin/naga

cd ..
cp naga.desktop $HOME/.config/autostart/
cp nagastart.sh /usr/local/bin/
mkdir $HOME/.naga
cp mapping.txt $HOME/.naga/

nohup bash nagastart.sh & >/dev/null

rm nohup.out
