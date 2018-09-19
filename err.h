#ifndef ERR_H
#define ERR_H

#include <string>
#include <memory>
#include <list>

class ServerException : public std::exception {
  std::string msg;
public:
  ServerException(std::string msg_) : msg(msg_) {}

  const char *what() const noexcept override {
    return msg.c_str();
  }
};

#endif // ERR_H