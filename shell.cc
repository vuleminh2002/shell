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

extern "C" void ctrlC(int sig) {
	//fflush(stdin);
	write(STDOUT_FILENO, "\n", 1);
	Shell::prompt();
}

void Shell::prompt() {
  #ifdef PRINTING
    printf("myshell>");
  #endif
  fflush(stdout);
}

int main() {

  //2.1 ctr+c handler
  struct sigaction sigCtrl;
	sa.sa_handler = ctrlC;
	sigemptyset(&sa.sa_mask);
	sigCtrl.sa_flags = SA_RESTART;

	if (sigaction(SIGINT, &sa, NULL)) {
		perror("sigaction");
		exit(2);
	}


  Shell::prompt();
  yyparse();
}

Command Shell::_currentCommand;
 