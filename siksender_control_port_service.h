#ifndef SIKSENDER_CONTROL_PORT_SERVICE_H
#define SIKSENDER_CONTROL_PORT_SERVICE_H

#include "siksender_shared_resource.h"
#include "udp_msg_receiver.h"

class ControlPortService : public ServerThread {
  const static bool DB = false;
  UDPsocket ctrl_sock;
public:
  ControlPortService(shared_ptr<SharedResource> &resource_) :
    ServerThread("control_receiver", resource_) {

    add_release_function([this] {
      this->release();
    });

    ctrl_sock.set_reusing_address();
    ctrl_sock.set_reusing_port();
    ctrl_sock.set_local_port(ntohs(resource->ctrl_addr.sin_port));
    ctrl_sock.add_broadcast();
  }

  void release() {
    ctrl_sock.closeSock();
  }

  void run() override {
    MsgReceiver recvr{512};
    recvr.set_sock(&ctrl_sock);

    Msg reply{replay_datagram(*resource).to_msg()};

    DEBC(DB, {
      sockaddr_in ss = ctrl_sock.get_local_addr();
      PRINTLNFC(DB, "port: " << ntohs(ss.sin_port) << ", " <<
                ((int) ss.sin_port))
    })

    while (!is_terminating()) {
      try {
        PRINTLNFC(DB, "Odbiorę msg");
        MsgFrom msg = recvr.recv_msg_from();
        PRINTLNFC(DB, "Odebrałem msg from " << ntohs(msg.src_addr.sin_port) << "  "
                                       << sin_addr_to_string(
                                         msg.src_addr.sin_addr));
        ControlDatagram dt{msg};
        ControlDatagramEnum genre = ctrl_dt_genre(dt);

        PRINTLNFC(DB, (int) genre);
        switch (genre) {
          case ControlDatagramEnum::LOOKUP: {
            PRINTLNFC(DB, "Mam lookup");
            bool not_send = true;
            while (!is_terminating() && not_send) {
              try {
                ctrl_sock.send_msg_to(reply, msg.src_addr);
                not_send = false;
              }
              catch (exception const &e) {
                PRINTLNFC(DB, "ERROR " << e.what())
                sleep_for(10ms);
              }
            }
          }
            break;
          case ControlDatagramEnum::REXMIT: PRINTLNFC(DB, "Mam rexmit");
            handle_rexmit(dt);
            break;
          default:
            break;
        }
      } catch (exception const &e) {
        PRINTLNFC(DB, "ERROR " << e.what());
      }
    }
  }

private:
  void handle_rexmit(ControlDatagram const &dt) {
    stringstream ss;
    ss.str(*(++dt.fields.begin()));
    PRINTLNC(DB, "pola: " << *(++dt.fields.begin()))
    uint64_t first_byte_num;
    list<uint64_t> list;
    PRINTLNC(DB, "odebrane z rexmitu:")

    bool is_div = true;
    char one_char;
    while (ss >> first_byte_num) {
      ss >> one_char;
      if (first_byte_num <= resource->last_byte_num
          && first_byte_num >= resource->earliest_byte_num) {

        if (first_byte_num % resource->psize != 0) {
          is_div = false;
          PRINTLNC(DB, "złe numery bytów")
          break;
        }

        PRINTC(DB, first_byte_num << " ")
        PRINTC(DB, "\n");
        list.push_back(first_byte_num);
      }
    }
    FLUSH()

    if (is_div) {
      lock_guard<mutex> rexmit_lk(resource->rexmit_mutex);
      for (auto it = list.begin(); it != list.end(); it++)
        resource->nums_to_rexmit.insert(*it);
    }
  }
};

#endif // SIKSENDER_CONTROL_PORT_SERVICE_H
