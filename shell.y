

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
    if (strcmp(Command::_currentSimpleCommand->_arguments[0]->c_str(), "echo") == 0 && strchr($1, '?'))
      #ifdef PRINTING
        printf("   Yacc: insert argument \"%s\"\n", $1->c_str());
      #endif
      Command::_currentSimpleCommand->insertArgument( $1 );
    else
		  expandWildCardsIfNecessary($1);
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
void expandWildCards(char *prefix, char *suffix)
{
    // 1) Convert 'suffix' to a regex
    char *regstr = wildcardToRegex(suffix);

    // Compile
    regex_t re;
    int ret = regcomp(&re, regstr, REG_EXTENDED | REG_NOSUB);
    free(regstr);
    if (ret != 0) {
        // If compile fails, fallback
        return; 
    }

    // 2) Open current directory
    DIR *dir = opendir(".");
    if (!dir) {
        perror("opendir");
        regfree(&re);
        return;
    }

    // 3) readdir + regexec
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // skip "." and ".." if you want
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (regexec(&re, entry->d_name, 0, NULL, 0) == 0) {
            // match
            // store into 'entries' array
            if (nEntries == maxEntries) {
                maxEntries *= 2;
                entries = (char**)realloc(entries, maxEntries * sizeof(char*));
                assert(entries);
            }
            entries[nEntries] = strdup(entry->d_name); 
            nEntries++;
        }
    }
    closedir(dir);
    regfree(&re);
}

// The user-facing function
void expandWildCardsIfNecessary(char *arg) {
    // Initialize / reset global array 
    maxEntries = 20;
    nEntries = 0;
    entries = (char**)malloc(maxEntries * sizeof(char*));

    if (strchr(arg, '*') || strchr(arg, '?')) {
        // Has wildcard => expand
        expandWildCards(NULL, arg);

        if (nEntries == 0) {
            // No matches => fallback to literal
            Command::_currentSimpleCommand->insertArgument(strdup(arg));
        } else {
            // sort
            qsort(entries, nEntries, sizeof(char*), cmpfunc);

            // insert each
            for (int i = 0; i < nEntries; i++) {
                Command::_currentSimpleCommand->insertArgument(entries[i]);
            }
        }
        free(entries);
    } else {
        // no wildcard => just insert directly
        Command::_currentSimpleCommand->insertArgument(strdup(arg));
    }
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




