#ifndef SIKRECEIVER_LOOKUP_SENDER_H
#define SIKRECEIVER_LOOKUP_SENDER_H

#include "sikreceiver_shared_resource.h"
#include "smart_thread.h"
#include "debug.h"

class LookupSender : public ServerThread {
  UDPsocket ctrl_sock;
public:
  LookupSender(shared_ptr<SharedResource> &resource_) : ServerThread("lookup",
                                                           resource_) {

    ctrl_sock.set_reusing_address();
    ctrl_sock.set_local_port(ntohs(resource->local_addr.sin_port));
    ctrl_sock.add_multicast_sending(resource->discover_addr);

    add_release_function([this] {
      this->release();
    });
  }

  void release() {
    ctrl_sock.closeSock();
  }

  void run() override {
    Msg msg{lookup_datagram().to_msg()};

    while (!is_terminating()) {
      bool not_send = true;
      try {
        ctrl_sock.send_msg(msg);
        not_send = false;
      } catch(exception const &e) {
        PRINTLN("nie udało się wysłać wiadomości")
        sleep_for(20ms);
      }
      if (!not_send)
        sleep_for(4950ms);
    }
  }
};

#endif // SIKRECEIVER_LOOKUP_SENDER_H
