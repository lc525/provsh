/*
 * provsh.cpp
 *
 *  Created on: Dec 13, 2012
 *      Author: calucian
 */

#include "provsh.h"
#include "shellcmd.h"
#include "command_separator.hpp"
#include <unistd.h>
#include <iostream>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

#include <readline/readline.h>
#include <readline/history.h>

using namespace provsh;
using namespace std;
using namespace boost::filesystem;

char* convert(const std::string & s)
{
   char *pc = new char[s.size()+1];
   std::strcpy(pc, s.c_str());
   return pc;
}

Shell::Shell(){

	//~ Set up signal handlers
	//
	struct sigaction sig_ign;
	sig_ign.sa_handler = SIG_IGN;
	sigemptyset (&sig_ign.sa_mask);
	sig_ign.sa_flags = 0;

	sigaction (SIGINT, NULL, &oldSIGINT);
	sigaction (SIGTSTP, NULL, &oldSIGTSTP);
	if (oldSIGINT.sa_handler != SIG_IGN)
		sigaction (SIGINT, &sig_ign, NULL);
	if (oldSIGTSTP.sa_handler != SIG_IGN)
		sigaction (SIGTSTP, &sig_ign, NULL);

	//~ History and autocomplete
	//
	rl_bind_key('\t', rl_complete);
	using_history();

	history_size = 500; //TODO lc525: read from configuration file

	char* home_dir = getenv("HOME");
	history_file_pathname = new char[100];
	char bash_history_file[100];

	strcat(history_file_pathname, home_dir);
	strcat(bash_history_file, home_dir);
	strcat(history_file_pathname, "/.provsh_history");
	strcat(bash_history_file, "/.bash_history");

	if(! boost::filesystem::exists(history_file_pathname)){ // try to initialize history from bash
		history_exists = false;
		if(boost::filesystem::exists(bash_history_file))
			read_history(bash_history_file);
	}
	else{
		history_exists = true;
		read_history(history_file_pathname);
	}

	/* TODO lc525: read prompt format from configuration file
	 *
	 * Set default configuration file to .provsh and read prompt from it.
	 * When no prompt is present, use prompt = "[provsh %cwd{1}]$ "; as
	 * default.
	 */
	prompt = "[π:calucian@lunit %cwd{1}]$ ";
	cwd = current_path();
	exit_flag = false;

	initBuiltinCommands();
}

Shell::~Shell(){
	//~ Restore signal handlers
	//
	if (oldSIGINT.sa_handler != SIG_IGN)
		sigaction (SIGINT, &oldSIGINT, NULL);
	if (oldSIGTSTP.sa_handler != SIG_IGN)
		sigaction (SIGTSTP, &oldSIGTSTP, NULL);


	//~ Persist command history
	//
	if(!history_exists){
		write_history(history_file_pathname);
	}
	else{
		append_history(history_size, history_file_pathname);
		history_truncate_file(history_file_pathname, history_size);
	}

	cmdmap::iterator cmdI = internalCommands.begin();
	while(cmdI!=internalCommands.end()){
		RunCommand* ptr = cmdI->second;
		delete ptr;
		cmdI++;
	}

	delete [] history_file_pathname;
}

void Shell::initBuiltinCommands(){
	new ExitCommand(this);
	new CdCommand(this);
}

void Shell::addInternalCommand(std::string cmdname, RunCommand* cmd){
	internalCommands.insert(cmdkvp(cmdname,cmd));
}

std::string Shell::getPrompt(){
	string str_prompt;
	boost::regex cwd_ex( "%cwd(\\{[[:digit:]]+\\})?" );

	boost::smatch what;
	if(boost::regex_search(prompt, what, cwd_ex)){
		int wlen=0;
		if(what.size()>1 && (wlen = what[1].length() ) !=0){ //partial path
			string match = what[1];
			int lev;
			string printcwd;
			std::stringstream levstr(match.substr(1,wlen-1));
			levstr >> lev;
			if(levstr){
				path::iterator cwdI = cwd.end();
				cwdI--;
				string sep="";
				while(lev>0 && cwdI!=cwd.begin()){
					printcwd.insert(0,(*cwdI).string()+sep);
					sep = "/";
					lev--; cwdI--;
				}
				str_prompt = boost::regex_replace(prompt, cwd_ex, printcwd, boost::match_default | boost::format_sed);
			}
			else{
				str_prompt = boost::regex_replace(prompt, cwd_ex, cwd.string(), boost::match_default | boost::format_sed);
			}
		}
		else{ //whole path
			str_prompt = boost::regex_replace(prompt, cwd_ex, cwd.string(), boost::match_default | boost::format_sed);
		}
	}
	else{
		str_prompt = prompt;
	}
	return str_prompt;
}

boost::optional<std::string> Shell::readCommandLine(){
	string prompt;
	string ml_cmdline = "";
	for(;;){ /* runs until line is not continued with \ */
		char* input;
		string cmdline;

		if(ml_cmdline.length() == 0)
			prompt = getPrompt();
		else
			prompt = "> ";
		input = readline(prompt.c_str());

		if(!input) return boost::optional<string>();
		if (*input) add_history(input); // only add to history if non-empty

		cmdline.assign(input);
		free(input);
		boost::algorithm::trim(cmdline);

		int cllen = cmdline.length();
		char lastc = *(cmdline.rbegin());
		if(cllen != 0 && lastc=='\\'){ /* we have a multi-line command */
			ml_cmdline += cmdline.substr(0,cllen-1);
			continue;
		}
		ml_cmdline += cmdline;
		break;
	}

	return boost::optional<string>(ml_cmdline);
}


std::string Shell::replaceEnvVars(std::string cmdline){

	string ret = cmdline;
	boost::regex cwd_env( "\\$(\\w+)" );

	boost::sregex_iterator env_iter(cmdline.begin(), cmdline.end(), cwd_env);
	boost::sregex_iterator end_iter;
	for(;env_iter!=end_iter; env_iter++){
		string env_var_name = (*env_iter)[1];
		char* repl = getenv(env_var_name.c_str());
		boost::regex cwd_env_var("\\$"+env_var_name);
		ret = boost::regex_replace(ret, cwd_env_var, repl, boost::match_default | boost::format_sed);
	}
	return ret;

}


void Shell::parseCommandLine(std::string cmdline){
	typedef boost::tokenizer<boost::command_separator<char> > Tok_esc;
	std::string separators(";&|()");
	boost::command_separator<char> cmdsep("\\", separators, "\"\'", true);

	vector<string> tokens;

	Tok_esc cmd_tok(cmdline, cmdsep);

	/*
	 * Concatenate successive separators (such as && or ||)
	 */
	Tok_esc::iterator ti=cmd_tok.begin();
	string sti=*ti;
	boost::algorithm::trim(sti);
	tokens.push_back(sti);
	ti++;
	unsigned int i=0;
	bool sep = false;
	for(; ti!=cmd_tok.end(); ti++){
		sti = *ti;
		if(sti.length()==1 && separators.find(sti)!=string::npos){
			if(sep) tokens[i]+=sti;
			else {
				tokens.push_back(sti);
				i++;
			}
			sep = !sep;
		}
		else{
			if(sti.length()>0){
				boost::algorithm::trim(sti);
				tokens.push_back(sti);
				i++;
				sep = false;
			}
		}
	}

	for(i=0; i<tokens.size(); i++){
		cout<<tokens[i]<<endl;
	}
}

void Shell::executeCommand(std::string cmdline){
	cmdline = replaceEnvVars(cmdline);
	typedef boost::tokenizer<boost::char_separator<char> > Tok;
	typedef boost::tokenizer<boost::escaped_list_separator<char> > Tok_esc;
	parseCommandLine(cmdline);

	// separate multiple commands on the same line (pipes, continuations)
	char cmdsep[] = "|&;";
	boost::char_separator<char> sep("", cmdsep);
	Tok tok(cmdline, sep);
	vector<string> cmdlines;
	cmdlines.assign(tok.begin(), tok.end());

	RunCommand* cmd = NULL;

	for(size_t i=0; i<cmdlines.size(); i++){
		string onecmd = cmdlines[i];
		boost::algorithm::trim(onecmd);
		if(onecmd.find_first_of(cmdsep) == string::npos){

			// split the command and its arguments
			boost::escaped_list_separator<char> onecmdsep(""," ","\"\'");
			Tok_esc onecmd_tok(onecmd, onecmdsep);

			string cmdName;

			vector<string> vec;
			vector<char*>* args = new vector<char*>();
			vec.assign(onecmd_tok.begin(), onecmd_tok.end());

			int argc = vec.size();
			if(argc == 0) continue;

			cmdName = vec[0];
			std::transform(vec.begin(), vec.end(), std::back_inserter(*args), convert);
			args->push_back(NULL);

			cmdmap::iterator cmdI = internalCommands.find(cmdName);
			if(cmdI != internalCommands.end()){
				cmd = cmdI->second;
				cmd->setArgs(argc, &(*args)[0]);
			}
			else{// not a builtin command
				cmd = new ExecCommand(this);
				cmd->setArgs(argc, &(*args)[0]);
			}

		}
	}

	if(cmd!=NULL){
		cmd->execute();
		if(!cmd->isBuiltin) delete cmd;
	}
}

bool Shell::shouldExit(){
	return exit_flag;
}

void Shell::setExit(){
	exit_flag = true;
}


