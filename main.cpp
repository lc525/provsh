/*
 * main.cpp
 *
 *  Created on: Dec 13, 2012
 *      Author: calucian
 */
#include "provsh.h"
#include "shellcmd.h"
#include <string>
#include <iostream>

using namespace provsh;
using namespace std;

int main(int argc, char** argv){
	Shell s;

	boost::optional<std::string> cmdline;
	while(!s.shouldExit()){ // runs until EOF or exit command

		cmdline = s.readCommandLine();

		if(!cmdline) { // EOF recieved, exit
			cout<<endl;
			break;
		}

		if((*cmdline).length()!=0){
			s.executeCommand(*cmdline);
		}

	}

}


