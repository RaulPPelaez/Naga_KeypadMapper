#!/bin/bash
sudo naga stop
sleep 0.3 > /dev/null 2>&1 &
echo "Deleting files"
sudo rm -f /usr/local/bin/naga*
sudo rm -f /etc/udev/rules.d/80-naga.rules

