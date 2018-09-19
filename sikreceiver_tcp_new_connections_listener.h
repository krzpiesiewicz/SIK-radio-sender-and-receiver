#ifndef SIKRECEIVER_TCP_NEW_CONNECTIONS_LISTENER_H
#define SIKRECEIVER_TCP_NEW_CONNECTIONS_LISTENER_H

#include "sikreceiver_tcp_manager.h"

class TCPnewConnectionsListener : public SmartThread {
  TCPmanager *tcp_manager;
  TCPsocket sock;
  shared_ptr<string> to_write;
public:
  TCPnewConnectionsListener(TCPmanager *tcp_manager_, int port_num);

  void run() override;
};

#endif // SIKRECEIVER_TCP_NEW_CONNECTIONS_LISTENER_H
