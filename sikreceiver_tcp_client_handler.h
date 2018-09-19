#ifndef SIKRECEIVER_TCP_CLIENT_HANDLER_H
#define SIKRECEIVER_TCP_CLIENT_HANDLER_H

#include "sikreceiver_tcp_manager.h"

class TCPclientHandler : public SmartThread {
  TCPmanager *tcp_manager;
  TCPsocket sock;
  shared_ptr<string> to_write;
  shared_ptr<SmartThread> codes_receiver;
public:
  TCPclientHandler(TCPmanager *tcp_manager_, int connection_fd);

  void run() override;

  void pass_code(string code);

  friend class TCPmanager;
private:
  void configure_telnet_client();

  void write_ui();
};

#endif // SIKRECEIVER_TCP_CLIENT_HANDLER_H
