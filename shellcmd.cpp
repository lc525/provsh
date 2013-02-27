/*
 * shellcmd.cpp
 *
 *  Created on: Dec 13, 2012
 *      Author: calucian
 */

#include <string>
#include <boost/filesystem.hpp>
#include "shellcmd.h"

using namespace provsh;

RunCommand::RunCommand(Shell* sh, const std::string cmdtok, bool isBuiltin){
	master = sh;
	_cmdtok=cmdtok;
	this->isBuiltin = isBuiltin;
	_argc = -1;
	_argv = NULL;

	if(isBuiltin == true)
		sh->addInternalCommand(cmdtok, this);
}

void CdCommand::execute(){
	if(_argc > 1){
		chdir(_argv[1]);
		master->cwd = boost::filesystem::current_path();
	}
}


/*
 * External or Composite Commands
 */

void CompositeCommand::add(RunCommand* c, std::string combine){
	CommandItem* ci = new CommandItem();
	ci->cmd = c;
	ci->combine = combine;
	ci->next = NULL;

	if(first == NULL) first = ci;
	else{
		CommandItem* cc = first;
		while(cc->next != NULL) cc = first->next;
		cc->next = ci;
	}
}

void CompositeCommand::execute(){

}

inline void ExecCommand::execute(){
	int status;
	if(_preForkExec != NULL) _preForkExec();
	childPid = fork();
	if (childPid == 0){
		if(_preChldExec != NULL) _preChldExec();
		int res = execvp(_argv[0],_argv);
		if(res == -1) {
			std::cout<<"provsh: "<<_argv[0]<<": ";
			std::cout<<strerror(errno)<<std::endl;
			exit(0);
		}
	}
	else{
		if(_waitChild)
			waitpid(childPid, &status, 0);
		if(_postChldExec != NULL) _postChldExec();
	}
}

