

/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%code requires 
{
#include <string>

#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif
}

%union
{
  char        *string_val;
  // Example of using a c++ type in yacc
  std::string *cpp_string;
}

%token <cpp_string> WORD
%token NOTOKEN GREAT NEWLINE GREAT2 GREATAMP GREATGREAT GREATGREATAMP PIPE LESS AMPERSAND

%{
//#define yylex yylex
#include <cstdio>
#include "shell.hh"

void yyerror(const char * s);
int yylex();

%}

%%

goal:
  commands
  ;

commands:
  command
  | commands command
  ;

command: simple_command
       ;

simple_command:	
  pipline iomodifier_opt_list background NEWLINE {
    #ifdef PRINTING
      printf("   Yacc: Execute command\n");
    #endif
    Shell::_currentCommand.execute();
  }
  | NEWLINE 
  | error NEWLINE { yyerrok; }
  ;

pipline:
  pipline PIPE command_and_args
  | command_and_args
  ;

command_and_args:
  command_word argument_list {
    Shell::_currentCommand.
    insertSimpleCommand( Command::_currentSimpleCommand );
  }
  ;

/* Zero or more redirections, in any order */
iomodifier_opt_list:
  iomodifier_opt_list iomodifier_opt
  | iomodifier_opt
  | /* can be empty */
  ;

argument_list:
  argument_list argument
  | /* can be empty */
  ;

argument:
  WORD {
    #ifdef PRINTING
      printf("   Yacc: insert argument \"%s\"\n", $1->c_str());
    #endif
    Command::_currentSimpleCommand->insertArgument( $1 );\
  }
  ;

command_word:
  WORD {
  //2.3: Exit
	if ( *$1, "exit" == 0 ) {
		printf("Good Bye!!\n");
		exit(1);
	}
    #ifdef PRINTING
      printf("   Yacc: insert command \"%s\"\n", $1->c_str());
    #endif
    Command::_currentSimpleCommand = new SimpleCommand();
    Command::_currentSimpleCommand->insertArgument( $1 );
  }
  ;

/* Optional background execution */

background:
/* empty */ {
      Shell::_currentCommand._background = false;
    }
  | AMPERSAND {
      Shell::_currentCommand._background = true;
    }
  ;

/* List of possible modifier */
iomodifier_opt:
  GREAT WORD {
    if (Shell::_currentCommand._outFile != NULL ){
      
        fprintf(stderr, "Ambiguous output redirect.\n");
      
		    exit(0);
	  }
    #ifdef PRINTING
      printf("   Yacc: insert output \"%s\"\n", $2->c_str());
    #endif
      Shell::_currentCommand._outFile = $2;
    }
  
  |
  GREATGREATAMP WORD {
      if (Shell::_currentCommand._outFile != NULL ){
		    printf("Ambiguous output redirect.\n");
		    exit(0);
	  }
    #ifdef PRINTING
      printf("   Yacc: insert stdout & stderr (append) to \"%s\"\n", $2->c_str());
    #endif
      Shell::_currentCommand._outFile = $2;
      Shell::_currentCommand._errFile = $2;
      Shell::_currentCommand._append = true;
    }
  |
  GREATGREAT WORD {
      if (Shell::_currentCommand._outFile != NULL ){
		    printf("Ambiguous output redirect.\n");
		    exit(0);
	  }
    #ifdef PRINTING
      printf("   Yacc: insert output \"%s\" (append)\n", $2->c_str());
    #endif
      Shell::_currentCommand._outFile = $2;
      Shell::_currentCommand._append = true;
    }
  |
  GREATAMP WORD {
      if (Shell::_currentCommand._outFile != NULL ){
		    printf("Ambiguous output redirect.\n");
		    exit(0);
	  }
    #ifdef PRINTING
      printf("   Yacc: insert stdout & stderr to \"%s\"\n", $2->c_str());
    #endif
      Shell::_currentCommand._outFile = $2;
      Shell::_currentCommand._errFile = $2; 
  }
  |
  GREAT2 WORD {
    #ifdef PRINTING
      printf("   Yacc: insert stderr redirection \"%s\"\n", $2->c_str());
    #endif
      Shell::_currentCommand._errFile = $2;
    }
  |
  LESS WORD {
      if (Shell::_currentCommand._inFile != NULL ){
		    printf("Ambiguous output redirect.\n");
		    exit(0);
	  }
    #ifdef PRINTING
      printf("   Yacc: insert input \"%s\"\n", $2->c_str());
    #endif
      Shell::_currentCommand._inFile = $2;
    }
  ;


%%

void
yyerror(const char * s)
{
#ifdef PRINTING
  fprintf(stderr,"%s", s);
#endif
}

#if 0
main()
{
  yyparse();
}
#endif




