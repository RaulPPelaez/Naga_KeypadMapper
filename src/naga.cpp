/*
 * Apocatarsis 2016
 * Released with absolutely no warranty, use with your own responsibility.
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
#define OFFSET 263

using namespace std;


class NagaDaemon {
    struct input_event ev1[64], ev2[64];
    int id, side_btn_fd, extra_btn_fd, size;

    vector<vector<string>> args;
    vector<vector<int>> options;
    
    vector<pair<const char *,const char *>> devices;
public:
    NagaDaemon(int argc, char *argv[]) {
		devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Epic-if01-event-kbd",             "/dev/input/by-id/usb-Razer_Razer_Naga_Epic-event-mouse");              // NAGA EPIC
		devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Dock-if01-event-kbd",        "/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Dock-event-mouse");         // NAGA EPIC DOCK
		devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_2014-if02-event-kbd",             "/dev/input/by-id/usb-Razer_Razer_Naga_2014-event-mouse");              // NAGA 2014
		devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga-if01-event-kbd",                  "/dev/input/by-id/usb-Razer_Razer_Naga-event-mouse");                   // NAGA MOLTEN
		devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Chroma-if01-event-kbd",      "/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Chroma-event-mouse");       // NAGA EPIC CHROMA
		devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Chroma_Dock-if01-event-kbd", "/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Chroma_Dock-event-mouse");  // NAGA EPIC CHROMA DOCK
		devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Chroma-if02-event-kbd",           "/dev/input/by-id/usb-Razer_Razer_Naga_Chroma-event-mouse");            // NAGA CHROMA
        
        size = sizeof(struct input_event);
        //Setup check
        for (auto &device : devices) {
        	if ((side_btn_fd = open(device.first, O_RDONLY)) != -1 &&  (extra_btn_fd = open(device.second, O_RDONLY)) != -1) {
				cout << "Reading from: " << device.first << " and " << device.second << endl;
				break;
			}
        }
        if (side_btn_fd == -1 || extra_btn_fd == -1) {
        	cerr << "No naga devices found or you don't have permission to access them." << endl;
        	exit(1);
        }
        //Initialize config
        this->loadConf("mapping_01.txt");
    }

    void loadConf(string filename) {
        args.clear();
        options.clear();
        args.resize(DEV_NUM_KEYS + EXTRA_BUTTONS);
        options.resize(DEV_NUM_KEYS + EXTRA_BUTTONS);

        string conf_file = string(getenv("HOME")) + "/.naga/" + filename;
        ifstream in(conf_file.c_str(), ios::in);
        if (!in) {
            cerr << "Cannot open " << conf_file << ". Exiting." << endl;
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
            pos = stoi(token1) - 1;
            if (token2 == "chmap") options[pos].push_back(0);
            else if (token2 == "key") options[pos].push_back(1);
            else if (token2 == "run") options[pos].push_back(2);
            else if (token2 == "click") options[pos].push_back(3);
            else if (token2 == "workspace_r") options[pos].push_back(4);
            else if (token2 == "workspace") options[pos].push_back(5);
            else if (token2 == "position") {
                options[pos].push_back(6);
                std::replace(line.begin(), line.end(), ',', ' ');
            }
            else if (token2 == "delay") options[pos].push_back(7);
            else if (token2 == "media") options[pos].push_back(8);
            else {
                cerr << "Not supported key action, check the syntax in " << conf_file << ". Exiting!" << endl;
                exit(1);
            }
            //cerr << "b) len: " << len << " pos: " << pos << " line: " << line << " args[pos] size:" << args[pos].size() << "\n";
            args[pos].push_back(line);
        }
        in.close();
    }

    void run() {
        int rd, rd1, rd2;
        fd_set readset;
	
	// Give application exclusive control over side buttons.
	ioctl(side_btn_fd, EVIOCGRAB, 1);

        while (1) {
            FD_ZERO(&readset);
            FD_SET(side_btn_fd, &readset);
            FD_SET(extra_btn_fd, &readset);
            rd = select(FD_SETSIZE, &readset, NULL, NULL, NULL);
            if (rd == -1) exit(2);

            if (FD_ISSET(side_btn_fd, &readset)) // Side buttons
            {
                rd1 = read(side_btn_fd, ev1, size * 64);
                if (rd1 == -1) exit(2);

                if (ev1[0].value != ' ' && ev1[1].value == 1 && ev1[1].type == 1)  // Only read the key press event
                    switch (ev1[1].code) {
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
                            chooseAction(ev1[1].code - 2);
                            break;
                            // do nothing on default
                    }

            }
            else // Extra buttons
            {
                rd2 = read(extra_btn_fd, ev2, size * 64);
                if (rd2 == -1) exit(2);

                if (ev2[1].type == 1 && ev2[1].value == 1) //Only extra buttons
                    switch (ev2[1].code) {
                        case 275:
                        case 276:
                            chooseAction(ev2[1].code - OFFSET);
                            break;
                            // do nothing on default
                    }

            }
        }
    }

    void chooseAction(int i) {
        const string keyop = "xdotool key --window getactivewindow ";
        const string clickop = "xdotool click ";
        const string workspace_r = "xdotool set_desktop --relative -- ";
        const string workspace = "xdotool set_desktop ";
        const string position = "xdotool mousemove ";

        int pid;
        unsigned int delay;
        string command;
        bool execution;
        for (unsigned int j = 0; j < options[i].size(); j++) {
            //cerr << "key: " << i << " action: " << j << " args: " << args[i][j] << "\n" ;
            execution = true;
            switch (options[i][j]) {
                case 0: //switch mapping
                    this->loadConf(args[i][j]);
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
                    delay = stoi(args[i][j]) * 1000;
                    usleep(delay);
                    execution = false;
                    break;
                case 8: //media options
                    command = "xdotool key XF86" + args[i][j] + " ";
                    break;
            }
            if (execution)
                pid = system(command.c_str());
        }
    }
};


int main(int argc, char *argv[]) {
    NagaDaemon daemon(argc, argv);
    clog << "Starting naga daemon" << endl;
    daemon.run();

    return 0;
}
