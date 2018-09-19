#ifndef SIKRECEIVER_REPLIES_RECEIVER_H
#define SIKRECEIVER_REPLIES_RECEIVER_H

#include "sikreceiver_shared_resource.h"
#include "smart_thread.h"
#include "time_util.h"
#include "udp_msg_receiver.h"

class RepliesReceiver : public ServerThread {
  bool DB = false;
  UDPsocket ctrl_sock;
public:
  RepliesReceiver(shared_ptr<SharedResource> &resource_) :
    ServerThread("replies_receiver", resource_) {

    ctrl_sock.set_reusing_address();
    ctrl_sock.set_local_port(ntohs(resource->local_addr.sin_port));

    add_release_function([this] {
    });
  }

  void release() {
    ctrl_sock.closeSock();
  }

  void run() override {
    MsgReceiver recvr{512};
    recvr.set_sock(&ctrl_sock);

    while (!is_terminating()) {
      try {
        MsgFrom msg = recvr.recv_msg_from();
        ControlDatagram dt = ControlDatagram(msg);
        PRINTLNFC(DB, "Odebrałem msg")
        if (ctrl_dt_genre(dt) == ControlDatagramEnum::REPLAY
            && dt.fields.size() >= 4) {
          auto it = dt.fields.begin();
          it++;
          string address = *it;
          it++;
          string port = *it;
          it++;
          string name = *it;
          it++;
          while (it != dt.fields.end()) {
            name += " " + *it;
            it++;
          }

          PRINTLNFC(DB, "Odebrałem dobry reply")

          int port_num = cast_port(std::stoi(port));
          sockaddr_in audio_addr = create_sockaddr_in(address, port_num);
          sockaddr_in ctrl_addr = msg.src_addr;
          ctrl_addr.sin_port = resource->ctrl_addr.sin_port;
          RadioChannel channel{name, address, port_num, audio_addr,
                               ctrl_addr, chrono::system_clock::now()};
          {
            unique_lock<mutex> channel_lk(resource->channels_mutex);
            auto it = resource->channels.find(channel);
            if (it != resource->channels.end()) {
              it->last_reply = channel.last_reply;
              PRINTLNFC(DB, "Poprawiam czas w starym kanale (" << it->name <<
                ") time " <<
                 display_time(channel.last_reply.time_since_epoch()))
              channel_lk.unlock();
            } else {
              PRINTLNFC(DB, "Dodaję nowy kanał (" << channel.name << ") time " <<
                display_time(channel.last_reply.time_since_epoch()))
              resource->channels.insert(channel);
              resource->is_new_channel = true;
              channel_lk.unlock();
              resource->main_thread->notify();
            }
          }
        }
      }
      catch (exception &e) {
        PRINTLNF("Error: " << e.what() << "\n");
      }
    }
  }
};

#endif // SIKRECEIVER_REPLIES_RECEIVER_H
