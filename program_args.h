#ifndef PROGRAM_ARGS_H
#define PROGRAM_ARGS_H

#include <string>
#include <list>
#include <memory>
#include <unordered_map>
#include <iostream>
#include "err.h"

class ArgumentException : public ServerException {
public:
  ArgumentException(std::string msg_) : ServerException(msg_) {}
};

struct ProgramArgs {
  std::unordered_map<std::string, std::string> umap;
  std::list<std::string> singles;
  std::string usage_txt;

  ProgramArgs() = default;

  ProgramArgs(int argc, char *argv[], std::string usage_txt_) :
    usage_txt(usage_txt_) {
    for (int i = 0; i < argc; i++) {
      if (argv[i][0] == '-') {
        if (i + 1 == argc || argv[i + 1][0] == '-')
          throw ArgumentException(usage_txt + "No value for option " +
                                  std::string((argv[i] + 1)) + ".");
        umap[std::string(&argv[i][1])] = std::string(argv[i + 1]);
        i++;
      } else {
        singles.push_back(argv[i]);
      }
    }
  }

  std::string *val_for_opt(std::string const &opt) {
    auto it = umap.find(opt);
    if (it != umap.end())
      return &(it->second);
    else
      return nullptr;
  }

  std::string val_for_opt(std::string const &opt,
                          std::string const &defalut) {
    auto val = val_for_opt(opt);
    return val == nullptr ? defalut : *val;
  }

  std::string force_val_for_opt(std::string const &opt) {
    auto val = val_for_opt(opt);
    if (val == nullptr)
      throw ArgumentException(usage_txt);
    return *val;
  }

  int intval_for_opt(std::string const &opt, int default_val) {
    try {
      return std::stoi(val_for_opt(opt, std::to_string(default_val)));
    } catch (std::invalid_argument const &e) {
      throw ArgumentException(usage_txt + " Value for option -" + opt +
                              "must be integral.");
    }
  }

  int force_intval_for_opt(std::string const &opt) {
    try {
      return std::stoi(force_val_for_opt(opt));
    } catch (std::invalid_argument const &e) {
      throw ArgumentException(usage_txt + " Value for option -" + opt +
                              "must be integral.");
    }
  }
};

#endif // PROGRAM_ARGS_H
