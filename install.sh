#!/bin/bash

cd src
g++ -O3 naga.cpp -o naga

if [ ! -f ./naga ]; then
echo "Error at compile! Aborting"
exit 1
fi

mv naga /usr/local/bin/naga
chmod u+s /usr/local/bin/naga

cd ..
cp naga.desktop $HOME/.config/autostart/naga.desktop

nohup bash nagastart.sh &
