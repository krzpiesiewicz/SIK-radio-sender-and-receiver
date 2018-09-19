#include <iostream>
#include <csignal>
#include <cstring>

#include "err.h"
#include "tcp_socket.h"
#include "program_args.h"
#include "numbers.h"
#include "host_and_net.h"
#include "udp_sock_and_msg.h"
#include "datagrams.h"
#include "smart_thread.h"
#include "sikreceiver_shared_resource.h"
#include "sikreceiver_main_thread.h"

using namespace std;

class Server {
  shared_ptr<SharedResource> resource;
  shared_ptr<MainThread> main_thread;

public:
  Server(string discover_addres_, int ctrl_num_,
         int ui_port_, size_t psize_, size_t bsize_, int rtime_, string name_) {

    AudioDatagram::AUDIO_DATA_SIZE = psize_;

    resource = make_shared<SharedResource>(discover_addres_, ctrl_num_,
                                           ui_port_,
                                           psize_,
                                           bsize_, rtime_,
                                           name_);

    main_thread = SmartThread::create<MainThread>(resource);
    resource->main_thread = main_thread;
  }

  void release() {
    if (main_thread != nullptr) {
      main_thread->terminate();
    }
  }

  void run() {
    main_thread->start();
    main_thread->join();
  }
};

Server *server_ptr;
exception_ptr eptr;

/**
 * @param sig
 */
static void catch_int(int) {
  if (server_ptr != nullptr)
    server_ptr->release();
  eptr = make_exception_ptr(ServerException("Signal caugth.\n"));
}

int main(int argc, char *argv[]) {

  signal(SIGINT, catch_int);

  try {
    string usage_txt = "Usage: " + string{argv[0]} + " [-d DISCOVER_ADDR] " +
                       "[-C CTRL_PORT] [-U UI_PORT] [-p PSIZE] [-b BSIZE] "
                       "[-R RTIME] "
                       "[-n NAZWA]\n";

    SET_THREAD_NAME("sikradio-receiver")

    ProgramArgs args{argc, argv, usage_txt};

    string discover_addr{args.val_for_opt("d", "255.255.255.255")};
    string name{args.val_for_opt("n", "")};

    int ctrl_port = args.intval_for_opt("C", 30000 + student_index % 10000);
    int ui_port = args.intval_for_opt("U", 10000 + student_index % 10000);
    int rtime = args.intval_for_opt("R", 250);

    size_t psize = args.intval_for_opt("p", 512);
    size_t bsize = args.intval_for_opt("b", 64 * 1024);

    Server server{discover_addr, ctrl_port, ui_port, psize,
                  bsize, rtime, name};
    server_ptr = &server;
    server.run();
    server_ptr = nullptr;
    if (eptr != nullptr)
      std::rethrow_exception(eptr);
  } catch (exception const &e) {
    cerr << "ERROR: " << e.what() << " (" << errno << ") " << strerror(errno)
         << endl;
    return 1;
  }
  return 0;
}