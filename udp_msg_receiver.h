#ifndef UDP_MSG_RECEIVER_H
#define UDP_MSG_RECEIVER_H

#include <functional>

class MsgReceiver {
  const static bool DB = false;
  std::size_t buff_size;
  std::shared_ptr<byte[]> buff;
  UDPsocket *sock;

public:
  MsgReceiver(std::size_t buff_size_);

  void set_sock(UDPsocket *sock_) {sock = sock_;}

  Msg recv_msg();

  MsgFrom recv_msg_from();

  void recv_msg(Msg & msg);

  void recv_msg_from(MsgFrom & msg);

  /**
   * Reads msg data and calls passed function @fun with arguments:
   * ptr: byte* and size: int and which are a pointer to the data
   * and the size of the data.
   * @param fun
   */
  void recv_msg_and_call_fun(std::function<void(const byte*, size_t)> fun);
private:
  inline std::byte *get_buff() {
    return buff.get();
  }
};

#endif // UDP_MSG_RECEIVER_H
