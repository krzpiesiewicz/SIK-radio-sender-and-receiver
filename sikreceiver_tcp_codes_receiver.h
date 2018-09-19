#ifndef SIKRECEIVER_TCP_CODES_RECEIVER_H
#define SIKRECEIVER_TCP_CODES_RECEIVER_H

#include "sikreceiver_tcp_client_handler.h"

class TCPcodesReceiver : public SmartThread {
  TCPclientHandler *tcp_client_handler;
  TCPsocket sock;
  shared_ptr<string> to_write;
public:
  TCPcodesReceiver(TCPclientHandler *tcp_client_handler_,
                   int connection_fd, int buffsize) :
    SmartThread("tcp_codes_receiver"),
    tcp_client_handler(tcp_client_handler_),
    sock(connection_fd, buffsize) {}

  void run() override {
    string code;

    while (!is_terminating() && !sock.in().eof()) {
      try {
        sock.in().getline(code);
        PRINTLNFC(TCPmanager::DB, "ODBIERAM po TCP");
        tcp_client_handler->pass_code(code);
      }
      catch (exception const &e) {
        PRINTLNFC(TCPmanager::DB, "ERROR " << e.what())
      }
    }
    tcp_client_handler->terminate();
  }
};

#endif // SIKRECEIVER_TCP_CODES_RECEIVER_H
