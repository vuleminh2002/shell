

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <regex.h>
#include <assert.h>
#include "command.hh"
void yyerror(const char * s);
int yylex();
static int cmpfunc(const void *a, const void *b);
void expandWildCardsIfNecessary(char *arg);
void expandWildCards(char *prefix, char *suffix);
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
    if (strcmp(Command::_currentSimpleCommand->_arguments[0]->c_str(), "echo") == 0 && strchr((char *)$1->c_str(), '?'))
      #ifdef PRINTING
        printf("   Yacc: insert argument \"%s\"\n", $1->c_str());
      #endif
      Command::_currentSimpleCommand->insertArgument( $1 );
    else
		  expandWildCardsIfNecessary((char *)$1->c_str());
  }
  ;

command_word:
  WORD {
  //2.3: Exit
	if ( *$1 == "exit") {
    #ifdef PRINTING
		  printf("Good Bye!!\n");
    #endif
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

// Global or static variables for Part 1
int maxEntries = 20;
int nEntries = 0;
char **entries = NULL;

// A helper compare function for qsort (or you can use C++ std::sort)
static int cmpfunc(const void *a, const void *b) {
    char *s1 = *(char**)a;
    char *s2 = *(char**)b;
    return strcmp(s1, s2);
}

// Convert wildcard pattern to a posix regex (e.g. '*.c' -> '^.*\.c$')
static char *wildcardToRegex(const char *arg) {
    // Enough space: each char might become 2+ chars in worst case
    char *reg = (char*)malloc(2*strlen(arg) + 10);
    char *r = reg;
    const char *a = arg;

    *r = '^'; r++;  // start anchor
    while (*a) {
        if (*a == '*') {
            *r = '.'; r++;
            *r = '*'; r++;
        } else if (*a == '?') {
            *r = '.'; r++;
        } else if (*a == '.') {
            *r = '\\'; r++;
            *r = '.'; r++;
        } else {
            *r = *a; r++;
        }
        a++;
    }
    *r = '$'; r++;
    *r = 0;   // null terminate
    return reg;
}

// The actual expansion function. For Part 1, we ignore 'prefix' (if any)
void expandWildCards(const std::string &prefix, const std::string &suffix)
{
    if (suffix.empty()) {
        if (nEntries == maxEntries) {
            maxEntries *= 2;
            entries = (char**)realloc(entries, maxEntries * sizeof(char*));
            assert(entries);
        }
        entries[nEntries++] = strdup(prefix.c_str());
        return;
    }

    size_t slashPos = suffix.find('/');
    std::string component = (slashPos == std::string::npos) ? suffix : suffix.substr(0, slashPos);
    std::string rest = (slashPos == std::string::npos) ? "" : suffix.substr(slashPos + 1);

    std::string dir = prefix.empty() ? "." : prefix;
    DIR *dp = opendir(dir.c_str());
    if (!dp) return;

    regex_t re;
    char *regexStr = wildcardToRegex(component.c_str());
    if (regcomp(&re, regexStr, REG_EXTENDED | REG_NOSUB) != 0) {
        free(regexStr);
        closedir(dp);
        return;
    }
    free(regexStr);

    struct dirent *entry;
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.' && component[0] != '.') continue;
        if (regexec(&re, entry->d_name, 0, NULL, 0) == 0) {
            std::string newPrefix = prefix.empty() ? entry->d_name : prefix + "/" + entry->d_name;
            expandWildCards(newPrefix, rest);
        }
    }
    regfree(&re);
    closedir(dp);
}

// The user-facing function
void expandWildCardsIfNecessary(char *arg) {
   nEntries = 0;
    maxEntries = 20;
    entries = (char**)malloc(maxEntries * sizeof(char*));

    std::string path(arg);
    if (path.find('*') != std::string::npos || path.find('?') != std::string::npos) {
        if (path[0] == '/') expandWildCards("/", path.substr(1));
        else expandWildCards("", path);

        qsort(entries, nEntries, sizeof(char*), cmpfunc);

        for (int i = 0; i < nEntries; ++i) {
            Command::_currentSimpleCommand->insertArgument(new std::string(entries[i]));
            free(entries[i]);
        }
    } else {
        Command::_currentSimpleCommand->insertArgument(new std::string(arg));
    }
    free(entries);
}


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




