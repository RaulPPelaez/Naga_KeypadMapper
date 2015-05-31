#!/bin/bash
#Only for naga in this moment! but you can easily change it or event manually put the id
NAGAID=$(xinput | grep Naga | grep keyboard | cut -d= -f2 | cut -b1-2)
xinput set-int-prop $NAGAID "Device Enabled" 8 0
naga /dev/input/by-id/usb-Razer_Razer_Naga_Epic-if01-event-kbd 
