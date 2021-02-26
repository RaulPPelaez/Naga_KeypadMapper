#!/bin/bash
if [[ $EUID -ne 0 && $(ps aux | grep naga | grep debug | grep -c root) -ge '1' ]]; then
	pkexec --user root naga killroot
fi
