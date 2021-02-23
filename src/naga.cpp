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
* this stuff is worth it, you can hand me a legal canadian joint in return.  lostallmymoney 2018
* ----------------------------------------------------------------------------
* This is lostallmymoney's branch of RaulPPelaez's original tool.
* Modifying a lot of stuff so it might never merge with master.
*/

#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <cstring>
#define OFFSET 262
using namespace std;

class configKey {
private:
  string code, content;
  bool internal, onKeyPressed;
public:
  configKey(string tcode, string tcontent, bool tinternal = false, bool tonKeyPressed = true){
    code = tcode;
    content = tcontent;
    internal = tinternal;
    onKeyPressed = tonKeyPressed;
  }
  string getCode(){
    return code;
  }
  string getContent(){
    return content;
  }
  bool getInternal(){
    return internal;
  }
  bool getOnKeyPressed(){
    return onKeyPressed;
  }
  void execute(string command, bool pressed){
    clog << "Running command : " << code << " for " << command << endl;
    int pid = system((content+command).c_str());
  }
};

class macroEvent{
private:
  int button;
  string type, content;
public:
  macroEvent(int tbutton, string ttype, string  tcontent){
    button = tbutton;
    type = ttype;
    content = tcontent;
  }
  int getButton(){
    return button;
  }
  string getType(){
    return type;
  }
  string getContent(){
    return content;
  }
};

class NagaDaemon {
private:
  std::vector<configKey *> configKeys;
  std::vector<macroEvent *> macroEvents;
  struct input_event ev1[64];
  int id, side_btn_fd, extra_btn_fd, size;
  vector<pair<const char *,const char *>> devices;
  const string conf_file = string(getenv("HOME")) + "/.naga/keyMap.txt";
public:
  NagaDaemon(int argc, char *argv[]) {

    //modulable device files list
    devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Epic-if01-event-kbd", "/dev/input/by-id/usb-Razer_Razer_Naga_Epic-event-mouse");                                  // NAGA EPIC
    devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Dock-if01-event-kbd", "/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Dock-event-mouse");                        // NAGA EPIC DOCK
    devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_2014-if02-event-kbd", "/dev/input/by-id/usb-Razer_Razer_Naga_2014-event-mouse");                                  // NAGA 2014
    devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga-if01-event-kbd", "/dev/input/by-id/usb-Razer_Razer_Naga-event-mouse");                                            // NAGA MOLTEN
    devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Chroma-if01-event-kbd", "/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Chroma-event-mouse");                    // NAGA EPIC CHROMA
    devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Chroma_Dock-if01-event-kbd", "/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Chroma_Dock-event-mouse");          // NAGA EPIC CHROMA DOCK
    devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Chroma-if02-event-kbd", "/dev/input/by-id/usb-Razer_Razer_Naga_Chroma-event-mouse");                              // NAGA CHROMA
    devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Hex-if01-event-kbd", "/dev/input/by-id/usb-Razer_Razer_Naga_Hex-event-mouse");                                    // NAGA HEXdevices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Hex_V2-if02-event-kbd",
    devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Hex_V2-if02-event-kbd","/dev/input/by-id/usb-Razer_Razer_Naga_Hex_V2-event-mouse");                               // NAGA HEX v2
    devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Trinity_00000000001A-if02-event-kbd", "/dev/input/by-id/usb-Razer_Razer_Naga_Trinity_00000000001A-event-mouse");  // Naga Trinity

    //modulable options list
    configKeys.emplace_back(new configKey("chmap", "", true, true)); //manage internals inside chooseAction method
    configKeys.emplace_back(new configKey("run", "setsid ", false, true));
    configKeys.emplace_back(new configKey("run2", "", false, true));
    configKeys.emplace_back(new configKey("runRelease", "setsid ", false, false));
    configKeys.emplace_back(new configKey("runRelease2", "", false, false));
    configKeys.emplace_back(new configKey("key", "setsid xdotool keydown --window getactivewindow ", false, true));
    configKeys.emplace_back(new configKey("key", "setsid xdotool keyup --window getactivewindow ", false, false));
    configKeys.emplace_back(new configKey("keyClick", "setsid xdotool key --window getactivewindow ", false, true));
    configKeys.emplace_back(new configKey("keyClickRelease", "setsid xdotool key --window getactivewindow ", false, false));
    configKeys.emplace_back(new configKey("mousePosition", "setsid xdotool mousemove ", false, true));
    configKeys.emplace_back(new configKey("mouseClick", "setsid xdotool click ", false, true));
    configKeys.emplace_back(new configKey("setWorkspace", "setsid xdotool set_desktop ", false, true));
    configKeys.emplace_back(new configKey("mouseClick", "setsid xdotool click ", false, true));

    size = sizeof(struct input_event);
    for (auto &device : devices) { //Setup check
      if ((side_btn_fd = open(device.first, O_RDONLY)) != -1 &&  (extra_btn_fd = open(device.second, O_RDONLY)) != -1) {
        clog << "Reading from: " << device.first << " and " << device.second << endl;
        break;
      }
    }
    if (side_btn_fd == -1 || extra_btn_fd == -1) {
      cerr << "No naga devices found or you don't have permission to access them." << endl;
      exit(1);
    }
    this->loadConf("defaultConfig");//Initialize config
  }

  void loadConf(string configName) {
    for ( int oo = 0; oo < macroEvents.size(); oo++){
      delete macroEvents[oo];
    }
    macroEvents.clear();
    macroEvents.resize(0);
    ifstream in(conf_file.c_str(), ios::in);
    if (!in) {
      cerr << "Cannot open " << conf_file << ". Exiting." << endl;
      exit(1);
    }
    bool found1 = false, found2 = false;
    string line, line1, token1;
    int pos, configLine, configEndLine;
    for (int readingLine = 1; getline(in, line) && !found2; readingLine++) {
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
    for (int readingLine = 1; getline(in, line) && readingLine<configEndLine; readingLine++){
      if (readingLine>configLine) //&& readingLine<configEndLine in the while
      {
        if (line[0] == '#' || line.find_first_not_of(' ') == std::string::npos) continue; //Ignore comments, empty lines, config= and configEnd
        pos = line.find('=');
        line1 = line.substr(0, pos); //line1 = numbers and stuff
        line.erase(0, pos+1); //line = command
        line1.erase(std::remove(line1.begin(), line1.end(), ' '), line1.end()); //Erase spaces inside 1st part of the line
        pos = line1.find("-");
        token1 = line1.substr(0, pos); //Isolate command type
        line1 = line1.substr(pos + 1);
        macroEvents.emplace_back(new macroEvent(stoi(token1), line1, line));//Encode and store mapping v2
      }
    }
    in.close();
  }

  void run() {
    int rd1;
    fd_set readset;
    ioctl(side_btn_fd, EVIOCGRAB, 1);// Give application exclusive control over side buttons.
    //ioctl(extra_btn_fd, EVIOCGRAB, 1); //stackoverflow please help
    while (1) {
      FD_ZERO(&readset);
      FD_SET(side_btn_fd, &readset);
      FD_SET(extra_btn_fd, &readset);
      rd1 = select(FD_SETSIZE, &readset, NULL, NULL, NULL);
      if (rd1 == -1) exit(2);
      if (FD_ISSET(side_btn_fd, &readset)) // Side buttons
      {
        rd1 = read(side_btn_fd, ev1, size * 64);
        if (rd1 == -1) exit(2);
        if (ev1[0].value != ' ' && ev1[1].type == EV_KEY)  //Key event (press or release)
        switch (ev1[1].code) {
          case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10:  case 11:  case 12:  case 13:
            chooseAction(ev1[1].code - 1, ev1[1].value); //ev1[1].value holds 1 if press event and 0 if release
          break;
        }
      }
      if (FD_ISSET(extra_btn_fd, &readset))// Extra buttons
      {
        rd1 = read(extra_btn_fd, ev1, size * 64);
        if (rd1 == -1) exit(2);
        if (ev1[1].type == 1){ //Only extra buttons
          switch (ev1[1].code) {
            case 275: case 276:
            chooseAction(ev1[1].code - OFFSET, ev1[1].value);
            break;
          }
        }
      }
    }
  }

  void chooseAction(int realKeyNb, int eventCode) {
    if(eventCode>1) return; //Only accept press or release events 1 for press 0 for release
    bool realKeyIsPressed = eventCode == 1;
    for (int ii = 0; ii < macroEvents.size(); ii++){ //looking for a match in keyMacros
      if(macroEvents[ii]->getButton() == realKeyNb){
        clog << "Type of action is : " << macroEvents[ii]->getType() << " & the cmd is : " << macroEvents[ii]->getContent() << endl;
        for (int ee = 0; ee < configKeys.size(); ee++){ //looking for a match in keyConfigs
          if(configKeys[ee]->getCode() == macroEvents[ii]->getType() && !configKeys[ee]->getInternal() && configKeys[ee]->getOnKeyPressed()==realKeyIsPressed){
            configKeys[ee]->execute(macroEvents[ii]->getContent(), realKeyIsPressed);//runs the Command
          } else if (configKeys[ee]->getCode() == macroEvents[ii]->getType() && configKeys[ee]->getInternal() && configKeys[ee]->getOnKeyPressed()==realKeyIsPressed){
            if(macroEvents[ii]->getType() == "chmap"){
              clog << "Switching config to : " << macroEvents[ii]->getContent() << endl;
              this->loadConf(macroEvents[ii]->getContent());//change config for macroEvents[ii]->getContent()
            }//else if(macroEvents[ii]->getType() == ""){} <---add other internal commands here (can only run one per button tho) you can also use configKeys[ee]->getContent() to get content/commands from internal operator
            ii=macroEvents.size();
            ee=configKeys.size();
          }
        }
      }
    }
  }
};

void stopD() {
  clog << "Stopping possible naga daemon" << endl;
  int pid = system(("nohup kill $(ps aux | grep naga | grep debug | grep -v "+ std::to_string((int)getpid()) +" | awk '{print $2}') > /dev/null 2>&1 &").c_str());
};

void stopDroot() {
  int pid = system(("nohup kill $(ps aux | grep naga | grep debug | grep root | grep -v "+ std::to_string((int)getpid()) +" | awk '{print $2}') > /dev/null 2>&1 &").c_str());
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
    }else if(strcmp(argv[1], "-kill")==0 || strcmp(argv[1], "--kill")==0 || strcmp(argv[1], "--stop")==0 || strcmp(argv[1], "-stop")==0){
      stopD();
    }else if(strcmp(argv[1], "-killroot")==0 || strcmp(argv[1], "--killroot")==0){
      stopDroot();
    }else if(strcmp(argv[1], "-uninstall")==0 || strcmp(argv[1], "--uninstall")==0){
      string answer;
      clog << "Are you sure you want to uninstall ? y/n" << endl;
      cin >> answer;
      if(answer.length() != 1 || ( answer[0] != 'y' &&  answer[0] != 'Y' )){
        clog << "Aborting" << endl;
      }else{
        int pid = system("bash /usr/local/bin/nagaUninstall.sh");
      }
    }else if(strcmp(argv[1], "-debug")==0 || strcmp(argv[1], "--debug")==0){
      stopD();
      usleep(40000);
      clog << "Starting naga daemon in debug mode..." << endl;
      xinputStart();
      NagaDaemon daemon(argc, argv);
      daemon.run();
    }
  } else {
    clog << "Possible arguments : " << endl << "  -start          Starts the daemon in hidden mode. (stops it before)" << endl << "  -stop           Stops the daemon." << endl << "  -debug          Starts the daemon in the terminal," << endl << "                      --giving access to logs. (stops it before)" << endl << "  -killroot       Stops the rooted daemon if ran as root." << endl;
  }
  return 0;
}
