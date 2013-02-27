/*
 * shellcmd.h
 *
 *  Created on: Dec 13, 2012
 *      Author: calucian
 */

#ifndef SHELLCMD_H_
#define SHELLCMD_H_
#include <string>
#include <vector>
#include "provsh.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <iostream>


namespace provsh{
	class RunCommand{
	public:
		bool isBuiltin;
	protected:
		Shell* master;
		std::string _cmdtok;
		int _argc;
		char** _argv;
	public:
		RunCommand(Shell* sh, const std::string cmdtok, bool isBuiltin=false);
		virtual ~RunCommand(){}
		virtual void execute() = 0;
		inline void setArgs(int argc, char** argv){ _argc = argc; _argv=argv; }
	};

	struct CommandItem{
		RunCommand* cmd;
		std::string combine;
		CommandItem* next;
	};

	class Commands{
		std::vector<std::string> cmds;
		std::vector<std::string> links;
	};

	class CompositeCommand : public RunCommand{
	private:
		CommandItem* first;
	public:
		CompositeCommand(Shell* s): RunCommand(s, "__internalComposite", false){ first = NULL; }
		virtual void execute();
		void add(RunCommand* c, std::string combine);
	};



	class ExitCommand : public RunCommand{
	public:
		ExitCommand(Shell* s) : RunCommand(s, "exit", true){};

		inline virtual void execute(){
			master->setExit();
		}
	};


	class CdCommand : public RunCommand{
	public:
		CdCommand(Shell* s) : RunCommand(s, "cd", true){};
		virtual void execute();
	};

	/* TODO lc525: implement command aliasing
	 *
	 * Enable aliases to be read from a configuration file and mapped to arbitrary commands.
	 * e.g.: alias cd..="cd .."
	 */


	class ExecCommand : public RunCommand{
		typedef void (*ExecHookType)();

		int childPid;
		bool _waitChild;
		ExecHookType _preForkExec;
		ExecHookType _preChldExec;
		ExecHookType _postChldExec;
	public:
		ExecCommand(Shell* s,
				bool waitChild = true,
				void (*preForkExec)() = NULL,
				void (*preChldExec)() = NULL,
				void (*postChldExec)() = NULL)
		: RunCommand(s, "__internalExecCommand", false){
			childPid = 0;
			_waitChild = waitChild;
			_preForkExec = preForkExec;
			_preChldExec = preChldExec;
			_postChldExec = postChldExec;
		}
		virtual void execute();
	};

}


#endif /* SHELLCMD_H_ */
