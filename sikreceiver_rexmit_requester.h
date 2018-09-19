#ifndef SIKRECEIVER_REXMIT_REQUESTER_H
#define SIKRECEIVER_REXMIT_REQUESTER_H

#include "sikreceiver_shared_resource.h"
#include "smart_thread.h"
#include "host_and_net.h"
#include "udp_msg_sender.h"
#include "debug.h"

class RexmitRequester : public ServerThread {
  const static bool DB = true, DB_LACKING = true;
  shared_ptr<UDPsocket> ctrl_sock;
  shared_ptr<MsgSender> sender;
  shared_ptr<ConditionVariableTimeAlarm> alarm;
public:
  RexmitRequester(shared_ptr<SharedResource> &resource_) :
    ServerThread("rexmit_requester", resource_) {

    alarm = SmartThread::create<ConditionVariableTimeAlarm>
      (ref(cv), ref(cv_m), chrono::milliseconds(resource->rtime),
       [this] {this->notify();});

    add_release_function([this] {
      this->release();
    });
  }

  void update_sock() {
    if (ctrl_sock != nullptr) {
      ctrl_sock->closeSock();
    }
    ctrl_sock = make_shared<UDPsocket>();
    ctrl_sock->set_reusing_address();
    ctrl_sock->bind_to_any_port();
    ctrl_sock->connect_to(resource->ctrl_addr);
  }

  void release_sock() {
    if (ctrl_sock != nullptr) {
      ctrl_sock->closeSock();
    }
    SmartThread::terminate(sender);
  }

  void release() {
    if (ctrl_sock != nullptr) {
      ctrl_sock->closeSock();
    }
    SmartThread::terminate(sender);
    SmartThread::terminate(alarm);
  }

  void run() override {

    alarm->start();

    auto &fifo = resource->fifo;
    auto &buff = fifo.buff;

    vector<uint64_t> now_lacking;
    now_lacking.resize(fifo.fifo_size + 7);
    int lacking_cnt, start_idx, last_idx;
    int64_t start_num;
    uint64_t start_byte;

    while (!is_terminating()) {

      reach_barrier();

      wait_until([this] {
        return resource->barrier_should_be_reached
               || resource->audio_receiving;
      });
      unlock();
      if (resource->barrier_should_be_reached)
        continue;

      {
        lock_guard<mutex> lk(resource->channels_mutex);
        update_sock();
        sender = SmartThread::create<MsgSender>(ctrl_sock);
        sender->set_sock(ctrl_sock);
        sender->start();
      }

      wait_until([&] {
        return resource->barrier_should_be_reached
               || (resource->audio_receiving && resource->first_byte_num_was_set);
      });
      unlock();
      if (resource->barrier_should_be_reached)
        continue;

      start_byte = 0;

      while (!is_terminating() && !resource->barrier_should_be_reached
             && resource->audio_receiving) {
        alarm->start_counting();

        start_byte = max(start_byte, resource->curr_byte_num.load());
        start_num = start_byte / resource->psize;
        fifo.lock();
        start_idx = fifo.idx_for_num(start_num);
        last_idx = fifo.idx_for_num(resource->last_byte_num / resource->psize);
        fifo.unlock();
        lacking_cnt = 0;

        while (start_idx != last_idx) {
          auto & elem = buff[start_idx];
          if (elem.num != start_num)
            now_lacking[lacking_cnt++] = start_byte;
          start_byte += resource->psize;
          start_num++;
          start_idx = fifo.idx_plus(start_idx);
        }
        DEBC(DB_LACKING,
             if (lacking_cnt > 0) {
               PRINTLNC(DB_LACKING, "Brakujące paczki (bytes nums): ")
               for (auto it = now_lacking.begin(); it != now_lacking.begin() + lacking_cnt; it++) {
                 PRINTC(DB_LACKING, *it << ", ")
               }
               FLUSHC(DB_LACKING)
            });
        if (lacking_cnt > 0) {
          auto last = now_lacking.begin() + lacking_cnt;
          sort(now_lacking.begin(), last);
          PRINTLNC(DB, "Wysyłam rexmit")
          sender->add_msg(rexmit_datagram(now_lacking.begin(), last).to_msg());
        }
        FLUSHC(DB)

        // for alarm:
        wait_until_notifying();
        unlock();
      }
      SmartThread::terminate(sender);
    }
  }
};

#endif // SIKRECEIVER_REXMIT_REQUESTER_H
