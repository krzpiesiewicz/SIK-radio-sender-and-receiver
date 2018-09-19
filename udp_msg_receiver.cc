#include <unistd.h>

#include "err.h"
#include "host_and_net.h"
#include "udp_sock_and_msg.h"
#include "udp_msg_receiver.h"


MsgReceiver::MsgReceiver(size_t buff_size_) : buff_size(buff_size_),
  buff(new std::byte[buff_size + 1], std::default_delete<byte[]>()) {}

inline std::shared_ptr<byte[]> get_shorter_buff(byte *buff, int len) {
  std::shared_ptr<byte[]> data{new std::byte[len],
                               std::default_delete<byte[]>()};

  for (int i = 0; i < len; i++)
    data[i] = buff[i];

  return data;
}

Msg MsgReceiver::recv_msg() {
  ssize_t rcv_len = read(sock->num(), get_buff(), buff_size);
  if (rcv_len < 0)
    throw ServerException("read");

  int len = rcv_len;
  get_buff()[len] = (byte) 0;

  return Msg{get_shorter_buff(get_buff(), len), (size_t) len};
}

void MsgReceiver::recv_msg_and_call_fun(
  std::function<void(const byte*, size_t)> fun) {
  ssize_t rcv_len = read(sock->num(), get_buff(), buff_size);
  if (rcv_len < 0)
    throw ServerException("read");

  size_t len = rcv_len;

  fun(get_buff(), len);
}

MsgFrom MsgReceiver::recv_msg_from() {
  struct sockaddr_in addr;
  socklen_t socklen = sizeof(sockaddr_in);
  ssize_t rcv_len = recvfrom(sock->num(), get_buff(), buff_size, 0,
                             (sockaddr *) &addr, &socklen);
  if (rcv_len < 0)
    throw ServerException("read");

  int len = (int) rcv_len;

  get_buff()[len] = (byte) 0;

  return MsgFrom{get_shorter_buff(get_buff(), len), (size_t) len, addr};
}