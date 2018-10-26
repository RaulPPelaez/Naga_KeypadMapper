#!/bin/bash
if [[ $EUID -ne 0 && $(ps aux | grep naga | grep debug | grep -c root) -ge '1' ]]; then
	pkexec --user root naga -killroot
fi

NAGAID2=$(xinput | grep Naga | grep keyboard | cut -d= -f2 | cut -f1)
if [[ `echo $NAGAID2 | wc -w` -eq 2 ]]; then
	if [[ `xinput get-button-map $(echo $NAGAID2 | awk '{print $1}') | grep 10 | wc -l` -eq 1 ]]; then
		xinput set-int-prop $(echo $NAGAID2 | awk '{print $1}') "Device Enabled" 8 0
	else
		xinput set-int-prop $(echo $NAGAID2 | awk '{print $2}') "Device Enabled" 8 0
	fi
else
	xinput set-int-prop $NAGAID2 "Device Enabled" 8 0
fi
