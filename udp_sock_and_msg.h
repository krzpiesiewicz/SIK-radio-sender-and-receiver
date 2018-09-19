#ifndef UDP_SOCK_AND_MSG_H
#define UDP_SOCK_AND_MSG_H

#include <memory>
#include <queue>
#include <chrono>
#include <cstddef>
#include <netinet/in.h>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

namespace {
  using namespace std;

  chrono::system_clock::time_point
  get_curr_time() {return chrono::system_clock::now();}
}

struct Msg {
  std::size_t data_len;
  shared_ptr<std::byte[]> data;
  std::chrono::system_clock::time_point time;

  Msg() = default;

  Msg(std::size_t data_len_,
      std::chrono::system_clock::time_point time_ = get_curr_time()) :
    data_len(data_len_),
    data(new std::byte[data_len], std::default_delete<byte[]>()),
    time(time_) {
//    data = new byte[data_len];
  }

  Msg(std::shared_ptr<std::byte[]> data_, std::size_t data_len_,
      std::chrono::system_clock::time_point time_ = get_curr_time()) :
    data_len(data_len_), data(data_), time(time_) {}

  inline byte* get_data() {
    return data.get();
  }

  inline const byte *get_data() const {
    return data.get();
  }
};

struct MsgFrom : Msg {
  sockaddr_in src_addr;

  MsgFrom(std::shared_ptr<std::byte[]> data_, std::size_t data_len_,
          sockaddr_in src_addr_,
          std::chrono::system_clock::time_point time_ = get_curr_time()) :
    Msg(data_, data_len_, time_), src_addr(src_addr_) {}
};

std::string sin_addr_to_string(in_addr sa);

sockaddr_in create_sockaddr_in(std::string address, int port_num);

class UDPsocket {
  const static bool DB = false;
  int sock_num;
  bool multicast_receiving, closing = false;
public:
  ip_mreq ipmreq;
  sockaddr_in local_addr, remote_addr;
public:
  UDPsocket();

  ~UDPsocket();

  int num() {return sock_num;}

  sockaddr_in get_local_addr();

  void set_local_port(int port);

  void bind_to_any_port();

  void set_reusing_address();

  void set_reusing_port();

  void connect_to(sockaddr_in remote_addr_);

  void disconnect();

  void add_multicast_sending(sockaddr_in remote_addr_);

  void add_multicast_receiving(sockaddr_in remote_addr_);

  void add_broadcast();

  void disable_multicast_receiving();

  void closeSock();

  void send_msg(const Msg &msg);

  void send_msg_to(const Msg &msg, const sockaddr_in & dst_addr);
};

#endif // UDP_SOCK_AND_MSG_H
