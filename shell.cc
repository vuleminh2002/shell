#include <cstdio>
#include <cstdio>       // for printf
#include <cstdlib>      // for exit
#include <csignal>      // for sigaction, SIGINT, etc.
#include <unistd.h>     // for write(), STDOUT_FILENO, isatty()
#include <sys/types.h>  // for pid_t (optional but good practice)
#include <sys/wait.h>  
#include "shell.hh"
//void yyrestart(FILE * file);
int yyparse(void);
//function to handle signal interupt ctr+c
extern "C" void ctrlC(int sig) {
	//fflush(stdin);
	write(STDOUT_FILENO, "\n", 1);
	Shell::prompt();
}

extern "C" void zombieHandler(int sig){
  int status;
  int pid = wait3(0, 0, NULL);

	while (waitpid(-1, NULL, WNOHANG) > 0) {};
	  //printf("[%d] exited.\n", pid);
}



bool Shell::_isSubshell = false;

void Shell::prompt() {
	if (isatty(0)) {
	#ifdef PRINTING
		printf("myshell>");
	#endif
		fflush(stdout);
}
}

int main() {

  //2.1 ctr+c handler
  struct sigaction sa;
	sa.sa_handler = ctrlC;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if (sigaction(SIGINT, &sa, NULL)) {
		perror("sigaction");
		exit(2);
	}

//2.2: Zombie sigaction
	//TODO: Only analysize signal if background flag is true
	struct sigaction sigZombie;

		sigZombie.sa_handler = zombieHandler;
		sigemptyset(&sigZombie.sa_mask);
		sigZombie.sa_flags = SA_RESTART | SA_NOCLDSTOP;
		if (sigaction(SIGCHLD, &sigZombie, NULL)) {
			perror("sigaction");
			exit(-1);
		}


  Shell::prompt();
  yyparse();
}

Command Shell::_currentCommand;
 