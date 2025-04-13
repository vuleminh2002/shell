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
  static std::string expandAllEnv(const std::string &input);
  static std::string lookupVar(const std::string &varName);
};

#endif
