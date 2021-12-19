#!/bin/bash
sudo naga stop
sleep 0.3 > /dev/null 2>&1 &
echo "Deleting files"
sudo rm -f /usr/local/bin/naga
sudo rm -f /usr/local/bin/nagaXinputStart.sh
sudo rm -f /etc/udev/rules.d/80-naga.rules
sudo rm -f /usr/local/bin/nagaUninstall.sh
