#ifndef shell_hh
#define shell_hh

#include "command.hh"

struct Shell {

  static void prompt();
  static bool _isSubshell;
  Shell::_lastStatus;
  static Command _currentCommand;
  Shell::_lastBackgroundPid;
  Shell::_lastArg;
};

#endif
