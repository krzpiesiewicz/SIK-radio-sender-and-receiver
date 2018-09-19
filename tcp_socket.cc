#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include "tcp_socket.h"
#include "debug.h"

using namespace std;

const bool SOCK_DB = false;

TCPsocket::TCPsocket(int port_num_, int buff_size_, int queue_len_) :
  buff_size(buff_size_), queue_len(queue_len_), is(*this), os(*this) {
  sock_num = socket(PF_INET, SOCK_STREAM, 0); // IPv4 TCP socket
  if (sock_num < 0) {
    close(sock_num);
    throw ServerException("socket");
  }

  option = 1;
  if (setsockopt(sock_num, SOL_SOCKET, SO_REUSEADDR, &option,
                 sizeof(option)) < 0) {
    throw ServerException("setsockopt");
  };

  server_addr.sin_family = AF_INET; // IPv4
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port_num_);

  // bind the socket to a concrete address
  if (bind(sock_num, (struct sockaddr *) &server_addr,
           sizeof(server_addr)) < 0) {
    throw ServerException("bind");
  }

  // buffor
  init_buff();
}

TCPsocket::TCPsocket(int connection_fd_, int buff_size_) :
  buff_size(buff_size_), is(*this), os(*this) {

  msg_sock_dsc = connection_fd_;

  // buffor
  init_buff();
}

void TCPsocket::init_buff() {
  buff = shared_ptr<char[]>(new char[buff_size + 1]);
  buff.get()[buff_size] = 0;
}

TCPsocket::~TCPsocket() {
  release();
}

void TCPsocket::listen_for_connections() {
  if (listen(sock_num, queue_len) < 0) {
    throw ServerException("listen");
  }
}

void TCPsocket::accept_connection(int flags) {
  socklen_t client_addr_len = sizeof(client_addr);

  msg_sock_dsc = accept4(sock_num, (struct sockaddr *) &client_addr,
                        &client_addr_len, flags);
  if (msg_sock_dsc < 0) {
    throw ServerException("accept connection");
  }
}

void TCPsocket::accept_connection() {
  accept_connection(0);
}

void TCPsocket::accept_nonblocking_connection() {
  accept_connection(SOCK_NONBLOCK);
}

void TCPsocket::close_connection() {
  if (close(msg_sock_dsc) < 0) {
    std::cerr << "close socket";
  }
}

void TCPsocket::release() {
  // Disables further send and receive operations
  if (shutdown(sock_num, SHUT_RDWR) < 0) {
    PRINTLNF("release: Error in shutdowning the socket " << sock_num <<
              std::endl);
  }
}

bool SIn::load_msg_to_ss() {

  int len = read(sock.msg_sock_dsc, sock.buff.get(), sock.buff_size);
  if (len != 0) {
    if (SOCK_DB) std::clog << "msg loaded" << std::endl;
    sock.buff[len] = 0;
    ss.str("");
    ss.clear();
    ss << sock.buff;
  } else {
    if (SOCK_DB) std::clog << "no further msg" << std::endl;
  }
  return len > 0;
}

SIn &SIn::getline(std::string &buff_str) {
  if (ss.eof()) {
    buff_str.clear();
  } else {
    std::getline(ss, buff_str);
    ss.get(); /* make ss EOF */
  }
  return *this;
}

bool SIn::eof() {
  bool is_eof = false;
  if (ss.eof()) {
    if (SOCK_DB) std::clog << "ss is empty" << std::endl;
    if (!load_msg_to_ss()) {
      is_eof = true;
    }
  }
  return is_eof;
}

SOut &SOut::operator<<(SOut &(*f)(SOut &)) {
  f(*this);
  return *this;
}

void SOut::send() {
  const std::string &str = ss.str();
  int snd_len = write(sock.msg_sock_dsc, str.c_str(), str.length());
  if (snd_len != (int) str.length()) {
    throw ServerException("Writing to client socket");
  }
  ss.str("");
  ss.clear();
}

SOut &SOut::flush(SOut &out_s) {
  out_s.send();
  return out_s;
}

SOut &SOut::endl(SOut &out_s) {
  return out_s << "\n" << SOut::flush;
}