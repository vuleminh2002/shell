#ifndef shell_hh
#define shell_hh

#include "command.hh"

struct Shell {

  static void prompt();
  static bool _isSubshell;
  static Command _currentCommand;
  static int _lastStatus;
  static int _lastBackgroundPid;
  static std::string _lastArg;
};

#endif
