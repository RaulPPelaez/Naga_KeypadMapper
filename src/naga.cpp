/*
Apocatarsis 2015
Released with absolutely no warranty, use with your own responsability.

The differences between the naga and any other input keyboard-like device in 
this code is in the number of keys (line 25)
And the code of each button (in the Naga this is Code=2 for button 1 and all the way to Code=13 for button 12)
you can change line 91 to adapt it to other devices.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <signal.h>
#include <iostream>
#include<vector>
#include<algorithm> 
#define DEV_NUM_KEYS 12
using namespace std;
class NagaDaemon{
  public:
    NagaDaemon(int argc, char *argv[]){
	device = NULL;	
	size = sizeof (struct input_event);		
      	this->load_conf();
	 //Setup check
  	if (argv[1] == NULL){
      	  printf("Please specify (on the command line) the path to the dev event interface devicei\n");
     	  exit (0);
     	}
  	if ((getuid ()) != 0)printf ("You are not root! This may not work...\n");
  	if (argc > 1) device = argv[1];
  	//Open Device
  	if ((fd = open (device, O_RDONLY)) == -1)printf ("%s is not a vaild device\n", device);

  	//Print Device Name
  	ioctl (fd, EVIOCGNAME (sizeof (name)), name);
  	printf ("Reading From : %s (%s)\n", device, name);	
    }
    void load_conf(){
	
	args.resize(DEV_NUM_KEYS);
	options.resize(DEV_NUM_KEYS);

	string conf_file;
	conf_file+=string(getenv("HOME"))+"/.naga/mapping.txt";
	ifstream in(conf_file.c_str(), ios::in);
  	if (!in) { cerr << "Cannot open " << conf_file << endl; exit(1); }
 	string line,token1,token2;
	int pos;
	while(getline(in,line)){
	  //Erase spaces
          line.erase(std::remove(line.begin(), line.end(),' '),line.end());
	  //Ignore comments
	  if (line[0] == '#') { continue; }
	  //Search option and argument
	  pos = line.find("-");
	  token1 = line.substr(0,pos);
	  line = line.substr(pos+1);
	  pos = line.find("=");
	  token2 = line.substr(0,pos);
	  line = line.substr(pos+1);
	  //Encode and store mapping
	  pos =  atoi(token1.c_str())-1;
	  if(token2=="key")options[pos] = 1;
	  else if(token2=="run")options[pos] = 2;
	  else printf("Not supported key action, check the syntax in mapping.txt!\n");
	  args[pos] = line;
	  if(pos==DEV_NUM_KEYS) break; //Only 12 keys for the Naga
	}
    }
    void Run(){
	string keyop="xdotool key --window getactivewindow ";
	string command;
	int pid;
	while (1){
	  rd = read (fd, ev, size * 64);
          value = ev[0].value;
          if (value != ' ' && ev[1].value == 1 && ev[1].type == 1){ // Only read the key press event
            for(int i=0;i<DEV_NUM_KEYS;i++){//For all keys
	      if(ev[1].code==(i+2)){ //If code i+2 is on (Only for naga)
	      	switch (options[i]){
		  case 1: //key
		    command = keyop+args[i];
		    break;
		  case 2: //run system command
		    command = "setsid "+args[i]+" &";
		    break;
		}//ENDSWITCH
		pid=system(command.c_str());
	      }//ENDIF
            }//ENDFOR
	  }//ENDIF
	}//ENDWHILE
    }
  private:
    struct input_event ev[64];
    int fd, rd, value, size;
    char name[256];
    char *device;
    vector<string> args;
    vector<int> options;







};

int main (int argc, char *argv[])
{
NagaDaemon daemon(argc,argv);

daemon.Run(); 

  return 0;
} 
