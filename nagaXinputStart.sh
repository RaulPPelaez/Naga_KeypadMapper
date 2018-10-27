#!/bin/bash
if [[ $EUID -ne 0 && $(ps aux | grep naga | grep debug | grep -c root) -ge '1' ]]; then
	pkexec --user root naga -killroot
fi

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
