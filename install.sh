#!/bin/bash
if [ "$(whoami)" != "root" ]; then
	echo "You need to be root! Aborting"
	exit 1
fi
cd src
g++ -O3 naga.cpp -o naga

if [ ! -f ./naga ]; then
echo "Error at compile! Aborting"
exit 1
fi

mv naga /usr/local/bin/
chmod u+s /usr/local/bin/

cd ..
cp naga.desktop $HOME/.config/autostart/
mkdir $HOME/.naga
cp mapping.txt $HOME/.naga/

nohup bash nagastart.sh &
