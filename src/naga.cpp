/*
 * Apocatarsis 2015
 * Released with absolutely no warranty, use with your own responsability.
 * 
 * This version is still in development
 */
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/select.h>

#define DEV_NUM_KEYS 12
#define EXTRA_BUTTONS 2

using namespace std;

const char * devices[][2] = {
		{"/dev/input/by-id/usb-Razer_Razer_Naga_Epic-if01-event-kbd","/dev/input/by-id/usb-Razer_Razer_Naga_Epic-event-mouse"}, // NAGA EPIC
		{"/dev/input/by-id/usb-Razer_Razer_Naga_2014-if02-event-kbd","/dev/input/by-id/usb-Razer_Razer_Naga_2014-event-mouse"}, // NAGA 2014
		{"/dev/input/by-id/usb-Razer_Razer_Naga-if01-event-kbd","/dev/input/by-id/usb-Razer_Razer_Naga-event-mouse"} // NAGA MOLTEN
};

class NagaDaemon {
	struct input_event ev1[64], ev2[64];
	int id, side_btn_fd, extra_btn_fd, rd, rd1,rd2, value, size;
	vector<string> args;
	vector<int> options;
	
	string keyop = "xdotool key --window getactivewindow ";
	string clickop = "xdotool click --window getactivewindow ";
	string workspace_r = "xdotool set_desktop --relative -- ";
	string workspace = "xdotool set_desktop ";
	string command;


public:
	NagaDaemon(int argc, char *argv[]) 
	{
		size = sizeof (struct input_event);
		//Initialize config
		this->load_conf("mapping_01.txt");
		//Setup check
		if (argv[1] == NULL) 
		{
			printf("Please specify if you're using Naga Epic or Naga 2014.\n");
			exit(0);
		}
		if ((string)argv[1] == "epic")
			id = 0;
		else if ((string)argv[1] =="2014")
			id = 1;
		else if ((string)argv[1] == "molten")
			id = 2;
		else
		{
			printf("Not valid device. Exiting.\n");
			exit(1);
		}
			
		
		//Open Devices
		if ((side_btn_fd = open(devices[id][0], O_RDONLY)) == -1)
		{
			printf("%s is not a vaild device or you don't have the permission to access it.\n", devices[id][0]);
			exit(1);
		}
		if ((extra_btn_fd = open(devices[id][1],O_RDONLY)) == -1)
		{
			printf("%s is not a vaild device or you don't have the permission to access it.\n", devices[id][1]);
			exit(1);
		}
		//Print Device Name
		printf("Reading From : %s and %s\n", devices[id][0], devices[id][1]);
	}

	void load_conf(string path) 
	{
		args.resize(DEV_NUM_KEYS+EXTRA_BUTTONS);
		options.resize(DEV_NUM_KEYS+EXTRA_BUTTONS);

		string conf_file = string(getenv("HOME")) + "/.naga/" + path;
		ifstream in(conf_file.c_str(), ios::in);
		if (!in) {
			cerr << "Cannot open " << conf_file << endl;
			exit(1);
		}
		
		string line, token1, token2;
		int pos;
		while (getline(in, line)) {
			//Erase spaces
			line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
			//Ignore comments
			if (line[0] == '#') 
				continue;
			//Search option and argument
			pos = line.find("-");
			token1 = line.substr(0, pos);
			line = line.substr(pos + 1);
			pos = line.find("=");
			token2 = line.substr(0, pos);
			line = line.substr(pos + 1);
			//Encode and store mapping
			pos = atoi(token1.c_str()) - 1;
			if (token2 == "chmap")options[pos] = 0;
			else if (token2 == "key")options[pos] = 1;
			else if (token2 == "run")options[pos] = 2;
			else if (token2 == "click")options[pos] = 3;
			else if (token2 == "workspace_r")options[pos] = 4;
			else if (token2 == "workspace")options[pos] = 5;
			else printf("Not supported key action, check the syntax in mapping_01.txt!\n");
			args[pos] = line;
			if (pos == DEV_NUM_KEYS+EXTRA_BUTTONS) 
				break; //Only 12 keys for the Naga + 2 buttons on the top
		}
	}

	void run() 
	{
		fd_set readset;		
		
		while (1) 
		{
			FD_ZERO(&readset);
			FD_SET(side_btn_fd, &readset );
			FD_SET(extra_btn_fd, &readset);
			rd = select(FD_SETSIZE , &readset, NULL, NULL, NULL);
			if (rd == -1) exit(2);

			if(FD_ISSET(side_btn_fd,&readset)) // Side buttons
			{
				rd1 = read(side_btn_fd, ev1, size * 64);
				if (rd1 == -1) exit(2);

				if (ev1[0].value != ' ' && ev1[1].value == 1 && ev1[1].type == 1)  // Only read the key press event
					for (int i = 0; i < DEV_NUM_KEYS; i++)//For all keys
						if (ev1[1].code == (i + 2)) //If code i+2 is on (Only for naga)
							chooseAction(i);
			}
			else // Extra buttons
			{
				rd2 = read(extra_btn_fd, ev2, size * 64);
				if (rd2 == -1) exit(2);

				if ((ev2[1].code == 275 || ev2[1].code == 276) && ev2[1].value == 1 ) //Only extra buttons
					for(int i = DEV_NUM_KEYS; i < DEV_NUM_KEYS+EXTRA_BUTTONS;i++)
						if(ev2[1].code == i+263) // Only line 13 and 14
							chooseAction(i);
			}
		}
	}
	
	void chooseAction(int i) 
	{
		int pid;
		switch (options[i]) {
			case 0: //switch mapping
				this->load_conf(args[i]);
				break;
			case 1: //key
				command = keyop + args[i];
				break;
			case 2: //run system command
				command = "setsid " + args[i] + " &";
				break;
			case 3: //click
				command = clickop + args[i];
				break;
			case 4: //move to workspace(relative)
				command = workspace_r + args[i];
				break;
			case 5: //move to workspace(absolute)
				command = workspace + args[i];
				break;
		}
		if(options[i])
			pid = system(command.c_str());
		}
};


int main(int argc, char *argv[]) {
	NagaDaemon daemon(argc, argv);
	daemon.run();

	return 0;
} 
