/*
 * provsh.h
 *
 *  Created on: Dec 13, 2012
 *      Author: calucian
 */

#ifndef PROVSH_H_
#define PROVSH_H_

#include <string>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <map>
#include <signal.h>


namespace provsh{
	class RunCommand;

	class Shell{
		typedef std::map<std::string, RunCommand*> cmdmap;
		typedef std::pair<std::string, RunCommand*> cmdkvp;

		friend class RunCommand;
	public:
		boost::filesystem::path cwd;
	private:
		cmdmap internalCommands;
		std::string prompt;

		int history_size;
		bool history_exists;
		char* history_file_pathname;

		bool exit_flag;
		struct sigaction oldSIGTSTP, oldSIGINT;

		void initBuiltinCommands();
		void addInternalCommand(std::string cmdname, RunCommand* cmd);

	public:
		Shell();
		~Shell();

		std::string getPrompt();
		boost::optional<std::string> readCommandLine();
		std::string replaceEnvVars(std::string cmdline);
		void parseCommandLine(std::string cmdline);
		void executeCommand(std::string cmdline);
		bool shouldExit();
		void setExit();

	};

}


#endif /* PROVSH_H_ */
