#ifndef SIKSENDER_SHARED_RESOURCE_H
#define SIKSENDER_SHARED_RESOURCE_H

#include "smart_thread.h"
#include "tcp_socket.h"
#include "radio_channel.h"
#include "menu.h"
#include "host_and_net.h"
#include "fifo.h"
#include "datagrams.h"

using namespace std;

struct SharedResource : BasicSender {
  sockaddr_in mcast_addr, ctrl_addr;

  FIFO<Msg> fifo;
  size_t psize, fsize;

  set<uint64_t> nums_to_rexmit;
  mutex rexmit_mutex;

  atomic<uint64_t> first_byte_num, last_byte_num, earliest_byte_num;

  uint64_t session_id;
  int rtime;

  SharedResource(string const &name_, string const &mcast_addr_, int data_port_,
                 int ctrl_port_, size_t psize_, size_t fsize_, int rtime_) :
    BasicSender(name_, mcast_addr_, data_port_),
    mcast_addr(create_sockaddr_in(mcast_addr_, data_port_)),
    fifo(fsize_ / psize_),
    psize(psize_), fsize(fsize_), rtime(rtime_) {

    ctrl_addr.sin_port = ntohs(cast_port(ctrl_port_));

    session_id = chrono::system_clock::now().time_since_epoch().count();

    AudioDatagram::AUDIO_DATA_SIZE = psize_;

    for (int i = 0; i < fifo.fifo_size; i++) {
      Msg &msg = fifo.buff[i].t;
      AudioDatagram dt;
      dt.session_id(session_id);
      dt.first_byte_num(0);
      msg = dt.to_msg();
    }
  }
};

class ServerThread : public SmartThread {
protected:
  shared_ptr<SharedResource> resource;
public:
  ServerThread(std::string name_, shared_ptr<SharedResource> & resource_) :
    SmartThread(name_), resource(resource_) {}

  virtual ~ServerThread() {};

  virtual void run() override = 0;
};

#endif // SIKSENDER_SHARED_RESOURCE_H
