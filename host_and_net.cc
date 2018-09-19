#include <memory>
#include <netinet/in.h>
#include <iostream>
#include <cstddef>

#include "err.h"
#include "host_and_net.h"

using namespace std;

bool operator==(in_addr const & a, in_addr const & b) {
  return a.s_addr == b.s_addr;
}

bool operator!=(in_addr const & a, in_addr const & b) {
  return !(a == b);
}

bool operator==(sockaddr_in const & a, sockaddr_in const & b) {
  return a.sin_addr == b.sin_addr && a.sin_port == b.sin_port
    && a.sin_family == b.sin_family;
}

bool operator!=(sockaddr_in const & a, sockaddr_in const & b) {
  return !(a == b);
}

uint16_t cast_port(int port_num) {
  uint16_t port;
  try {
    port = static_cast<uint16_t>(port_num);
  } catch (...) {
    throw ServerException("problem with casting int to uint16_t");
  }
  return port;
}

uint64_t htonll(uint64_t value) {
  int num = 42;
  // check if it demands byte swapping
  if (*(uint8_t *) &num == 42) {
    uint32_t high_part = htonl((uint32_t) (value >> 32));
    uint32_t low_part = htonl((uint32_t) (value & 0xFFFFFFFFLL));
    return (((uint64_t) low_part) << 32) | high_part;
  } else {
    return value;
  }
}

uint64_t ntohll(uint64_t value) {
  int num = 42;
  // check if it demands byte swapping
  if (*(uint8_t *) &num == 42) {
    uint32_t high_part = ntohl((uint32_t) (value >> 32));
    uint32_t low_part = ntohl((uint32_t) (value & 0xFFFFFFFFLL));
    return (((uint64_t) low_part) << 32) | high_part;
  } else {
    return value;
  }
}

void write_ll(byte **buff, uint64_t value) {
  uint64_t *uint_ptr = (uint64_t *) *buff;
  *uint_ptr = htonll(value);
  *buff += sizeof(uint64_t);
}

uint64_t read_ll(const byte **buff) {
  uint64_t *uint_ptr = (uint64_t *) *buff;
  *buff += sizeof(uint64_t);
  return ntohll(*uint_ptr);
}

void write_bytes(byte **buff, const byte bytes[], unsigned num_of_bytes) {
  for (unsigned i = 0; i < num_of_bytes; i++)
    (*buff)[i] = bytes[i];
  *buff += num_of_bytes * sizeof(byte);
}

void read_bytes(const byte **buff, byte target[], unsigned num_of_bytes) {
  for (unsigned i = 0; i < num_of_bytes; i++)
    target[i] = (*buff)[i];
  *buff += num_of_bytes * sizeof(byte);
}

istream &operator>>(istream &op, byte &b) {
  unsigned char c;
  op >> c;
  b = (byte) c;
  return op;
}

ostream &operator<<(ostream &op, byte b) {
  auto c = (unsigned char) b;
  op << c;
  return op;
}

void print_tab(std::byte tab[], std::size_t size) {
  for (int i = 0; i * sizeof(std::byte) < size; i++)
    cout << tab[i];
  cout << "\n";
}

void print_shptr(std::shared_ptr<std::byte[]> sh, std::size_t size) {
  print_tab(sh.get(), size);
}