#include "sikreceiver_tcp_manager.h"
#include "sikreceiver_tcp_client_handler.h"
#include "sikreceiver_tcp_new_connections_listener.h"

TCPnewConnectionsListener::TCPnewConnectionsListener(TCPmanager *tcp_manager_, int port_num) :
  SmartThread("tcp_new_connections_listener"),
  tcp_manager(tcp_manager_),
  sock(port_num, TCPmanager::BUFF_SIZE, TCPmanager::QUEUE_LENGTH) {

  add_release_function([this] {
    sock.release();
    PRINTLNFC(TCPmanager::DB, "Sock released." << endl);
  });
}

void TCPnewConnectionsListener::run() {
  bool ok = false;
  while (!is_terminating() && !ok)
    try {
      sock.listen_for_connections();
      ok = true;
    } catch (exception const &e) {
      PRINTLNFC(TCPmanager::DB, "ERROR " << e.what());
    }

  while (!is_terminating()) {
    sleep_for(20ms);
    if (tcp_manager->clients_count < tcp_manager->CLIENTS_MAX) {
      try {
        sock.accept_connection();
        lock_guard<mutex> manager_lk(tcp_manager->cv_m);
        shared_ptr<TCPclientHandler> t_sh = SmartThread::create<TCPclientHandler>
          (tcp_manager, sock.msg_sock_descriptor());
        tcp_manager->clients_handlers.push_back(t_sh);
        tcp_manager->clients_count++;
        t_sh->start();
      }
      catch (exception const &e) {
        PRINTLNFC(TCPmanager::DB, "ERROR " << e.what());
      }
    }
    else
      sleep_for(200ms);
  }
}