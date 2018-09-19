#ifndef RADIO_CHANNEL_H
#define RADIO_CHANNEL_H

#include <string>
#include <chrono>
#include <netinet/in.h>

struct RadioChannel {
  std::string name, address;
  int port;
  mutable bool is_new = true;
  mutable sockaddr_in audio_addr;
  mutable sockaddr_in ctrl_addr;
  mutable std::chrono::system_clock::time_point last_reply;

  RadioChannel(std::string name_, std::string address_, int port_, sockaddr_in
  audio_addr_, sockaddr_in ctrl_addr_,
               std::chrono::system_clock::time_point time_) :
    name(name_), address(address_), port(port_), audio_addr(audio_addr_),
    ctrl_addr(ctrl_addr_), last_reply(time_) {}

  inline bool operator==(RadioChannel const &another) {
    return name == another.name && address == another.address
           && port == another.port;
  }

  struct cmp {
    inline bool operator()(RadioChannel const &a, RadioChannel const &b)
    const {
      if (a.name == b.name) {
        if (a.address == b.address)
          return a.port < b.port;
        return a.address < b.address;
      }
      return a.name < b.name;
    }
  };
};

#endif // RADIO_CHANNEL_H
