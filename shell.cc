#include <cstdio>

#include "shell.hh"

int yyparse(void);

void Shell::prompt() {
  #ifdef PRINTING
    printf("myshell>");
  #ifdef PRINTING
  fflush(stdout);
}

int main() {
  Shell::prompt();
  yyparse();
}

Command Shell::_currentCommand;
 