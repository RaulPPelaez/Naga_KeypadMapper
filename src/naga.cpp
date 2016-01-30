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
	int id, side_btn_fd, extra_btn_fd, size;
	vector<vector<string>> args;
	vector<vector<int>> options;

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
			printf("Not a valid device. Exiting.\n");
			exit(1);
		}
			
		
		//Open Devices
		if ((side_btn_fd = open(devices[id][0], O_RDONLY)) == -1)
		{
			printf("%s is not a valid device or you don't have the permission to access it.\n", devices[id][0]);
			exit(1);
		}
		if ((extra_btn_fd = open(devices[id][1],O_RDONLY)) == -1)
		{
			printf("%s is not a valid device or you don't have the permission to access it.\n", devices[id][1]);
			exit(1);
		}
		//Print Device Name
		printf("Reading From : %s and %s\n", devices[id][0], devices[id][1]);
	}

	void load_conf(string path) 
	{
		args.clear();
		options.clear();
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
			if (token2 == "chmap") options[pos].push_back( 0);
			else if (token2 == "key") options[pos].push_back( 1);
			else if (token2 == "run") options[pos].push_back( 2);
			else if (token2 == "click") options[pos].push_back( 3);
			else if (token2 == "workspace_r") options[pos].push_back( 4);
			else if (token2 == "workspace") options[pos].push_back( 5);
			else if (token2 == "position") {
				options[pos].push_back( 6);
				std::replace(line.begin(),line.end(), ',', ' ');
			}
			else if (token2 == "delay") options[pos].push_back( 7);
			else printf("Not supported key action, check the syntax in mapping_01.txt!\n");
			//cerr << "b) len: " << len << " pos: " << pos << " line: " << line << " args[pos] size:" << args[pos].size() << "\n"; 
			args[pos].push_back(line);
			if (pos == DEV_NUM_KEYS+EXTRA_BUTTONS-1) 
				break; //Only 12 keys for the Naga + 2 buttons on the top
		}
	}

	void run() 
	{
		int rd, rd1, rd2;
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
					switch(ev1[1].code)
					{
						case 2:
						case 3:
						case 4:
						case 5:
						case 6:
						case 7:
						case 8:
						case 9:
						case 10:
						case 11:
						case 12:
						case 13:
							chooseAction(ev1[1].code-2);
							break;
						// do nothing on default
					}
			
			}
			else // Extra buttons
			{
				rd2 = read(extra_btn_fd, ev2, size * 64);
				if (rd2 == -1) exit(2);

				if (ev2[1].type == 1 && ev2[1].value == 1 ) //Only extra buttons
					switch(ev2[1].code)
					{
						case 275:
						case 276:
							chooseAction(ev2[1].code-263);
							break;
						// do nothing on default
					}

			}
		}
	}
	
	void chooseAction(int i)
	{
		const string keyop = "xdotool key --window getactivewindow ";
		const string clickop = "xdotool click --window getactivewindow ";
		const string workspace_r = "xdotool set_desktop --relative -- ";
		const string workspace = "xdotool set_desktop ";
		const string position = "xdotool mousemove ";

		int pid;
		unsigned int delay;
		string command;
		bool execution;
		for (int j = 0; j < options[i].size(); j++){
			//cerr << "key: " << i << " action: " << j << " args: " << args[i][j] << "\n" ;
			execution = true;
			switch (options[i][j]) {
				case 0: //switch mapping
					this->load_conf(args[i][j]);
					execution = false;
					break;
				case 1: //key
					command = keyop + args[i][j];
					break;
				case 2: //run system command
					command = "setsid " + args[i][j] + " &";
					break;
				case 3: //click
					command = clickop + args[i][j];
					break;
				case 4: //move to workspace(relative)
					command = workspace_r + args[i][j];
					break;
				case 5: //move to workspace(absolute)
					command = workspace + args[i][j];
					break;
				case 6: //move cursor to position
					command = position + args[i][j];
					break;
				case 7: //delay execution n milliseconds
					delay = atoi(args[i][j].c_str()) * 1000 ;
					usleep(delay);
					execution = false;
					break;

	
			}
			if(execution)
				pid = system(command.c_str());
		}
	}
};


int main(int argc, char *argv[]) {
	NagaDaemon daemon(argc, argv);
	cerr << "Start daemon";
	daemon.run();

	return 0;
}
