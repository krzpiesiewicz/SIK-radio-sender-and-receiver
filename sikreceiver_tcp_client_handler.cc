#include "telnet.h"
#include "console_codes.h"
#include "sikreceiver_tcp_manager.h"
#include "sikreceiver_tcp_codes_receiver.h"
#include "sikreceiver_tcp_client_handler.h"

TCPclientHandler::TCPclientHandler(TCPmanager *tcp_manager_, int connection_fd) :
  SmartThread("tcp_client_handler"),
  tcp_manager(tcp_manager_), sock(connection_fd, TCPmanager::BUFF_SIZE) {

  add_release_function([this] {
    sock.close_connection();
    PRINTLNFC(TCPmanager::DB, "Connection closed." << endl);
    SmartThread::terminate(codes_receiver);
  });

  codes_receiver = SmartThread::create<TCPcodesReceiver>
    (this, connection_fd, TCPmanager::BUFF_SIZE);
  // while creating an object we should have critical section to
  // tcp_manager->cv_m:
  to_write = tcp_manager->to_write;
}

void TCPclientHandler::run() {
  PRINTLNFC(TCPmanager::DB, "Handling connection." << endl);

  codes_receiver->start();

  configure_telnet_client();
  write_ui();

  while (!is_terminating()) {
    wait_until_notifying();
    write_ui();
    unlock();
  }
}

void TCPclientHandler::configure_telnet_client() {
  using namespace telnet;
  sock.out() <<
             IAC << WILL << ECHO <<
             IAC << DO << ECHO <<
             IAC << DO << LINEMODE <<
             SOut::flush;
}

void TCPclientHandler::write_ui() {
  shared_ptr<string> text = to_write;
  PRINTLNFC(TCPmanager::DB, "mam text");

  if (text != nullptr) {
    bool written = false;
    while (!is_terminating() && !written) {
      try {
        sock.out() << telnet::NAOFFD << cons::CLEAR << *text <<
                   SOut::flush;
        written = true;
      }
      catch (exception const &e) {
        PRINTLNFC(TCPmanager::DB, "ERROR " << e.what())
      }
    }
  }
}

void TCPclientHandler::pass_code(string code) {
  tcp_manager->execute_command(code);
}