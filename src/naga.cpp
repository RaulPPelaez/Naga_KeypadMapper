/* See https://github.com/RaulPPelaez/Naga_KeypadMapper/graphs/contributors
* for a full list of contributors.
* This program is free software. It comes without any warranty, to the extent
* permitted by applicable law. You can redistribute it and/or modify it under the
* terms of the Beer-ware license revision 42.
* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* RaulPPelaez, et. al wrote this file.  As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a beer in return. RaulPPelaez 2016
* ----------------------------------------------------------------------------
*/

/*See https://github.com/lostallmymoney/Razer_Key_Mapper_Linux/graphs/contributors
* for a full list of contributors of this branch.
* This program is free software. It comes without any warranty, to the extent
* permitted by applicable law. You can redistribute it and/or modify it under the
* terms of the Beer-ware license revision 420.
* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 420):
* RaulPPelaez et. al wrote this file and lostallmymoney made a branch. As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can gimme me a j in return.   lostallmymoney 2018
* ----------------------------------------------------------------------------
* This is lostallmymoney's branch of RaulPPelaez's original tool.
* Modifying a lot of stuff so it might never merge with master.
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
#include <cstring>
#define DEV_NUM_KEYS 12
#define EXTRA_BUTTONS 2
#define OFFSET 263
using namespace std;

class NagaDaemon {
  enum class Operators{chmap, key, run, run2, run3, run4, run5, run6, click, workspace, workspace_r, position, delay, toggle};
  struct input_event ev1[64], ev2[64];
  int id, side_btn_fd, extra_btn_fd, size;
  vector<vector<string>> args;
  vector<vector<Operators>> options;
  vector<vector<int>> state;
  vector<pair<const char *,const char *>> devices;
  const string conf_file = string(getenv("HOME")) + "/.naga/keyMap.txt";
  const string keydownop = "xdotool keydown --window getactivewindow ";
  const string keyupop = "xdotool keyup --window getactivewindow ";
  const string clickop = "xdotool click ";
  const string workspace_r = "xdotool set_desktop --relative -- ";
  const string workspace = "xdotool set_desktop ";
  const string position = "xdotool mousemove ";
public:
  NagaDaemon(int argc, char *argv[]) {
    devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Epic-if01-event-kbd",
    "/dev/input/by-id/usb-Razer_Razer_Naga_Epic-event-mouse");              // NAGA EPIC
    devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Dock-if01-event-kbd",
    "/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Dock-event-mouse");         // NAGA EPIC DOCK
    devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_2014-if02-event-kbd",
    "/dev/input/by-id/usb-Razer_Razer_Naga_2014-event-mouse");              // NAGA 2014
    devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga-if01-event-kbd",
    "/dev/input/by-id/usb-Razer_Razer_Naga-event-mouse");                   // NAGA MOLTEN
    devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Chroma-if01-event-kbd",
    "/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Chroma-event-mouse");       // NAGA EPIC CHROMA
    devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Chroma_Dock-if01-event-kbd",
    "/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Chroma_Dock-event-mouse");  // NAGA EPIC CHROMA DOCK
    devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Chroma-if02-event-kbd",
    "/dev/input/by-id/usb-Razer_Razer_Naga_Chroma-event-mouse");            // NAGA CHROMA
    devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Hex-if01-event-kbd",
    "/dev/input/by-id/usb-Razer_Razer_Naga_Hex-event-mouse");               // NAGA HEX

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
    this->loadConf("defaultConfig");
  }

  void loadConf(string configName) {
    args.clear();
    options.clear();
    state.clear();
    args.resize(DEV_NUM_KEYS + EXTRA_BUTTONS);
    options.resize(DEV_NUM_KEYS + EXTRA_BUTTONS);
    state.resize(DEV_NUM_KEYS + EXTRA_BUTTONS);
    ifstream in(conf_file.c_str(), ios::in);
    if (!in) {
      cerr << "Cannot open " << conf_file << ". Exiting." << endl;
      exit(1);
    }
    bool found1 = false, found2 = false;
    string line, line1, token1;
    int pos, configLine, readingLine=0, configEndLine;

    while (getline(in, line) && !found2) {
      readingLine++;

      if(!found1 && line.find("config="+configName) != string::npos) //finding configname
      {
        configLine=readingLine;
        found1=true;
        clog << "Found config start : "<< readingLine << endl;
      }

      if(found1 && line.find("configEnd") != string::npos)//finding configEnd
      {
        configEndLine=readingLine;
        found2=true;
        clog << "Found config end : "<< readingLine << endl;
      }
    }

    if (!found1 || !found2) {
      cerr << "Error with config names and configEnd : " << configName << ". Exiting." << endl;
      exit(1);
    }

    in.clear();
    in.seekg(0, ios::beg); //reset file reading

    readingLine=1;
    while (getline(in, line) && readingLine<configEndLine){
      if (readingLine>configLine) //&& readingLine<configEndLine in the while
      {
        if (line[0] == '#' || line.find_first_not_of(' ') == std::string::npos) continue; //Ignore comments, empty lines, config= and configEnd

        pos = line.find('=');
        line1 = line.substr(0, pos); //line1 = numbers and stuff

        line.erase(0, pos+1); //line = command
        line1.erase(std::remove(line1.begin(), line1.end(), ' '), line1.end()); //Erase spaces

        pos = line1.find("-");
        token1 = line1.substr(0, pos); //Isolate command type
        line1 = line1.substr(pos + 1);
        //Encode and store mapping
        pos = stoi(token1) - 1;
        if (line1 == "chmap") options[pos].push_back(Operators::chmap);
        else if (line1 == "key") options[pos].push_back(Operators::key);
        else if (line1 == "run") options[pos].push_back(Operators::run);
        else if (line1 == "run2") options[pos].push_back(Operators::run2);
        else if (line1 == "run3") options[pos].push_back(Operators::run3);
        else if (line1 == "run4") options[pos].push_back(Operators::run4);
        else if (line1 == "run5") options[pos].push_back(Operators::run5);
        else if (line1 == "run6") options[pos].push_back(Operators::run6);
        else if (line1 == "click") options[pos].push_back(Operators::click);
        else if (line1 == "workspace_r") options[pos].push_back(Operators::workspace_r);
        else if (line1 == "workspace") options[pos].push_back(Operators::workspace);
        else if (line1 == "position") {
          options[pos].push_back(Operators::position);
          std::replace(line.begin(), line.end(), ',', ' ');
        }
        else if (line1 == "delay") options[pos].push_back(Operators::delay);
        else if (line1 == "toggle") options[pos].push_back(Operators::toggle);
        else {
          cerr << "Not supported key action, check the syntax in " << conf_file << ". Exiting!" << endl;
          exit(1);
        }
        args[pos].push_back(line);
        state[pos].push_back(0); // Default state initialise
      }
      readingLine++;
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
        if (ev1[0].value != ' ' && ev1[1].type == EV_KEY)  //Key event (press or release)
        //clog << "ev1[1].code"<< ev1[1].code << endl;
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
          chooseAction(ev1[1].code - 2, ev1[1].value); //ev1[1].value holds 1 if press event and 0 if release
          break;  // do nothing on default
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
          chooseAction(ev2[1].code - OFFSET, 1);
          break;
        }
      }
    }
  }

  void chooseAction(int i, int eventCode) {
    if(eventCode>1) return; //Only accept press or release events 1 for press 0 for release
    int pid;
    unsigned int delay;
    string command;
    bool execution;
    for (unsigned int j = 0; j < options[i].size(); j++) {
      execution = true;
      switch (options[i][j]) {

        case Operators::chmap:
        this->loadConf(args[i][j]);
        execution = false;
        break;

        case Operators::key:
        if(eventCode==1)           command = keydownop + args[i][j];
        else if(eventCode==0)      command = keyupop + args[i][j];
        break;

        case Operators::run:
        command = "setsid " + args[i][j];
        if(eventCode==0) execution=false;
        break;

        case Operators::run2:
        command = "setsid " + args[i][j];
        break;

        case Operators::run3:
        command = args[i][j];
        if(eventCode==0) execution=false;
        break;

        case Operators::run4:
        command = args[i][j];
        break;

        case Operators::run5:
        command = "setsid " + args[i][j];
        if(eventCode==1) execution=false;
        break;

        case Operators::run6:
        command = args[i][j];
        if(eventCode==1) execution=false;
        break;

        case Operators::click:       command = clickop + args[i][j];         if(eventCode==0) execution=false; break;
        case Operators::workspace_r: command = workspace_r + args[i][j];	   if(eventCode==0) execution=false; break;
        case Operators::workspace:   command = workspace + args[i][j];	   if(eventCode==0) execution=false; break;
        case Operators::position:    command = position + args[i][j];        if(eventCode==0) execution=false; break;
        case Operators::delay: //delay execution n milliseconds

        delay = stoi(args[i][j]) * 1000;
        usleep(delay);
        execution = false;
        break;
        case Operators::toggle: // Toggle action
        if(eventCode==0){
          execution=false;
        }
        else{
          if (state[i][j] == 0) {
            command = keydownop + args[i][j];
            state[i][j] = 1;
          }
          else if (state[i][j] == 1) {
            command = keyupop + args[i][j];
            state[i][j] = 0;
          }
        }
        break;
        default: //never too safe
        execution=false;
        break;
      }
      if (execution){
        clog << "Command : " << command << endl;
        pid = system(command.c_str());
      }
    }
  }
};

void stopD() {
  clog << "Stopping possible naga daemon" << endl;
  int pid = system(("nohup kill $(ps aux | grep naga | grep -v Uninstall | grep -v grep | grep -v cpp | grep -v "+ std::to_string((int)getpid()) +" | awk '{print $2}') > /dev/null 2>&1 &").c_str());
};

void xinputStart(){
  int pid = system("/usr/local/bin/nagaXinputStart.sh");
};

int main(int argc, char *argv[]) {
  if(argc>1){
    if(strcmp(argv[1], "-start")==0 || strcmp(argv[1], "--start")==0){
      clog << "Starting naga daemon in hidden mode..." << endl;
      xinputStart();
      int pid = system("nohup naga -debug > /dev/null 2>&1 &");
      exit(0);
    }else if(strcmp(argv[1], "-kill")==0 || strcmp(argv[1], "--kill")==0 || strcmp(argv[1], "--stop")==0 || strcmp(argv[1], "-stop")==0){
      stopD();
      exit(0);
    }else if(strcmp(argv[1], "-uninstall")==0 || strcmp(argv[1], "--uninstall")==0){
      string answer;
      clog << "Are you sure you want to uninstall ? y/n" << endl;
      cin >> answer;
      if(answer.length() != 1 || answer[0] != 'y'){
        clog << "Aborting" << endl;
      }else{
        int pid = system("bash /usr/local/bin/nagaUninstall.sh");
      }
      exit(0);
    }else if(strcmp(argv[1], "-debug")==0 || strcmp(argv[1], "--debug")==0){
      stopD();
      usleep(40000);
      clog << "Starting naga daemon in debug mode..." << endl;
      xinputStart();
      NagaDaemon daemon(argc, argv);
      daemon.run();
      exit(0);
    }
  } else {
    clog << "Possible arguments : " << endl << "  -start          Starts the daemon in hidden mode. (stops it before)" << endl << "  -stop           Stops the daemon." << endl << "  -debug          Starts the daemon in the terminal," << endl << "                      --giving access to logs. (stops it before)" << endl;
  }
  return 0;
}
