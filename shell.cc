#include <cstdio>
#include <cstdio>       // for printf
#include <cstdlib>      // for exit
#include <csignal>      // for sigaction, SIGINT, etc.
#include <unistd.h>     // for write(), STDOUT_FILENO, isatty()
#include <sys/types.h>  // for pid_t (optional but good practice)
#include <sys/wait.h>  
#include "shell.hh"
void yyrestart(FILE * file);
int yyparse(void);



void Shell::prompt() {
  #ifdef PRINTING
    printf("myshell>");
  #endif
  fflush(stdout);
}

int main() {


  Shell::prompt();
  yyparse();
}

Command Shell::_currentCommand;
 