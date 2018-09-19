#ifndef HOST_AND_NET_H
#define HOST_AND_NET_H

#include <memory>
#include <netinet/in.h>
#include <iostream>
#include <cstddef>

bool operator==(in_addr const & a, in_addr const & b);
bool operator!=(in_addr const & a, in_addr const & b);
bool operator==(sockaddr_in const & a, sockaddr_in const & b);
bool operator!=(sockaddr_in const & a, sockaddr_in const & b);

uint16_t cast_port(int port_num);

uint64_t htonll(uint64_t value);

uint64_t ntohll(uint64_t value);

void write_ll(std::byte **buff, uint64_t value);

uint64_t read_ll(const std::byte **buff);

void write_bytes(std::byte **buff, std::byte *bytes, unsigned num_of_bytes);

void read_bytes(const std::byte **buff, std::byte target[],
                unsigned num_of_bytes);

std::istream &operator>>(std::istream &op, const std::byte &b);

std::ostream &operator<<(std::ostream &op, const std::byte b);

void print_tab(std::byte tab[], std::size_t size);

void print_shptr(std::shared_ptr<std::byte[]> sh, std::size_t size);

#endif // HOST_AND_NET_H
