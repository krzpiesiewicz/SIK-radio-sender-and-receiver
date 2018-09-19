#ifndef SIKSENDER_REXMITER_H
#define SIKSENDER_REXMITER_H

#include <chrono>

#include "siksender_shared_resource.h"
#include "time_alarm.h"

class Rexmiter : public ServerThread {
  const static bool DB = true;
  shared_ptr<UDPsocket> audio_sock;
  shared_ptr<MsgSender> audio_sender;
  shared_ptr<ConditionVariableTimeAlarm> alarm;
public:
  Rexmiter(shared_ptr<SharedResource> & resource_) :
    ServerThread("rexmiter", resource_) {

    add_release_function([this] {
      this->release();
    });

    audio_sock = make_shared<UDPsocket>();
    audio_sock->set_reusing_address();
    audio_sock->bind_to_any_port();
    audio_sock->add_multicast_sending(resource->mcast_addr);

    audio_sender = SmartThread::create<MsgSender>(audio_sock);

    alarm = ConditionVariableTimeAlarm::create<ConditionVariableTimeAlarm>
      (ref(cv), ref(cv_m), chrono::milliseconds(resource->rtime),
       [this] {this->notify();});
  }

  void release() {
    if (audio_sock != nullptr)
      audio_sock->closeSock();

    SmartThread::terminate(alarm);
    SmartThread::terminate(audio_sender);
  }

  void run() override {
    alarm->start();
    audio_sender->start();

    while (!is_terminating()) {
      alarm->start_counting();
      wait_until_notifying();
      lk.unlock();

      list<uint64_t> bytes_nums;
      {
        lock_guard<mutex> rexmit_lk(resource->rexmit_mutex);
        for (auto it = resource->nums_to_rexmit.begin();
             it != resource->nums_to_rexmit.end(); it++)
          if (*it >= resource->earliest_byte_num)
            bytes_nums.push_back(*it);
        resource->nums_to_rexmit.clear();
      }

      for (auto it = bytes_nums.begin(); it != bytes_nums.end(); it++)
        if (*it >= resource->earliest_byte_num) {
          int64_t num = (*it) / resource->psize;
          resource->fifo.lock();
          auto elem = resource->fifo.get(num);
          resource->fifo.unlock();
          if (elem != nullptr && elem->num == num) {
            PRINTLNFC(DB, "Dodaję do wysłania paczkę rexmitu")
            audio_sender->add_msg(elem->t);
          }
        }
    }
  }
};

#endif // SIKSENDER_REXMITER_H
