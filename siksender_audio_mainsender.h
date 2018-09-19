#ifndef SIKSENDER_AUDIO_MAINSENDER_H
#define SIKSENDER_AUDIO_MAINSENDER_H

#include <string>

#include "siksender_shared_resource.h"
#include "host_and_net.h"
#include "udp_msg_sender.h"
#include "fifo.h"

class MainAudioSender : public ServerThread {
  shared_ptr<UDPsocket> audio_sock;
  shared_ptr<MsgSender> audio_sender;
public:
  MainAudioSender(shared_ptr<SharedResource> & resource_) :
    ServerThread("audio_mainsender", resource_) {

    add_release_function([this] {
      this->release();
    });

    audio_sock = make_shared<UDPsocket>();
    audio_sock->set_reusing_address();
    audio_sock->bind_to_any_port();
    audio_sock->add_multicast_sending(resource->mcast_addr);

    audio_sender = SmartThread::create<MsgSender>(audio_sock);
  }

  void release() {
    if (audio_sock != nullptr)
      audio_sock->closeSock();
    SmartThread::terminate(audio_sender);
  }

  void run() override {

    bool end_of_file = false;
    uint64_t first_byte_num = 0;
    int64_t dt_num = 0;

    audio_sender->start();

    ios_base::sync_with_stdio(false);

    while (!end_of_file) {

      resource->fifo.lock();
      auto elem = resource->fifo.get_to_set(dt_num);
      resource->fifo.unlock();
      if (elem == nullptr)
        throw ServerException("Cannot set next datagram");
      Msg &msg = elem->t;
      AudioDatagram dt{msg};
      dt.first_byte_num(first_byte_num);

      for (int i = 0; i < (int) AudioDatagram::AUDIO_DATA_SIZE; i++)
        if (cin.get((char &) dt.audio_data()[i]).eof()) {
          PRINTLNF("eof");
          end_of_file = true;
          break;
        }
      if (!end_of_file) {
        audio_sender->add_msg(msg);

        resource->last_byte_num = first_byte_num;
        if (first_byte_num > resource->fsize)
          resource->earliest_byte_num = first_byte_num - resource->fsize;

        first_byte_num += resource->psize;
        dt_num++;
      }
    }

    audio_sender->end_when_all_msgs_sent();
    SmartThread::join(audio_sender);
  }
};

#endif // SIKSENDER_AUDIO_MAINSENDER_H
