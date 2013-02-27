/*
 * cmdparser.h
 *
 *  Created on: Jan 25, 2013
 *      Author: calucian
 */

#ifndef CMDPARSER_H_
#define CMDPARSER_H_

namespace provsh{
namespace parser{


enum AST_TYPE{
	OP,
	COMMAND
};

struct ASTNode{
	AST_TYPE 		type;
	std::string 	text;
	bool			subshell_root;

	ASTNode* left;
	ASTNode* right;
};


class Parser{
	int op_precedence(std::string c){
		if(c == "&&" || c == ";" || c=="||") return 3;
		if(c == "|") return 2;
		if(c == ">" || c == ">>" || c=="<") return 1;
		return -1;
	}


};



}} /* }parser }provsh  */


#endif /* CMDPARSER_H_ */
