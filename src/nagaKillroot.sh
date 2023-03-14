#!/bin/bash
if [ "$(id -u)" -ne 0 ];then
	if [ $# -eq 0 ]; then
		kill $(ps aux | grep naga | grep -v root | grep start | awk '{print $2}') > /dev/null 2>&1
		if [ $(ps aux | grep naga | grep -v grep | grep -v nagaKillroot | grep -c root) -ne 0 ]; then
			pkexec --user root kill $(ps aux | grep naga | grep -v grep | grep -v nagaKillroot | grep root | awk '{print $2}')
		fi
	else
		kill $(ps aux | grep naga | grep -v root | grep start | grep -v $1 | awk '{print $2}') > /dev/null 2>&1
		if [ $(ps aux | grep naga | grep -v grep | grep -v nagaKillroot | grep -v $1 | grep -c root) -ne 0 ]; then
			pkexec --user root kill $(ps aux | grep naga | grep -v grep | grep -v nagaKillroot | grep -v $1 | grep root | awk '{print $2}')
		fi
	fi	
else
	if [ $# -eq 0 ]; then
		kill $(ps aux | grep naga | grep -v root | grep start | awk '{print $2}') > /dev/null 2>&1
		if [ $(ps aux | grep naga | grep -v grep | grep -v nagaKillroot | grep -c root) -ne 0 ]; then
			kill $(ps aux | grep naga | grep -v grep | grep -v nagaKillRoot | grep root | awk '{print $2}')
		fi
	else
		kill $(ps aux | grep naga | grep -v root | grep start | grep -v $1 | awk '{print $2}') > /dev/null 2>&1
		if [ $(ps aux | grep naga | grep -v grep | grep -v nagaKillroot | grep -v $1 | grep -c root) -ne 0 ]; then
			kill $(ps aux | grep naga | grep -v grep | grep -v nagaKillroot | grep -v $1 | grep root | awk '{print $2}')
		fi
	fi
fi
