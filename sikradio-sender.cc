#include <iostream>
#include <csignal>
#include <cstring>
#include "err.h"
#include "program_args.h"
#include "udp_sock_and_msg.h"
#include "numbers.h"
#include "datagrams.h"
#include "siksender_shared_resource.h"
#include "siksender_main_thread.h"


using namespace std;

class Server {
  shared_ptr<SharedResource> resource;
  shared_ptr<MainThread> main_thread;

public:
  Server(string const &name_, string const &mcast_addr_,
         int data_port_, int ctrl_port_, size_t psize_, size_t fsize_,
         int rtime_) :
    resource(make_shared<SharedResource>(name_, mcast_addr_, data_port_,
                                         ctrl_port_,
                                         psize_, fsize_,
                                         rtime_)) {

    main_thread = SmartThread::create<MainThread>(resource);
  }

  void release() {
    SmartThread::terminate(main_thread);
  }

  void run() {
    main_thread->start();
    SmartThread::join(main_thread);
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

  try {
    signal(SIGINT, catch_int);
    SET_THREAD_NAME("sikradio-sender")

    string usage_txt = "Usage: " + string{argv[0]} + " -a MCAST_ADDR " +
                       "[-P DATA_PORT] [-C CTRL_PORT] [-p PSIZE] [-f FSIZE] [-R RTIME] "
                       "[-n NAZWA]\n";

    ProgramArgs args{argc, argv, usage_txt};

    string mcast_addr{args.force_val_for_opt("a")};
    string name{args.val_for_opt("n", "Nienazwany Nadajnik")};

    int data_port = args.intval_for_opt("P", 20000 + student_index % 10000);
    int ctrl_port = args.intval_for_opt("C", 30000 + student_index % 10000);

    size_t psize = args.intval_for_opt("p", 512);
    size_t fsize = args.intval_for_opt("f", 128 * 1024);
    int rtime = args.intval_for_opt("R", 250);

    Server server{name, mcast_addr, data_port, ctrl_port, psize,
                  fsize, rtime};
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