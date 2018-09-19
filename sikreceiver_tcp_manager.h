#ifndef SIKRECEIVER_TCP_MANAGER_H
#define SIKRECEIVER_TCP_MANAGER_H

#include <memory>
#include <sstream>
#include <unordered_map>

#include "sikreceiver_shared_resource.h"
#include "smart_thread.h"

using namespace std;

class TCPmanager : public ServerThread {
  static const int QUEUE_LENGTH = 20;
  static const int BUFF_SIZE = 1000;
  static const unsigned int CLIENTS_MAX = 20;

  unsigned int clients_count = 0;

  shared_ptr<SmartThread> new_connections_listener;

  shared_ptr<string> to_write;

  /**
   * User interface menu.
   */
  UIMenu ui_menu;

  /**
   * Map with textual console code as a key and Command as value.
   */
  unordered_map<string, function<void(void)>> commands;

  bool curr_connection_end;
  MenuElement *chosen_option;

  list<shared_ptr<SmartThread>> clients_handlers;

  friend class TCPclientHandler;
  friend class TCPnewConnectionsListener;
public:
  TCPmanager(shared_ptr<SharedResource> &resource_);

  void run() override;

  void create_menu();

  static const bool DB = false;
private:

  void set_write_to_from_menu();

  void set_commands();

  void execute_command(string command_key);

  void change_channel_by_client(int move);
};

#endif // SIKRECEIVER_TCP_MANAGER_H
