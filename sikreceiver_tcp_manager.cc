#include <memory>
#include <sstream>
#include <unordered_map>

#include "sikreceiver_shared_resource.h"
#include "smart_thread.h"
#include "telnet.h"
#include "console_codes.h"
#include "sikreceiver_tcp_manager.h"
#include "sikreceiver_tcp_new_connections_listener.h"
#include "sikreceiver_tcp_client_handler.h"

using namespace std;

TCPmanager::TCPmanager(shared_ptr<SharedResource> &resource_) :
  ServerThread("tcp_manager", resource_) {

  set_commands();

  add_release_function([this] {
    SmartThread::terminate(new_connections_listener);
    for (auto it = clients_handlers.begin(); it != clients_handlers.end(); it++)
      SmartThread::terminate((*it));
  });

  create_menu();
  set_write_to_from_menu();
}

void TCPmanager::run() {
  new_connections_listener = SmartThread::create<TCPnewConnectionsListener>
    (this, ntohs(resource->ui_addr.sin_port));

  new_connections_listener->start();

  while (!is_terminating()) {
    wait_until_notifying();
    PRINTLNFC(TCPmanager::DB, "OBUDZIŁEM SIĘ!!!")
    set_write_to_from_menu();
    unlock();
  }
}

string long_dash = "------------------------------------------------------------------------";

void TCPmanager::set_write_to_from_menu() {
  stringstream ss;
  ss << long_dash << "\n" << telnet::NAOFFD << "  SIK Radio" << "\n" <<
     telnet::NAOFFD << long_dash << "\n" << telnet::NAOFFD <<
     ui_menu << long_dash;
  to_write = make_shared<string>(ss.str());

  for (auto it = clients_handlers.begin();
       it != clients_handlers.end(); it++) {
    if ((*it)->is_terminating()) {
      PRINTLNC(TCPmanager::DB, "Usuwam client handlera");
      clients_handlers.erase(it);
      clients_count--;
    } else {
      lock_guard<mutex> handler_lk((*it)->cv_m);
      PRINTLNC(TCPmanager::DB, "Zmieniłem to_write");
      auto ch = dynamic_pointer_cast<TCPclientHandler>(*it);
      ch->to_write = to_write;
      ch->notify_without_lock_quard();
    }
  }
  FLUSHC(TCPmanager::DB);
}

void TCPmanager::create_menu() {
  auto root = make_shared<MenuElement>();

  int j = 0, shift = 0;
  list<shared_ptr<MenuElement>> channels_opts;
  for (auto it = resource->channels.begin(); it != resource->channels.end();
       it++) {
    channels_opts.push_back(make_shared<MenuElement>(it->name));
    root->add_child(*channels_opts.rbegin());

    if (it == resource->chosen_channel)
      shift = j;
    j++;
  }

  ui_menu.set_root(root);
  for (int i = 0; i < shift; i++)
    ui_menu.select_next();
}

void TCPmanager::execute_command(string code) {
  auto it = commands.find(code);
  if (it != commands.end()) {
    it->second();
  }
}

void TCPmanager::set_commands() {
  commands[cons::KEY_UP] = [&] {change_channel_by_client(-1);};
  commands[cons::KEY_DOWN] = [&] {change_channel_by_client(1);};
}

void TCPmanager::change_channel_by_client(int move) {
  lock_guard<mutex> lk_main(resource->main_thread->cv_m);
  resource->curr_channel_change = move;
  resource->main_thread->notify_without_lock_quard();
}