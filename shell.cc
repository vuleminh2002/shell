#include <cstdio>
#include <cstdio>       // for printf
#include <cstdlib>      // for exit
#include <csignal>      // for sigaction, SIGINT, etc.
#include <unistd.h>     // for write(), STDOUT_FILENO, isatty()
#include <sys/types.h>  // for pid_t (optional but good practice)
#include <sys/wait.h>  
#include "shell.hh"
#include <regex>
#include <string>
#include <iostream>
//void yyrestart(FILE * file);
int yyparse(void);
int Shell::_lastStatus = 0;
int Shell::_lastBackgroundPid = -1;
std::string Shell::_lastArg = "";

std::string expandAllEnv(const std::string &input);
std::string lookupVar(const std::string &varName);

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
 std::string expandAllEnv(const std::string &input) {
    // We apply expansions in a loop until no more matches found.
    // This ensures multiple expansions in one token are replaced.
    std::string result = input;

    // Pattern A: ${var}
    std::regex curlyPattern("\\$\\{([^}]+)\\}");

    bool changed = true;
    while (changed) {
        changed = false;

            std::smatch match;
            if (std::regex_search(result, match, curlyPattern)) {
                // e.g. match[1] => the var name
                std::string varName = match[1].str();
                std::string expansion = lookupVar(varName);

                // Replace first occurrence
                result.replace(match.position(0), match.length(0), expansion);
                changed = true;  // We made at least one replacement
            }
    }

    return result;
}
 std::string lookupVar(const std::string &varName) {
    // This function decides how to interpret each var:
    // e.g., $! => Shell::_lastBackgroundPid, $? => Shell::_lastStatus, ...
    if (varName == "!") {
        // last background PID
        return std::to_string(Shell::_lastBackgroundPid);
    } else if (varName == "?") {
        return std::to_string(Shell::_lastStatus);
    } else if (varName == "_") {
        return Shell::_lastArg;
    } else if (varName == "SHELL") {
        char path[1024];
        if (realpath("/proc/self/exe", path)) {
            return std::string(path);
        } else {
            return "/proc/self/exe"; // fallback
        }
    } else {
        // a normal environment variable
        const char *val = getenv(varName.c_str());
        return val ? val : "";
    }
}




bool Shell::_isSubshell = false;

void Shell::prompt() {
	if (true) {
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
	//Only analyszw signal if background flag is true
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
 