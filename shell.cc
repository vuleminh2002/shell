#include <cstdio>

#include "shell.hh"
#ifdef PRINTING
int yyparse(void);

void Shell::prompt() {
  printf("myshell>");
  fflush(stdout);
}

int main() {
  Shell::prompt();
  yyparse();
}

Command Shell::_currentCommand;
  #endif  