#ifndef MESSAGE_SENDER_H
#define MESSAGE_SENDER_H

#include "udp_sock_and_msg.h"
#include "smart_thread.h"
#include "err.h"
#include "debug.h"

class MsgSender : public SmartThread {
  const static bool DB = false;
  std::shared_ptr<UDPsocket> sock;
  std::queue<Msg> queue;
  std::atomic<int> qsize = 0;
  std::atomic<bool> send_all_and_end = false;
public:
  MsgSender(std::shared_ptr<UDPsocket> sock_) : SmartThread(get_next_name()) {
    set_sock(sock_);
    add_release_function([this]{this->release();});
  }

  void set_sock(std::shared_ptr<UDPsocket> sock_) {sock = sock_;}

  void release() {
    sock->closeSock();
  }

  void run() {
    while (!is_terminating() && !(send_all_and_end && q_size() == 0)) {
      wait_until_notifying([this] {
        return this->q_size() > 0 || this->send_all_and_end;
      });
      unlock();
      while (!is_terminating() && q_size() > 0)
        send_msg();
    }
  }

  void end_when_all_msgs_sent() {
    send_all_and_end = true;
    notify();
  }

  void add_msg(const Msg & msg) {
    std::lock_guard<std::mutex> lk(cv_m);
    queue.push(msg);
    qsize++;
    notify_without_lock_quard();
  }

  int q_size() {return qsize;}

private:
  void send_msg() {
    send_msg(10);
  }

  void send_msg(int attempts) {
    lock();
    if (queue.empty())
      throw ServerException("Empty queue");
    auto& msg= queue.front();
    unlock();

    bool not_send = true;
    while (!is_terminating() && not_send && attempts > 0)
      try {
        sock->send_msg(msg);
        not_send = false;
      } catch (exception const &e) {
        PRINTLNF("ERROR in sender:" << e.what())
        sleep_for(10ms);
        attempts--;
      }

    lock();
    queue.pop();
    qsize--;
    unlock();
  }

  static std::string get_next_name() {
    static std::atomic<int> id = 0;
    id++;
    return "msg_sender of id=" + to_string(id) + " created by " + GET_THREAD_NAME();
  }
};

#endif //  MESSAGE_SENDER_H
