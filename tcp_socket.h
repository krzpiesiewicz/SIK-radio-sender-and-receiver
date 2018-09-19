#ifndef SOCKET_H
#define SOCKET_H

#include <sstream>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <functional>
#include <atomic>
#include "err.h"

class TCPsocket;

class SIn {
  std::stringstream ss;
  TCPsocket const &sock;

  /**
   * Waits for a message from client and load it to stringstream,
   * where it will can be accessed by operator>>.
   * Returns true if there is a message from client
   * and false when the connection is closed.
   * @return
   */
  bool load_msg_to_ss();

public:
  SIn() = delete;

  SIn(TCPsocket &sock_) : sock(sock_) {ss.get(); /* make ss EOF */ }

  template<typename T>
  SIn &operator>>(T &t);

  SIn &getline(std::string &buff_str);

  bool eof();
};

class SOut {
  std::stringstream ss;
  TCPsocket const &sock;
public:
  SOut() = delete;

  SOut(TCPsocket &sock_) : sock(sock_) {ss.str("");}


  template<typename T>
  SOut &operator<<(T const &t);

  SOut &operator<<(SOut &(*f)(SOut &));

  /**
   * Sends to client everything got by the output stream since last flushing.
   */
  void send();

  static SOut &flush(SOut &out_s);

  static SOut &endl(SOut &out_s);
};

/**
 * IPv4 socket with stream in and out.
 */
class TCPsocket {
  std::atomic<bool> released = false;
  int sock_num, msg_sock_dsc, buff_size, queue_len, option;
  struct sockaddr_in server_addr, client_addr;
  std::shared_ptr<char[]> buff;
  SIn is;
  SOut os;

  void init_buff();
public:
  TCPsocket(int port_num_, int buff_size_, int queue_len_);

  TCPsocket(int connection_fd_, int buff_size_);

  ~TCPsocket();

  int sock_number() {return sock_num;}

  int msg_sock_descriptor() {return msg_sock_dsc;}

  /**
   * Starts listening for new connections but doesn't answer any.
   */
  void listen_for_connections();

  void accept_connection(int flags);

  /**
   * Accepts a client's connection from queue.
   */
  void accept_connection();

  void accept_nonblocking_connection();

  /**
   * Closes a current conection to client.
   */
  void close_connection();

  /**
   * Releases resources.
   */
  void release();

  /**
   * Returns the stream for writing to client.
   * @return
   */
  SOut &out() {return os;}

  /**
   * Returns the stream for reading from client.
   * @return
   */
  SIn &in() {return is;}

  friend class SIn;

  friend class SOut;
};

template<typename T>
SIn &SIn::operator>>(T &t) {
  if (!eof()) {
    ss >> t;
  }
  return *this;
}

template<typename T>
SOut &SOut::operator<<(T const &t) {
  ss << t;
  return *this;
}

#endif // SOCKET_H
