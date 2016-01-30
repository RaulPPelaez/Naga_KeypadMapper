#!/bin/bash
# For Naga Epic, 2014 and Molten atm. If you wanna add another device, please check the naga.cpp source code!
NAGAID1=$(xinput | grep Naga | grep keyboard | cut -d= -f2 | cut -b1-2)
xinput set-int-prop $NAGAID1 "Device Enabled" 8 0
 
# Insanity, just because naga 2014
NAGAID2=$(xinput | grep Naga | grep pointer | cut -d= -f2 | cut -f1)
if [[ `echo $NAGAID2 | wc -w` -eq 2 ]]; then
if [[ `xinput get-button-map $(echo $NAGAID2 | awk '{print $1}') | grep 10 | wc -l` -eq 1 ]]; then
xinput set-button-map $(echo $NAGAID2 | awk '{print $1}') 1 2 3 4 5 6 7 11 10 8 9 13 14 15
else
xinput set-button-map $(echo $NAGAID2 | awk '{print $2}') 1 2 3 4 5 6 7 11 10 8 9 13 14 15
fi
else
xinput set-button-map $NAGAID2 1 2 3 4 5 6 7 11 10 8 9 13 14 15
fi

# If you didn't run the install.sh or you don't have supported model you have to call naga daemon here e.g.  naga epic


