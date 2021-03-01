//This is lostallmymoney's remake of RaulPPelaez's original tool.
//RaulPPelaez, et. al wrote the original file.  As long as you retain this notice you
//can do whatever you want with this stuff.

#include <iostream>
#include <vector>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <cstring>
#include <thread>
#include <map>
#include <sstream>
#define OFFSET 262
using namespace std;

class configKey {
private:
const string content;
const bool internal, onKeyPressed;
public:
const bool& IsOnKeyPressed() const {
	return onKeyPressed;
}
const bool& isInternal() const {
	return internal;
}
const void execute(string const& command) const {
	(void)!(system((content+command).c_str()));
}
configKey(string&& tcontent, bool tinternal, bool tonKeyPressed) : content(tcontent), internal(tinternal), onKeyPressed(tonKeyPressed){
}
};

class MacroEvent {
private:
const configKey * keyType;
const string type, content;
public:
const configKey * KeyType() const {
	return keyType;
}
const string& Type() const {
	return type;
}
const string& Content() const {
	return content;
}
const void execute() const {
	keyType->execute(content);
}
MacroEvent(configKey * tkeyType, string * ttype, string * tcontent) : keyType(tkeyType), type(*ttype), content(*tcontent){
}
};

class configSwitchScheduler {
private:
bool scheduledReMap = false;
string scheduledReMapString="";
public:
const string& RemapString() const {
	return scheduledReMapString;
}
const bool& isRemapScheduled() const {
	return scheduledReMap;
}
void scheduleReMap(string const& reMapString) {
	scheduledReMapString = reMapString;
	scheduledReMap = true;
}
void unScheduleReMap(){
	scheduledReMap=false;
}
};

class NagaDaemon {
private:
const string conf_file = string(getenv("HOME")) + "/.naga/keyMap.txt";

std::map<std::string, configKey*> configKeysMap;
std::map<std::string, std::map<int, std::vector<MacroEvent *> > > macroEventsKeyMap;
configSwitchScheduler configSwitcher = configSwitchScheduler();

string currentConfigName;
struct input_event ev1[64];
int side_btn_fd, extra_btn_fd, size;
vector<pair<const char *,const char *> > devices;

void loadConf(string configName) {
	if(!macroEventsKeyMap.contains(configName)) {
		ifstream in(conf_file.c_str(), ios::in);
		if (!in) {
			cerr << "Cannot open " << conf_file << ". Exiting." << endl;
			exit(1);
		}
		bool found1 = false, found2 = false;
		int pos, configLine, configEndLine;
		string commandContent;

		for (int readingLine = 1; getline(in, commandContent) && !found2; readingLine++) {
			if(!found1 && commandContent.find("config="+configName) != string::npos)//finding configname
			{
				configLine=readingLine;
				found1=true;
			}
			if(found1 && commandContent.find("configEnd") != string::npos)//finding configEnd
			{
				configEndLine=readingLine;
				found2=true;
			}
		}
		if (!found1 || !found2) {
			clog << "Error with config names and configEnd : " << configName << ". Exiting." << endl;
			exit(1);
		}
		in.clear();
		in.seekg(0, ios::beg);//reset file reading

		for (int readingLine = 1; getline(in, commandContent) && readingLine<configEndLine; readingLine++) {
			if (readingLine>configLine) {
				if (commandContent[0] == '#' || commandContent.find_first_not_of(' ') == std::string::npos) continue; //Ignore comments, empty lines
				pos = commandContent.find('=');
				string commandType = commandContent.substr(0, pos);//commandType = numbers + command type
				commandContent.erase(0, pos+1);//commandContent = command content
				commandType.erase(std::remove(commandType.begin(), commandType.end(), ' '), commandType.end());//Erase spaces inside 1st part of the line
				pos = commandType.find("-");
				string buttonNumber = commandType.substr(0, pos);//Isolate button number
				commandType = commandType.substr(pos + 1);//Isolate command type
				for(auto& c : commandType)
				{
					c = tolower(c);
				}

				if(configKeysMap.contains(commandType)) { //filter out bad types
					if(commandType=="key") {
						if(commandContent.size()==1) {
							commandContent = hexChar(commandContent[0]);
						}
						macroEventsKeyMap[configName][stoi(buttonNumber)].emplace_back(new MacroEvent(configKeysMap["keypressonpress"], &commandType, &commandContent));
						macroEventsKeyMap[configName][stoi(buttonNumber)].emplace_back(new MacroEvent(configKeysMap["keyreleaseonrelease"], &commandType, &commandContent));
					}else if(commandType=="string" || commandType=="stringrelease") {
						string commandContent2="";
						for(long unsigned jj=0; jj<commandContent.size(); jj++) {
							commandContent2 += " "+hexChar(commandContent[jj]);
						}
						macroEventsKeyMap[configName][stoi(buttonNumber)].emplace_back(new MacroEvent(configKeysMap[commandType], &commandType, &commandContent2));
					}else{
						macroEventsKeyMap[configName][stoi(buttonNumber)].emplace_back(new MacroEvent(configKeysMap[commandType], &commandType, &commandContent));
					}//Encode and store mapping v3
				}
			}
		}
		in.close();
	}
	currentConfigName = configName;
	(void)!(system(("notify-send -t 200 'New config :' '"+configName+"'").c_str()));
}

string hexChar(char a){
	stringstream hexedChar;
	hexedChar << "0x00" << std::hex << (int)(a);
	return hexedChar.str();
}

void run() {
	fd_set readset;
	ioctl(side_btn_fd, EVIOCGRAB, 1);// Give application exclusive control over side buttons.
	while (1) {
		if(configSwitcher.isRemapScheduled()) {//remap
			this->loadConf(configSwitcher.RemapString());//change config for macroEvents[ii]->Content()
			configSwitcher.unScheduleReMap();
		}

		FD_ZERO(&readset);
		FD_SET(side_btn_fd, &readset);
		FD_SET(extra_btn_fd, &readset);
		int rd1 = select(FD_SETSIZE, &readset, NULL, NULL, NULL);
		if (rd1 == -1) exit(2);
		if (FD_ISSET(side_btn_fd, &readset))// Side buttons
		{
			rd1 = read(side_btn_fd, ev1, size * 64);
			if (rd1 == -1) exit(2);
			if (ev1[0].value != ' ' && ev1[1].type == EV_KEY) {//Key event (press or release)
				switch (ev1[1].code) {
				case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10:  case 11:  case 12:  case 13:
					std::thread actionThread(chooseAction, (ev1[1].value == 1), &macroEventsKeyMap[currentConfigName][ev1[1].code - 1], &configSwitcher);//real key number = ev1[1].code - 1
					actionThread.detach();
					break;
				}
			}
		}else if (FD_ISSET(extra_btn_fd, &readset))// Extra buttons
		{
			rd1 = read(extra_btn_fd, ev1, size * 64);
			if (rd1 == -1) exit(2);
			if (ev1[1].type == 1) {//Only extra buttons
				switch (ev1[1].code) {
				case 275: case 276:
					std::thread actionThread(chooseAction, (ev1[1].value == 1), &macroEventsKeyMap[currentConfigName][ev1[1].code - OFFSET], &configSwitcher);//real key number = ev1[1].code - OFFSET
					actionThread.detach();
					break;
				}
			}
		}
	}
}

static void chooseAction(bool pressed, std::vector<MacroEvent *> * relativeMacroEventsPointer, configSwitchScheduler * congSwitcherPointer) {
	for(auto & macroEventPointer : (*relativeMacroEventsPointer)) {//run all the events at Key
		if(macroEventPointer->KeyType()->IsOnKeyPressed() == pressed) {  //test if key state is matching
			if((macroEventPointer->KeyType()->isInternal())) {  //INTERNAL COMMANDS
				if (macroEventPointer->Type() == "sleep" || macroEventPointer->Type() == "sleeprelease") {
					usleep(stoul(macroEventPointer->Content()) * 1000);  //microseconds make me dizzy in keymap.txt
				}else if(macroEventPointer->Type() == "chmap" || macroEventPointer->Type() == "chmaprelease") {
					congSwitcherPointer->scheduleReMap(macroEventPointer->Content());  //schedule config switch/change
				}
			}else{  //CASUAL COMMANDS
				macroEventPointer->execute();  //runs the Command
			}
		}
	}
}
public:
NagaDaemon() {
	//modulable device files list
	devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Epic-if01-event-kbd", "/dev/input/by-id/usb-Razer_Razer_Naga_Epic-event-mouse");                                        // NAGA EPIC
	devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Dock-if01-event-kbd", "/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Dock-event-mouse");                              // NAGA EPIC DOCK
	devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_2014-if02-event-kbd", "/dev/input/by-id/usb-Razer_Razer_Naga_2014-event-mouse");                                        // NAGA 2014
	devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga-if01-event-kbd", "/dev/input/by-id/usb-Razer_Razer_Naga-event-mouse");                                                  // NAGA MOLTEN
	devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Chroma-if01-event-kbd", "/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Chroma-event-mouse");                          // NAGA EPIC CHROMA
	devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Chroma_Dock-if01-event-kbd", "/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Chroma_Dock-event-mouse");                // NAGA EPIC CHROMA DOCK
	devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Chroma-if02-event-kbd", "/dev/input/by-id/usb-Razer_Razer_Naga_Chroma-event-mouse");                                    // NAGA CHROMA
	devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Hex-if01-event-kbd", "/dev/input/by-id/usb-Razer_Razer_Naga_Hex-event-mouse");                                          // NAGA HEX
	devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Hex_V2-if02-event-kbd","/dev/input/by-id/usb-Razer_Razer_Naga_Hex_V2-event-mouse");                                     // NAGA HEX v2
	devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Trinity_00000000001A-if02-event-kbd", "/dev/input/by-id/usb-Razer_Razer_Naga_Trinity_00000000001A-event-mouse");        // NAGA Trinity
	devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Left_Handed_Edition-if02-event-kbd", "/dev/input/by-id/usb-Razer_Razer_Naga_Left_Handed_Edition-event-mouse");          // NAGA Left Handed

	//modulable options list to manage internals inside chooseAction method arg1:COMMAND, arg2:isInternal, arg3:onKeyPressed?
	configKeysMap.insert(std::pair<std::string, configKey*>("key", NULL));//special one

	configKeysMap.insert(std::pair<std::string, configKey*>("chmap", new configKey("", true, true)));//change keymap
	configKeysMap.insert(std::pair<std::string, configKey*>("chmaprelease", new configKey("", true, false)));

	configKeysMap.insert(std::pair<std::string, configKey*>("sleep", new configKey("", true, true)));
	configKeysMap.insert(std::pair<std::string, configKey*>("sleeprelease", new configKey("", true, false)));

	configKeysMap.insert(std::pair<std::string, configKey*>("run", new configKey("setsid ", false, true)));
	configKeysMap.insert(std::pair<std::string, configKey*>("run2", new configKey("", false, true)));

	configKeysMap.insert(std::pair<std::string, configKey*>("runrelease", new configKey("setsid ", false, false)));
	configKeysMap.insert(std::pair<std::string, configKey*>("runrelease2", new configKey("", false, false)));

	configKeysMap.insert(std::pair<std::string, configKey*>("keypressonpress", new configKey("setsid xdotool keydown --window getactivewindow ", false, true)));
	configKeysMap.insert(std::pair<std::string, configKey*>("keypressonrelease", new configKey("setsid xdotool keydown --window getactivewindow ", false, false)));

	configKeysMap.insert(std::pair<std::string, configKey*>("keyreleaseonpress", new configKey("setsid xdotool keyup --window getactivewindow ", false, true)));
	configKeysMap.insert(std::pair<std::string, configKey*>("keyreleaseonrelease", new configKey("setsid xdotool keyup --window getactivewindow ", false, false)));

	configKeysMap.insert(std::pair<std::string, configKey*>("keyclick", new configKey("setsid xdotool key --window getactivewindow ", false, true)));
	configKeysMap.insert(std::pair<std::string, configKey*>("keyclickrelease", new configKey("setsid xdotool key --window getactivewindow ", false, false)));

	configKeysMap.insert(std::pair<std::string, configKey*>("string", new configKey("setsid xdotool key --delay 0 --window getactivewindow", false, true)));
	configKeysMap.insert(std::pair<std::string, configKey*>("stringrelease", new configKey("setsid xdotool key --delay 0 --window getactivewindow", false, false)));

	size = sizeof(struct input_event);
	for (auto &device : devices) {//Setup check
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
	this->run();
}
};

void stopD() {
	clog << "Stopping possible naga daemon" << endl;
	(void)!(system(("kill $(ps aux | grep naga | grep debug | grep -v "+ std::to_string((int)getpid()) +" | awk '{print $2}') > /dev/null 2>&1").c_str()));
	(void)!(system(("/usr/local/bin/killroot.sh")));
};

void stopDRoot() {
	(void)!(system(("kill $(ps aux | grep naga | grep debug | grep root | grep -v "+ std::to_string((int)getpid()) +" | awk '{print $2}') > /dev/null 2>&1").c_str()));
};

//arguments manage
int main(int argc, char *argv[]) {
	if(argc>1) {
		if(strstr(argv[1], "start")!=NULL) {
			clog << "Stopping possible naga daemon\nStarting naga daemon in hidden mode..." << endl;
			(void)!(system("setsid naga debug > /dev/null 2>&1 &"));
		}else if(strstr(argv[1], "killroot")!=NULL) {
			stopDRoot();
		}else if(strstr(argv[1], "kill")!=NULL || strstr(argv[1], "stop")!=NULL) {
			stopD();
		}else if(strstr(argv[1], "uninstall")!=NULL) {
			string answer;
			clog << "Are you sure you want to uninstall ? y/n" << endl;
			cin >> answer;
			if(answer.length() != 1 || ( answer[0] != 'y' &&  answer[0] != 'Y' )) {
				clog << "Aborting" << endl;
			}else{
				(void)!(system("bash /usr/local/bin/nagaUninstall.sh"));
			}
		}else if(strstr(argv[1], "debug")!=NULL) {
			stopD();
			usleep(40000);
			clog << "Starting naga daemon in debug mode..." << endl;
			(void)!(system("/usr/local/bin/nagaXinputStart.sh"));
			NagaDaemon();
		}
	} else {
		clog << "Possible arguments : \n  -start          Starts the daemon in hidden mode. (stops it before)\n  -stop           Stops the daemon.\n  -debug          Starts the daemon in the terminal,\n" <<
		  "giving access to logs. (stops it before)."<< endl;
	}
	return 0;
}
