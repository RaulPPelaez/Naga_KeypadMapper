//This is lostallmymoney's remake of RaulPPelaez's original tool.
//RaulPPelaez, et. al wrote the original file.  As long as you retain this notice you
//can do whatever you want with this stuff.

#include "fakeKeys.h"

#include <iostream>
#include <vector>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <cstring>
#include <thread>
#include <mutex>
#include <map>
#include <sstream>
#define OFFSET 262
using namespace std;

mutex fakeKeyFollowUpsMutex, configSwitcherMutex;
vector<pair<char, FakeKey *> *> fakeKeyFollowUps;
int fakeKeyFollowCount = 0;

class configKey {
	private:
	const string prefix;
	const bool internal, onKeyPressed;
	void (*internalFunction)(string * c);
	public:
	const bool& IsOnKeyPressed() const { return onKeyPressed; }
	const bool& isInternal() const { return internal; }
	const void runInternal(string * content) const { internalFunction(content); }
	const string& Prefix() const { return prefix; }
	configKey(string&& tcontent, bool tonKeyPressed, void (*tinternalF)(string *cc) = NULL) : prefix(tcontent), internal(tinternalF!=NULL), onKeyPressed(tonKeyPressed), internalFunction(tinternalF){
	}
};

class MacroEvent {
	private:
	const configKey * keyType;
	const string type, content;
	public:
	const configKey * KeyType() const { return keyType; }
	const string& Type() const { return type; }
	const string& Content() const { return content; }
	const void execute() const {
		(void)!(system((keyType->Prefix()+content).c_str()));
	}

	MacroEvent(configKey * tkeyType, string * ttype, string * tcontent) : keyType(tkeyType), type(*ttype), content(*tcontent){
	}
};

typedef pair<string, configKey*> stringAndConfigKey;
typedef vector<MacroEvent *> MacroEventVector;
typedef pair<const char *,const char *> CharAndChar;

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
	void scheduleReMap(string * reMapString) {
		scheduledReMapString = *reMapString;
		scheduledReMap = true;
	}
	void unScheduleReMap(){
		scheduledReMap=false;
	}
};

configSwitchScheduler configSwitcher = configSwitchScheduler();

class NagaDaemon {
private:
const string conf_file = string(getenv("HOME")) + "/.naga/keyMap.txt";

map<string, configKey*> configKeysMap;
map<string, map<int, map<bool, MacroEventVector > > > macroEventsKeyMaps;

string currentConfigName;
struct input_event ev1[64];
vector<CharAndChar > devices;

void loadConf(string configName) {
	if(!macroEventsKeyMaps.contains(configName)) {
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
				if (commandContent[0] == '#' || commandContent.find_first_not_of(' ') == string::npos) continue; //Ignore comments, empty lines
				pos = commandContent.find('=');
				string commandType = commandContent.substr(0, pos);//commandType = numbers + command type
				commandContent.erase(0, pos+1);//commandContent = command content
				commandType.erase(remove(commandType.begin(), commandType.end(), ' '), commandType.end());//Erase spaces inside 1st part of the line
				pos = commandType.find("-");
				string buttonNumber = commandType.substr(0, pos);//Isolate button number
				commandType = commandType.substr(pos + 1);//Isolate command type
				for(char & c : commandType)
					c = tolower(c);

				if(configKeysMap.contains(commandType)) { //filter out bad types
					int buttonNumberI;
					try {
						buttonNumberI = stoi(buttonNumber);
					} catch (...) {
						clog << "At config line " << readingLine << ": expected a number" << endl;
						exit(1);
					}

					if(commandType=="key") {
						if(commandContent.size()==1){
							commandContent = hexChar(commandContent[0]);
						}
						macroEventsKeyMaps[configName][buttonNumberI][true].emplace_back(new MacroEvent(configKeysMap["keypressonpress"], &commandType, &commandContent));
						macroEventsKeyMaps[configName][buttonNumberI][false].emplace_back(new MacroEvent(configKeysMap["keyreleaseonrelease"], &commandType, &commandContent));
					}else{
						macroEventsKeyMaps[configName][buttonNumberI][configKeysMap[commandType]->IsOnKeyPressed()].emplace_back(new MacroEvent(configKeysMap[commandType], &commandType, &commandContent));
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
	hexedChar << "0x00" << hex << (int)(a);
	return hexedChar.str();
}

int side_btn_fd, extra_btn_fd, size;
input_event * ev11;
fd_set readset;

void run() {
	ioctl(side_btn_fd, EVIOCGRAB, 1);// Give application exclusive control over side buttons.
	ev11 = &ev1[1];
	while (1) {
		if(configSwitcher.isRemapScheduled()) {//remap
			loadConf(configSwitcher.RemapString());//change config for macroEvents[ii]->Content()
			configSwitcher.unScheduleReMap();
		}

		FD_ZERO(&readset);
		FD_SET(side_btn_fd, &readset);
		FD_SET(extra_btn_fd, &readset);
		if (select(FD_SETSIZE, &readset, NULL, NULL, NULL) == -1) exit(2);
		if (FD_ISSET(side_btn_fd, &readset))// Side buttons
		{
			if (read(side_btn_fd, ev1, size * 64) == -1) exit(2);
			if (ev1[0].value != ' ' && ev11->type == EV_KEY) {//Key event (press or release)
				switch (ev11->code) {
				case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10:  case 11:  case 12:  case 13:
					thread(chooseAction, &macroEventsKeyMaps[currentConfigName][ev11->code - 1][ev11->value == 1]).detach();//real key number = ev11->code - 1
					break;
				}
			}
		}else if (FD_ISSET(extra_btn_fd, &readset))// Extra buttons
		{
			if (read(extra_btn_fd, ev1, size * 64) == -1) exit(2);
			if (ev11->type == 1) {//Only extra buttons
				switch (ev11->code) {
				case 275: case 276:
					thread(chooseAction, &macroEventsKeyMaps[currentConfigName][ev11->code - OFFSET][ev11->value == 1]).detach();//real key number = ev11->code - OFFSET
					break;
				}
			}
		}
	}
}

//Functions that can be given to configKeys
static void writeString(string *macroContent){
	int strSize = (*macroContent).size();
	FakeKey *aKeyFaker = fakekey_init(XOpenDisplay(NULL));
	for(int z = 0; z < strSize; z++){
		fakekey_press(aKeyFaker, (unsigned char *)&(*macroContent)[z], 8, 0);
		fakekey_release(aKeyFaker);
	}
	XFlush(aKeyFaker->xdpy);
	XCloseDisplay(aKeyFaker->xdpy);
}

static void specialPress(string *macroContent){
	lock_guard<mutex> guard(fakeKeyFollowUpsMutex);
	FakeKey * aKeyFaker = fakekey_init(XOpenDisplay(NULL));
	fakekey_press(aKeyFaker, (unsigned char *)&(*macroContent)[0], 8, 0);
	XFlush(aKeyFaker->xdpy);
	fakeKeyFollowUps.emplace_back(new pair<char, FakeKey *>((*macroContent)[0],aKeyFaker));
	fakeKeyFollowCount++;
}

static void specialRelease(string *macroContent){
	lock_guard<mutex> guard(fakeKeyFollowUpsMutex);
	if(fakeKeyFollowCount>0){
		for(int vectorId = fakeKeyFollowUps.size()-1; vectorId>=0; vectorId--){
			pair<char, FakeKey *> * aKeyFollowUp = fakeKeyFollowUps[vectorId];
			if(get<0>(*aKeyFollowUp) == (*macroContent)[0]){
				FakeKey * aKeyFaker = get<1>(*aKeyFollowUp);
				fakekey_release(aKeyFaker);
				XFlush(aKeyFaker->xdpy);
				XCloseDisplay(aKeyFaker->xdpy);
				fakeKeyFollowUps.erase(fakeKeyFollowUps.begin() + vectorId);
				fakeKeyFollowCount--;
			}
			delete aKeyFollowUp;
		}
	}else
		clog << "No candidate for key release" << endl;
}

static void chmapNow(string *macroContent){
	lock_guard<mutex> guard(configSwitcherMutex);
	configSwitcher.scheduleReMap(macroContent);  //schedule config switch/change
}

static void sleepNow(string *macroContent){
	usleep(stoul(*macroContent) * 1000);  //microseconds make me dizzy in keymap.txt
}
//end of configKeys functions



static void chooseAction(MacroEventVector * relativeMacroEventsPointer) {
	for(MacroEvent * macroEventPointer : *relativeMacroEventsPointer) {//run all the events at Key
		if(macroEventPointer->KeyType()->isInternal()) {  //INTERNAL COMMANDS
			macroEventPointer->KeyType()->runInternal((string *)&macroEventPointer->Content());
		}else	macroEventPointer->execute();  //runs the Command
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
	devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Pro_000000000000-if02-event-kbd" , "/dev/input/by-id/usb-Razer_Razer_Naga_Pro_000000000000-event-mouse"); 							// NAGA PRO WIRELESS
	devices.emplace_back("/dev/input/by-id/usb-1532_Razer_Naga_Pro_000000000000-if02-event-kbd" , "/dev/input/by-id/usb-1532_Razer_Naga_Pro_000000000000-event-mouse"); // NAGA PRO

	//modulable options list to manage internals inside chooseAction method arg1:COMMAND, arg2:isInternal, arg3:onKeyPressed?
	configKeysMap.insert(stringAndConfigKey("key", NULL));//special one

	configKeysMap.insert(stringAndConfigKey("chmap", new configKey("", true, chmapNow)));//change keymap
	configKeysMap.insert(stringAndConfigKey("chmaprelease", new configKey("", false, chmapNow)));

	configKeysMap.insert(stringAndConfigKey("sleep", new configKey("", true, sleepNow)));
	configKeysMap.insert(stringAndConfigKey("sleeprelease", new configKey("", false, sleepNow)));

	configKeysMap.insert(stringAndConfigKey("run", new configKey("setsid ", true)));
	configKeysMap.insert(stringAndConfigKey("run2", new configKey("", true)));

	configKeysMap.insert(stringAndConfigKey("runrelease", new configKey("setsid ", false)));
	configKeysMap.insert(stringAndConfigKey("runrelease2", new configKey("", false)));

	configKeysMap.insert(stringAndConfigKey("keypressonpress", new configKey("setsid xdotool keydown --window getactivewindow ", true)));
	configKeysMap.insert(stringAndConfigKey("keypressonrelease", new configKey("setsid xdotool keydown --window getactivewindow ", false)));

	configKeysMap.insert(stringAndConfigKey("keyreleaseonpress", new configKey("setsid xdotool keyup --window getactivewindow ", true)));
	configKeysMap.insert(stringAndConfigKey("keyreleaseonrelease", new configKey("setsid xdotool keyup --window getactivewindow ", false)));

	configKeysMap.insert(stringAndConfigKey("keyclick", new configKey("setsid xdotool key --window getactivewindow ", true)));
	configKeysMap.insert(stringAndConfigKey("keyclickrelease", new configKey("setsid xdotool key --window getactivewindow ", false)));

	configKeysMap.insert(stringAndConfigKey("string", new configKey("", true, writeString)));
	configKeysMap.insert(stringAndConfigKey("stringrelease", new configKey("", false, writeString)));

	configKeysMap.insert(stringAndConfigKey("specialpressonpress", new configKey("", true, specialPress)));
	configKeysMap.insert(stringAndConfigKey("specialpressonrelease", new configKey("", false, specialPress)));

	configKeysMap.insert(stringAndConfigKey("specialreleaseonpress", new configKey("", true, specialRelease)));
	configKeysMap.insert(stringAndConfigKey("specialreleaseonrelease", new configKey("", false, specialRelease)));

	size = sizeof(struct input_event);
	for (CharAndChar &device : devices) {//Setup check
		if ((side_btn_fd = open(device.first, O_RDONLY)) != -1 &&  (extra_btn_fd = open(device.second, O_RDONLY)) != -1) {
			clog << "Reading from: " << device.first << " and " << device.second << endl;
			break;
		}
	}
	if (side_btn_fd == -1 || extra_btn_fd == -1) {
		cerr << "No naga devices found or you don't have permission to access them." << endl;
		exit(1);
	}
	loadConf("defaultConfig");//Initialize config
	run();
	// Disconnect from X
	//XCloseDisplay(display);
}
};

void stopD() {
	clog << "Stopping possible naga daemon" << endl;
	(void)!(system(("kill $(ps aux | grep naga | grep debug | grep -v "+ to_string((int)getpid()) +" | awk '{print $2}') > /dev/null 2>&1").c_str()));
	(void)!(system(("/usr/local/bin/killroot.sh")));
};

void stopDRoot() {
	(void)!(system(("kill $(ps aux | grep naga | grep debug | grep root | grep -v "+ to_string((int)getpid()) +" | awk '{print $2}') > /dev/null 2>&1").c_str()));
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
