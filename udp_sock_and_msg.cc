#include <memory>
#include <queue>
#include <chrono>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mutex>

#include "err.h"
#include "host_and_net.h"

#include "udp_sock_and_msg.h"
#include "debug.h"

UDPsocket::UDPsocket() {
  sock_num = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock_num < 0)
    throw ServerException("socket");
  local_addr.sin_family = AF_INET;
  local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  local_addr.sin_port = 0;
}

UDPsocket::~UDPsocket() {
  if (!closing)
    closeSock();
}

sockaddr_in create_sockaddr_in(std::string address, int port_num) {
  sockaddr_in addr;

  if (inet_aton(address.c_str(), &addr.sin_addr) == 0)
    throw ServerException("inet_aton");

  addr.sin_port = htons(cast_port(port_num));
  addr.sin_family = AF_INET;

  return addr;
}

string sin_addr_to_string(in_addr sa) {
  char tmp[20];
  inet_ntop(AF_INET, &sa, tmp, INET_ADDRSTRLEN);
  return string(tmp);
}

sockaddr_in UDPsocket::get_local_addr() {
  sockaddr_in addr;
  if (getsockname(num(), (sockaddr*) &addr, (socklen_t*) sizeof addr) < 0)
    PRINTLNFC(DB, "Cannot getsockname from sock num " << num() << "\n")
  return addr;
}

void UDPsocket::set_local_port(int port_num) {
  // binding to local address and the certain port
  in_port_t port = cast_port(port_num);

  local_addr.sin_family = AF_INET;
  local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  local_addr.sin_port = htons(port);
  if (bind(sock_num, (struct sockaddr *) &local_addr, sizeof(local_addr)) < 0)
    throw ServerException("bind");
}

void UDPsocket::bind_to_any_port() {
  // binding to local address and any port
  set_local_port(0);
}

void UDPsocket::set_reusing_address() {
  int optval = 1;
  if (setsockopt(sock_num, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) < 0)
    throw ServerException("setsockopt so_reuseaddr");
}

void UDPsocket::set_reusing_port() {
  int optval = 1;
  if (setsockopt(sock_num, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval) < 0)
    throw ServerException("setsockopt so_reuseport");
}

void UDPsocket::connect_to(sockaddr_in remote_addr_) {
  remote_addr = remote_addr_;
  if (connect(sock_num, (sockaddr*) &remote_addr, sizeof remote_addr) < 0)
    throw ServerException("connect");
}

void UDPsocket::disconnect() {
  remote_addr.sin_family = AF_UNSPEC;
  if (connect(sock_num, (sockaddr*) &remote_addr, sizeof remote_addr) < 0)
    throw ServerException("disconnect");
}

const int TTL_VALUE = 4;

void UDPsocket::add_multicast_sending(sockaddr_in remote_addr_) {

  remote_addr = remote_addr_;

  // activating broadcasting
  add_broadcast();

  // setting TTL for group datagrams
  int optval = TTL_VALUE;
  if (setsockopt(sock_num, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &optval,
                 sizeof optval) < 0)
    throw ServerException("setsockopt multicast ttl");

  if (connect(sock_num, (sockaddr *) &remote_addr, sizeof remote_addr) < 0)
    throw ServerException("connect");
}

void UDPsocket::add_multicast_receiving(sockaddr_in remote_addr) {
  // connect to multicast group
  ipmreq.imr_interface.s_addr = htonl(INADDR_ANY);
  ipmreq.imr_multiaddr = remote_addr.sin_addr;
  if (setsockopt(sock_num, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&ipmreq,
                 sizeof ipmreq) < 0)
    throw ServerException("setsockopt ip add membership");

  local_addr.sin_port = remote_addr.sin_port;
  if (bind(sock_num, (struct sockaddr *) &local_addr, sizeof local_addr) < 0)
    throw ServerException("bind");
}

void UDPsocket::add_broadcast() {
  int optval = 1;
  if (setsockopt(sock_num, SOL_SOCKET, SO_BROADCAST, (void *) &optval,
                 sizeof optval) < 0)
    throw ServerException("setsockopt broadcast");
}

void UDPsocket::disable_multicast_receiving() {
  setsockopt(sock_num, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void *) &ipmreq,
             sizeof ipmreq);
}

void UDPsocket::closeSock() {
  if (!closing) {
    closing = true;
    if (multicast_receiving)
      disable_multicast_receiving();
    if (shutdown(sock_num, 2) < 0)
      PRINTLNFC(DB, "Problem with shutdowning the socket " << sock_num << ".\n");
    if (close(sock_num) < 0)
      PRINTLNFC(DB, "Problem with closing the socket " << sock_num << ".\n");
  }
}

void UDPsocket::send_msg(const Msg &msg) {
  if (write(num(), msg.get_data(), msg.data_len) != (ssize_t) msg.data_len)
    throw ServerException("write\n");
}

void UDPsocket::send_msg_to(const Msg &msg, const sockaddr_in &dst_addr) {
  if (sendto(num(), msg.get_data(), msg.data_len, 0,
             (sockaddr *) &dst_addr, (socklen_t) sizeof(dst_addr))
    != (ssize_t) msg.data_len) {
    throw ServerException("sendto\n");
  }
}