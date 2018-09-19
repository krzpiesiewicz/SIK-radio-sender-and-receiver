#ifndef SIKRECEIVER_AUDIO_RECEIVER_H
#define SIKRECEIVER_AUDIO_RECEIVER_H

#include <memory>

#include "sikreceiver_shared_resource.h"
#include "smart_thread.h"
#include "host_and_net.h"
#include "udp_msg_receiver.h"

using namespace std;

class AudioReceiver : public ServerThread {
  const static bool DB = true;
  shared_ptr<UDPsocket> audio_sock;
  MsgReceiver audio_recvr;
  bool set_addr = false;
public:
  AudioReceiver(shared_ptr<SharedResource> &resource_) :
    ServerThread("audio_receiver", resource_),
    audio_recvr{AudioDatagram::audio_datagram_all_size()} {

    add_release_function([this] {
      release();
    });
  }

  void update_sock() {
    lock_guard<mutex> lk(cv_m);
    if (!set_addr
        || resource->audio_addr.sin_addr != audio_sock->ipmreq.imr_multiaddr
        || resource->audio_addr.sin_port != audio_sock->local_addr.sin_port) {


      if (set_addr) {
        audio_sock->closeSock();
      } else {
        set_addr = true;

      }

      audio_sock = make_shared<UDPsocket>();
      audio_sock->set_reusing_address();
      audio_sock->add_multicast_receiving(resource->audio_addr);
      audio_recvr.set_sock(audio_sock.get());
    }
  }

  void release() {
    if (audio_sock != nullptr)
      audio_sock->closeSock();
  }

  void release_sock() {
    lock_guard<mutex> lk(cv_m);
    if (audio_sock != nullptr) {
      audio_sock->closeSock();
    }
    set_addr = false;
  }

  void run() override {
    while (!is_terminating()) {

      reach_barrier();

      wait_until([this] {return resource->barrier_should_be_reached
                                || resource->audio_receiving;
      });
      unlock();
      if (resource->barrier_should_be_reached)
        continue;

      {
        lock_guard<mutex> lk(resource->channels_mutex);
        update_sock();
      }

      while (!is_terminating() && !resource->barrier_should_be_reached
             && resource->audio_receiving && !resource->first_byte_num_was_set) {
        PRINTLNFC(DB, "Spróbuję dostać pierwszą wiadomość")
        try {
          audio_recvr.recv_msg_and_call_fun
            ([this](const byte *buff, size_t size) {
              check_first_audio_msg_and_save(buff, size);
            });

          resource->rexmit_requester->notify();
        } catch (exception &e) {
          {
            lock_guard<mutex> lk(resource->channels_mutex);
            resource->audio_playing_error = true;
            resource->barrier_should_be_reached = true;
            resource->main_thread->notify();
          }
          break;
        }
      }

      while (!is_terminating() && !resource->barrier_should_be_reached
             && resource->audio_receiving) {

        try {
          audio_recvr.recv_msg_and_call_fun
            ([this](const byte *buff, size_t size) {
              check_audio_msg_and_save(buff, size);
            });

          if (!resource->audio_playing &&
              4 * (resource->last_byte_num + resource->psize -
                   resource->first_byte_num - 1) >=
              3 * resource->bsize) {
              resource->audio_playing = true;
              PRINTLNFC(DB, "trzeba zacząć grać")
            resource->audio_player->notify();
          }
        } catch (const exception &e) {
          PRINTLNFC(DB, "ale wiadomość jest zła!" << e.what());
        }
      }
    }
  }

private:
  inline void check_first_audio_msg_and_save(const byte *buff, size_t size) {
    if (is_terminating())
      return;
    if (size == AudioDatagram::audio_datagram_all_size()) {

      uint64_t session_id, first_byte_num;
      buff = AudioDatagram::read_header(session_id, first_byte_num, buff, size);

      PRINTLNFC(DB, "dostałem pierwszą wiadomość");

      save_datagram(session_id, first_byte_num, buff);

      {
        lock_guard<mutex> lock(resource->channels_mutex);
        resource->session_id = session_id;
        resource->first_byte_num = first_byte_num;
        resource->last_byte_num = first_byte_num;
        resource->curr_byte_num = first_byte_num;

        resource->first_byte_num_was_set = true;
      }
    }
  }

  inline void check_audio_msg_and_save(const byte *buff, size_t size) {
    if (is_terminating())
      return;
    if (size == AudioDatagram::audio_datagram_all_size()) {

      uint64_t session_id, first_byte_num;
      buff = AudioDatagram::read_header(session_id, first_byte_num, buff, size);

      if (session_id == resource->session_id
          && first_byte_num > resource->curr_byte_num) {

        if (first_byte_num >= resource->last_byte_num.load()
            || abs((long long) (resource->last_byte_num.load() -
                                first_byte_num)) <=
               (long long) resource->bsize) {

          resource->last_byte_num = max(resource->last_byte_num.load(),
                                        first_byte_num);
          save_datagram(session_id, first_byte_num, buff);
        }
      } else if (session_id > resource->session_id) {
        {
          lock_guard<mutex> lk(resource->channels_mutex);
          resource->audio_playing_error = true;
          resource->barrier_should_be_reached = true;
          resource->main_thread->notify();
        }
      }
    }
  }

  inline void save_datagram(uint64_t session_id, uint64_t first_byte_num,
                            const byte *buff) {
    auto elem = resource->fifo.get_to_set(first_byte_num / resource->psize);
    if (elem != nullptr) {
      AudioDatagram &dt = elem->t;
      dt.session_id(session_id);
      dt.first_byte_num(first_byte_num);
      AudioDatagram::read_audio_data(buff, dt.audio_data(),
                                     AudioDatagram::AUDIO_DATA_SIZE);
    } else {
    }
  }
};


#endif // SIKRECEIVER_AUDIO_RECEIVER_H
