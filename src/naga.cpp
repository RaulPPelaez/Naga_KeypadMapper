//This is lostallmymoney's remake of RaulPPelaez's original tool.
//RaulPPelaez, et. al wrote the original file.  As long as you retain this notice you
//can do whatever you want with this stuff.

//EXPERIMENTAL
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>

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
#define N_MODIFIER_INDEXES (Mod5MapIndex + 1)
using namespace std;

typedef enum
{
	FAKEKEYMOD_SHIFT = (1<<1),
	FAKEKEYMOD_CONTROL = (1<<2),
	FAKEKEYMOD_ALT = (1<<3),
	FAKEKEYMOD_META = (1<<4)

} FakeKeyModifier;

typedef enum
{
	command, key, keypress, keyrelease, keyclick, type, chmap, ssleep
} CommandTypeEnum;

struct FakeKey
{
	Display *xdpy;
	int min_keycode, max_keycode;
	int n_keysyms_per_keycode;
	KeySym *keysyms;
	int held_keycode;
	int held_state_flags;
	KeyCode modifier_table[N_MODIFIER_INDEXES];
	int shift_mod_index, alt_mod_index, meta_mod_index;
};

static int utf8_to_ucs4 (const unsigned char *src_orig, unsigned int *dst, int len)
{
	const unsigned char *src = src_orig;
	unsigned char s;
	int extra;
	unsigned int result;

	if (len == 0)
		return 0;

	s = *src++;
	len--;

	if (!(s & 0x80))
	{
		result = s;
		extra = 0;
	}
	else if (!(s & 0x40))
	{
		return -1;
	}
	else if (!(s & 0x20))
	{
		result = s & 0x1f;
		extra = 1;
	}
	else if (!(s & 0x10))
	{
		result = s & 0xf;
		extra = 2;
	}
	else if (!(s & 0x08))
	{
		result = s & 0x07;
		extra = 3;
	}
	else if (!(s & 0x04))
	{
		result = s & 0x03;
		extra = 4;
	}
	else if ( !(s & 0x02))
	{
		result = s & 0x01;
		extra = 5;
	}
	else
	{
		return -1;
	}
	if (extra > len)
		return -1;

	while (extra--)
	{
		result <<= 6;
		s = *src++;

		if ((s & 0xc0) != 0x80)
			return -1;

		result |= s & 0x3f;
	}
	*dst = result;
	return src - src_orig;
}

FakeKey* fakekey_init(Display *xdpy)
{
	FakeKey *fk = NULL;
	int event, error, major, minor;
	XModifierKeymap *modifiers;
	int mod_index;
	int mod_key;
	KeyCode *kp;

	if (xdpy == NULL) return NULL;

	if (!XTestQueryExtension(xdpy, &event, &error, &major, &minor))
	{
		return NULL;
	}

	fk = (FakeKey*)malloc(sizeof(FakeKey));
	memset(fk,0,sizeof(FakeKey));

	fk->xdpy = xdpy;

	/* Find keycode limits */

	XDisplayKeycodes(fk->xdpy, &fk->min_keycode, &fk->max_keycode);

	/* Get the mapping */

	/* TODO: Below needs to be kept in sync with anything else
	 * that may change the keyboard mapping.
	 *
	 * case MappingNotify:
	 * XRefreshKeyboardMapping(&ev.xmapping);
	 *
	 */

	fk->keysyms = XGetKeyboardMapping(fk->xdpy,
	                                  fk->min_keycode,
	                                  fk->max_keycode - fk->min_keycode + 1,
	                                  &fk->n_keysyms_per_keycode);


	modifiers = XGetModifierMapping(fk->xdpy);

	kp = modifiers->modifiermap;

	for (mod_index = 0; mod_index < 8; mod_index++)
	{
		fk->modifier_table[mod_index] = 0;

		for (mod_key = 0; mod_key < modifiers->max_keypermod; mod_key++)
		{
			int keycode = kp[mod_index * modifiers->max_keypermod + mod_key];

			if (keycode != 0)
			{
				fk->modifier_table[mod_index] = keycode;
				break;
			}
		}
	}

	for (mod_index = Mod1MapIndex; mod_index <= Mod5MapIndex; mod_index++)
	{
		if (fk->modifier_table[mod_index])
		{
			KeySym ks = XKeycodeToKeysym(fk->xdpy,
			                             fk->modifier_table[mod_index], 0);

			/*
			 * Note: ControlMapIndex is already defined by xlib
			 * ShiftMapIndex
			 */

			switch (ks)
			{
			case XK_Meta_R:
			case XK_Meta_L:
				fk->meta_mod_index = mod_index;
				break;

			case XK_Alt_R:
			case XK_Alt_L:
				fk->alt_mod_index = mod_index;
				break;

			case XK_Shift_R:
			case XK_Shift_L:
				fk->shift_mod_index = mod_index;
				break;
			}
		}
	}

	if (modifiers)
		XFreeModifiermap(modifiers);

	return fk;
}

int fakekey_reload_keysyms(FakeKey *fk)
{
	if (fk->keysyms)
		XFree(fk->keysyms);

	fk->keysyms = XGetKeyboardMapping(fk->xdpy,
	                                  fk->min_keycode,
	                                  fk->max_keycode - fk->min_keycode + 1,
	                                  &fk->n_keysyms_per_keycode);
	return 1;
}

int fakekey_send_keyevent(FakeKey *fk, KeyCode keycode, Bool is_press, int flags)
{
	if (flags)
	{
		if (flags & FAKEKEYMOD_SHIFT)
			XTestFakeKeyEvent(fk->xdpy, fk->modifier_table[ShiftMapIndex],
			                  is_press, CurrentTime);

		if (flags & FAKEKEYMOD_CONTROL)
			XTestFakeKeyEvent(fk->xdpy, fk->modifier_table[ControlMapIndex],
			                  is_press, CurrentTime);

		if (flags & FAKEKEYMOD_ALT)
			XTestFakeKeyEvent(fk->xdpy, fk->modifier_table[fk->alt_mod_index],
			                  is_press, CurrentTime);

		XSync(fk->xdpy, False);
	}

	XTestFakeKeyEvent(fk->xdpy, keycode, is_press, CurrentTime);

	XSync(fk->xdpy, False);

	return 1;
}

int fakekey_press_keysym(FakeKey *fk, KeySym keysym, int flags)
{
	static int modifiedkey;
	KeyCode code = 0;

	if ((code = XKeysymToKeycode(fk->xdpy, keysym)) != 0)
	{
		/* we already have a keycode for this keysym */
		/* Does it need a shift key though ? */
		if (XKeycodeToKeysym(fk->xdpy, code, 0) != keysym)
		{
			/* TODO: Assumes 1st modifier is shifted */
			if (XKeycodeToKeysym(fk->xdpy, code, 1) == keysym)
				flags |= FAKEKEYMOD_SHIFT; /* can get at it via shift */
			else
				code = 0; /* urg, some other modifier do it the heavy way */
		}
		else
		{
			/* the keysym is unshifted; clear the shift flag if it is set */
			flags &= ~FAKEKEYMOD_SHIFT;
		}
	}

	if (!code)
	{
		int index;

		/* Change one of the last 10 keysyms to our converted utf8,
		 * remapping the x keyboard on the fly.
		 *
		 * This make assumption the last 10 arn't already used.
		 * TODO: probably safer to check for this.
		 */

		modifiedkey = (modifiedkey+1) % 10;

		/* Point at the end of keysyms, modifier 0 */

		index = (fk->max_keycode - fk->min_keycode - modifiedkey - 1) * fk->n_keysyms_per_keycode;

		fk->keysyms[index] = keysym;

		XChangeKeyboardMapping(fk->xdpy,
		                       fk->min_keycode,
		                       fk->n_keysyms_per_keycode,
		                       fk->keysyms,
		                       (fk->max_keycode-fk->min_keycode));

		XSync(fk->xdpy, False);

		/* From dasher src;
		 * There's no way whatsoever that this could ever possibly
		 * be guaranteed to work (ever), but it does.
		 *
		 */

		code = fk->max_keycode - modifiedkey - 1;

		/* The below is lightly safer;
		 *
		 * code = XKeysymToKeycode(fk->xdpy, keysym);
		 *
		 * but this appears to break in that the new mapping is not immediatly
		 * put to work. It would seem a MappingNotify event is needed so
		 * Xlib can do some changes internally ? ( xlib is doing something
		 * related to above ? )
		 *
		 * Probably better to try and grab the mapping notify *here* ?
		 */

		if (XKeycodeToKeysym(fk->xdpy, code, 0) != keysym)
		{
			/* TODO: Assumes 1st modifier is shifted */
			if (XKeycodeToKeysym(fk->xdpy, code, 1) == keysym)
				flags |= FAKEKEYMOD_SHIFT; /* can get at it via shift */
		}
	}

	if (code != 0)
	{
		fakekey_send_keyevent(fk, code, True, flags);

		fk->held_state_flags = flags;
		fk->held_keycode = code;

		return 1;
	}

	fk->held_state_flags = 0;
	fk->held_keycode = 0;

	return 0; /* failed */
}

int fakekey_press(FakeKey *fk, const unsigned char *utf8_char_in, int len_bytes, int flags)
{
	unsigned int ucs4_out;

	if (fk->held_keycode) /* key is already held down */
		return 0;

	/* TODO: check for Return key here and other chars */

	if (len_bytes < 0)
	{
		/*
		   unsigned char *p = utf8_char_in;
		   while (*p != '\0') { len_bytes++; p++; }
		 */
		len_bytes = strlen((const char*)utf8_char_in); /* OK ? */
	}

	if (utf8_to_ucs4 (utf8_char_in, &ucs4_out, len_bytes) < 1)
	{
		return 0;
	}

	/* first check for Latin-1 characters (1:1 mapping)
	   if ((keysym >= 0x0020 && keysym <= 0x007e) ||
	   (keysym >= 0x00a0 && keysym <= 0x00ff))
	   return keysym;
	 */

	if (ucs4_out > 0x00ff) /* < 0xff assume Latin-1 1:1 mapping */
		ucs4_out = ucs4_out | 0x01000000; /* This gives us the magic X keysym */

	return fakekey_press_keysym(fk, (KeySym)ucs4_out, flags);
}

void fakekey_repeat(FakeKey *fk)
{
	if (!fk->held_keycode)
		return;

	fakekey_send_keyevent(fk, fk->held_keycode, True, fk->held_state_flags);
}

void fakekey_release(FakeKey *fk)
{
	if (!fk->held_keycode)
		return;

	fakekey_send_keyevent(fk, fk->held_keycode, False, fk->held_state_flags);

	fk->held_state_flags = 0;
	fk->held_keycode = 0;
}

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
	const void special(string const& command, bool isPressed = true) const {

	clog << "Special reached" << endl;

	Display *display;
	display = XOpenDisplay(NULL);
	FakeKey* theKeyFaker = fakekey_init(display);
	int strSize = command.size();
	for(int z = 0; z < strSize; z++){
		unsigned char keyChar = command[z];
		fakekey_press(theKeyFaker, &keyChar, 8, 0);
		fakekey_release(theKeyFaker);
	}

	// Clear the X buffer which actually sends the key press
	XFlush(display);
	// Disconnect from X
	XCloseDisplay(display);

	clog << "Special ends" << endl;
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
	const void special() const {
		keyType->special(content);
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

map<string, configKey*> configKeysMap;
map<string, map<int, MacroEventVector > > macroEventsKeyMap;
configSwitchScheduler configSwitcher = configSwitchScheduler();

string currentConfigName;
struct input_event ev1[64];
int side_btn_fd, extra_btn_fd, size;
vector<CharAndChar > devices;

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
				if (commandContent[0] == '#' || commandContent.find_first_not_of(' ') == string::npos) continue; //Ignore comments, empty lines
				pos = commandContent.find('=');
				string commandType = commandContent.substr(0, pos);//commandType = numbers + command type
				commandContent.erase(0, pos+1);//commandContent = command content
				commandType.erase(remove(commandType.begin(), commandType.end(), ' '), commandType.end());//Erase spaces inside 1st part of the line
				pos = commandType.find("-");
				string buttonNumber = commandType.substr(0, pos);//Isolate button number
				commandType = commandType.substr(pos + 1);//Isolate command type
				for(char & c : commandType)
				{
					c = tolower(c);
				}

				if(configKeysMap.contains(commandType)) { //filter out bad types
					int buttonNumberI;
					try {
						buttonNumberI = stoi(buttonNumber);
					} catch (...) {
						clog << "At config line " << readingLine << ": expected a number" << endl;
						exit(1);
					}
					MacroEventVector * currentMacroEvents = &macroEventsKeyMap[configName][buttonNumberI];
					if(commandType=="key") {
						if(commandContent.size()==1) {
							commandContent = hexChar(commandContent[0]);
						}
						currentMacroEvents->emplace_back(new MacroEvent(configKeysMap["keypressonpress"], &commandType, &commandContent));
						currentMacroEvents->emplace_back(new MacroEvent(configKeysMap["keyreleaseonrelease"], &commandType, &commandContent));
					}else if(commandType=="string" || commandType=="stringrelease") {
						string commandContent2="";
						for(long unsigned jj=0; jj<commandContent.size(); jj++) {
							commandContent2 += " "+hexChar(commandContent[jj]);
						}
						currentMacroEvents->emplace_back(new MacroEvent(configKeysMap[commandType], &commandType, &commandContent2));
					}else{
						currentMacroEvents->emplace_back(new MacroEvent(configKeysMap[commandType], &commandType, &commandContent));
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

void run() {
	fd_set readset;
	ioctl(side_btn_fd, EVIOCGRAB, 1);// Give application exclusive control over side buttons.
	input_event * ev11 = &ev1[1];
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
					thread actionThread(chooseAction, (ev11->value == 1), &macroEventsKeyMap[currentConfigName][ev11->code - 1], &configSwitcher);//real key number = ev11->code - 1
					actionThread.detach();
					break;
				}
			}
		}else if (FD_ISSET(extra_btn_fd, &readset))// Extra buttons
		{
			if (read(extra_btn_fd, ev1, size * 64) == -1) exit(2);
			if (ev11->type == 1) {//Only extra buttons
				switch (ev11->code) {
				case 275: case 276:
					thread actionThread(chooseAction, (ev11->value == 1), &macroEventsKeyMap[currentConfigName][ev11->code - OFFSET], &configSwitcher);//real key number = ev11->code - OFFSET
					actionThread.detach();
					break;
				}
			}
		}
	}
}

static void chooseAction(bool pressed, MacroEventVector * relativeMacroEventsPointer, configSwitchScheduler * congSwitcherPointer) {
	for(MacroEvent * macroEventPointer : *relativeMacroEventsPointer) {//run all the events at Key
		if(macroEventPointer->KeyType()->IsOnKeyPressed() == pressed) {  //test if key state is matching
			if(macroEventPointer->KeyType()->isInternal()) {  //INTERNAL COMMANDS
				if (macroEventPointer->Type() == "sleep" || macroEventPointer->Type() == "sleeprelease") {
					usleep(stoul(macroEventPointer->Content()) * 1000);  //microseconds make me dizzy in keymap.txt
				}else if(macroEventPointer->Type() == "chmap" || macroEventPointer->Type() == "chmaprelease") {
					congSwitcherPointer->scheduleReMap(macroEventPointer->Content());  //schedule config switch/change
				}else if(macroEventPointer->Type() == "special" || macroEventPointer->Type() == "specialrelease") {
					macroEventPointer->special();
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
	devices.emplace_back("/dev/input/by-id/usb-Razer_Razer_Naga_Pro_000000000000-if02-event-kbd" , "/dev/input/by-id/usb-Razer_Razer_Naga_Pro_000000000000-event-mouse"); 							// NAGA PRO WIRELESS

	//modulable options list to manage internals inside chooseAction method arg1:COMMAND, arg2:isInternal, arg3:onKeyPressed?
	configKeysMap.insert(stringAndConfigKey("key", NULL));//special one

	configKeysMap.insert(stringAndConfigKey("chmap", new configKey("", true, true)));//change keymap
	configKeysMap.insert(stringAndConfigKey("chmaprelease", new configKey("", true, false)));

	configKeysMap.insert(stringAndConfigKey("sleep", new configKey("", true, true)));
	configKeysMap.insert(stringAndConfigKey("sleeprelease", new configKey("", true, false)));

	configKeysMap.insert(stringAndConfigKey("run", new configKey("setsid ", false, true)));
	configKeysMap.insert(stringAndConfigKey("run2", new configKey("", false, true)));

	configKeysMap.insert(stringAndConfigKey("runrelease", new configKey("setsid ", false, false)));
	configKeysMap.insert(stringAndConfigKey("runrelease2", new configKey("", false, false)));

	configKeysMap.insert(stringAndConfigKey("keypressonpress", new configKey("setsid xdotool keydown --window getactivewindow ", false, true)));
	configKeysMap.insert(stringAndConfigKey("keypressonrelease", new configKey("setsid xdotool keydown --window getactivewindow ", false, false)));

	configKeysMap.insert(stringAndConfigKey("keyreleaseonpress", new configKey("setsid xdotool keyup --window getactivewindow ", false, true)));
	configKeysMap.insert(stringAndConfigKey("keyreleaseonrelease", new configKey("setsid xdotool keyup --window getactivewindow ", false, false)));

	configKeysMap.insert(stringAndConfigKey("keyclick", new configKey("setsid xdotool key --window getactivewindow ", false, true)));
	configKeysMap.insert(stringAndConfigKey("keyclickrelease", new configKey("setsid xdotool key --window getactivewindow ", false, false)));

	configKeysMap.insert(stringAndConfigKey("string", new configKey("setsid xdotool key --delay 0 --window getactivewindow", false, true)));
	configKeysMap.insert(stringAndConfigKey("stringrelease", new configKey("setsid xdotool key --delay 0 --window getactivewindow", false, false)));

	configKeysMap.insert(stringAndConfigKey("special", new configKey("", true, true)));
	configKeysMap.insert(stringAndConfigKey("specialrelease", new configKey("", true, false)));

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
