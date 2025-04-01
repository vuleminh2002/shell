/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 * DO NOT PUT THIS PROJECT IN A PUBLIC REPOSITORY LIKE GIT. IF YOU WANT 
 * TO MAKE IT PUBLICALLY AVAILABLE YOU NEED TO REMOVE ANY SKELETON CODE 
 * AND REWRITE YOUR PROJECT SO IT IMPLEMENTS FUNCTIONALITY DIFFERENT THAN
 * WHAT IS SPECIFIED IN THE HANDOUT. WE OFTEN REUSE PART OF THE PROJECTS FROM  
 * SEMESTER TO SEMESTER AND PUTTING YOUR CODE IN A PUBLIC REPOSITORY
 * MAY FACILITATE ACADEMIC DISHONESTY.
 */

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>     // open, O_ flags
#include <sys/wait.h>
#include <iostream>
#include <unistd.h>
#include "command.hh"
#include "shell.hh"
#include <sys/types.h>


Command::Command() {
    // Initialize a new vector of Simple Commands
    _simpleCommands = std::vector<SimpleCommand *>();
    _append = false;
    _outFile = NULL;
    _inFile = NULL;
    _errFile = NULL;
    _background = false;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
    // add the simple command to the vector
    _simpleCommands.push_back(simpleCommand);
}

void Command::clear() {
    // deallocate all the simple commands in the command vector
  

    // remove all references to the simple commands we've deallocated
    // (basically just sets the size to 0)
    _simpleCommands.clear();

    for (auto simpleCommand : _simpleCommands) {
        delete simpleCommand;
    }
    if (_errFile == _outFile) {
        // free it once
        delete _outFile;
        _outFile = nullptr;
        _errFile = nullptr;
    } else {
        // free them separately
        if (_outFile) { delete _outFile; }
        _outFile = nullptr;
        if (_errFile) { delete _errFile; }
        _errFile = nullptr;
    }
   
    if ( _inFile ) {
        delete _inFile;
    }
    _inFile = NULL;


    
    _append = false;
    _background = false;
}

void Command::print() {
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    int i = 0;
    // iterate over the simple commands and print them nicely
    for ( auto & simpleCommand : _simpleCommands ) {
        printf("  %-3d ", i++ );
        simpleCommand->print();
    }

    printf( "\n\n" );
    printf( "  Output       Input        Error        Background\n" );
    printf( "  ------------ ------------ ------------ ------------\n" );
    printf( "  %-12s %-12s %-12s %-12s\n",
            _outFile?_outFile->c_str():"default",
            _inFile?_inFile->c_str():"default",
            _errFile?_errFile->c_str():"default",
            _background?"YES":"NO");
    printf( "\n\n" );
}

bool Command::builtIn(int i) {
    // If the first argument is "printenv"

    if (strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "printenv") == 0) {


        for (char **env = environ; *env; env++) {
            printf("%s\n", *env);
        }
        clear();
        Shell::prompt();
        return true;  // means "handled"
    }

    // check other built-ins: cd, setenv, etc.

    return false; // not handled
}

void Command::execute() {
    // Don't do anything if there are no simple commands
    if ( _simpleCommands.size() == 0 ) {
        Shell::prompt();
        return;
    }
    
    if (builtIn(0)) {
        // builtIn handled it in the parent; no need to fork
        return;
    }
    // Print contents of Command data structure
    #ifdef PRINTING
        print();
    #endif
    
    // Add execution here
    // For every simple command fork a new process
    // Setup i/o redirection
    // and call exec
    //1b.1
    int defaultin = dup(0);
    int defaultout = dup(1);
    int defaulterr = dup(2);

    int fdin  = 0;  // Will hold next command's input source
    int fdout = 0;  // Will hold next command's output destination
    int fderr = 0;  // fd index that holds err source


    //Step 1: Setting up input
    if (_inFile) {
        fdin = open(_inFile->c_str(), O_RDONLY);
        if (fdin < 0) {
        }
    }
    else {

        //if no input file, use the default input
        fdin = dup(defaultin);
    }
 
    //Step 2: setup error 
    if (_errFile) {
        // if we have an error file, open for append or truncate
        if (_append) {
            fderr = open(_errFile->c_str(),
                         O_WRONLY | O_CREAT | O_APPEND,
                         0600);
        } else {
            fderr = open(_errFile->c_str(),
                         O_WRONLY | O_CREAT | O_TRUNC,
                         0600);
        }
        if (fderr < 0) {
            perror("open (error)");
            // again, handle or continue
        }
    } else {
        // No error file -> default error
        fderr = dup(defaulterr);
    }
    dup2(fderr, 2);
    close(fderr);

    int lastPid = -1;

    //step 3: Loop over simpleCommand
    for (size_t i = 0; i < _simpleCommands.size(); i++) {
        //printf("got into the loop baby");
        dup2(fdin, 0);
        close(fdin);
        //if last SimpleCommand (process the output)
        if ( i == _simpleCommands.size() - 1) {
            if (_outFile) {
                if (_append) {
                    fdout = open (_outFile->c_str(), O_WRONLY | O_CREAT | O_APPEND, 0600);
                }
                 else {
                    fdout = open(_outFile->c_str(),
                                O_WRONLY | O_CREAT | O_TRUNC,
                                0600);
                }
                if (fdout < 0) {
                    perror("open (output)");
                }
            } else {
                // No output file -> use default stdout
                fdout = dup(defaultout);
                }
            }
            else {//part 1b.3
                 // Not the last command => create a pipe
                int fdpipe[2];
                if (pipe(fdpipe) == -1) {
                    perror("pipe");
                    exit(2);
            }
                fdout = fdpipe[1];   // current command writes to fdpipe[1]
                fdin  = fdpipe[0];    //assign fdin for the next command
            }

        dup2(fdout, 1);
        close(fdout); 
      

        //Step 4: fork a child.
        lastPid = fork();

        if (lastPid < 0) {
            //if error occurs on fork
            perror("fork");
            exit(2);
        }
        if (lastPid == 0) {
            // CHILD PROCESS
            signal(SIGINT, SIG_DFL); // handle ctr+c kill child process
            // Convert arguments from std::vector<std::string*> to char* array
            SimpleCommand *scmd = _simpleCommands[i];
            size_t argCount = scmd->_arguments.size();
            char **argv = new char*[argCount + 1];
            for (size_t j = 0; j < argCount; j++) {
                argv[j] = const_cast<char*>(scmd->_arguments[j]->c_str());
            }
            argv[argCount] = nullptr;
            
            // execvp
            execvp(argv[0], argv);
            perror("execvp");
            _exit(1);
        }
    }

    // 6) Restore in/out/err to defaults
    dup2(defaultin, 0);
    dup2(defaultout, 1);
    dup2(defaulterr, 2);

    close(defaultin);
    close(defaultout);
    close(defaulterr);

    // 7) If not background, wait for last command

    if (!_background) {
        waitpid(lastPid, nullptr, 0);
    }

    
    // Clear to prepare for next command
    clear();

    // Print new prompt
    Shell::prompt();
}

SimpleCommand * Command::_currentSimpleCommand;
