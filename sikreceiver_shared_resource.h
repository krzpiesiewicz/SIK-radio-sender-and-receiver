#ifndef SIKRECEIVER_SHARED_RESOURCE_H
#define SIKRECEIVER_SHARED_RESOURCE_H

#include "smart_thread.h"
#include "tcp_socket.h"
#include "radio_channel.h"
#include "menu.h"
#include "host_and_net.h"
#include "fifo.h"
#include "datagrams.h"

using namespace std;

struct SharedResource {
  shared_ptr<SmartThread> main_thread, audio_receiver, audio_player, rexmit_requester;
  sockaddr_in audio_addr, ctrl_addr, discover_addr, local_addr, ui_addr;

  FIFO<AudioDatagram> fifo;
  size_t psize, bsize;

  atomic<uint64_t> session_id, first_byte_num, last_byte_num, curr_byte_num;
  atomic<bool> audio_receiving = false, audio_playing = false,
    is_new_channel = false, first_byte_num_was_set = false,
    audio_playing_error = false, was_alarm = true,
    ready_for_audio_receiving = false, barrier_should_be_reached = true,
    audio_receiving_should_be_initialized = false,
    resources_for_receiving_ready = false;
  int rtime;
  string name;

  int barrier_size = 3;
  atomic<int> ready_cnt = 0, broken_cnt = barrier_size;

  mutex channels_mutex, barrier_mutex;

  multiset<RadioChannel, RadioChannel::cmp> channels;
  multiset<RadioChannel, RadioChannel::cmp>::iterator chosen_channel;
  int curr_channel_change = 0;

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

  SharedResource(string discover_addres_, int ctrl_num_, int ui_port_,
                 size_t psize_, size_t bsize_, int rtime_, string name_) :
    discover_addr(create_sockaddr_in(discover_addres_, ctrl_num_)),
    fifo(bsize_ / psize_), psize(psize_), bsize(bsize_),
    rtime(rtime_), name(name_) {

    for (int i = 0; i < fifo.fifo_size; i++) {
      AudioDatagram & dt = fifo.buff[i].t;
      dt.session_id(-1);
      dt.first_byte_num(0);
    }

    ctrl_addr.sin_port = htons(cast_port(ctrl_num_));
    ui_addr.sin_port = htons(cast_port(ui_port_));

    chosen_channel = channels.end();
  }

  void notify_threads() {
    audio_player->notify();
    rexmit_requester->notify();
    audio_receiver->notify();
  }
};

class ServerThread : public SmartThread {
protected:
  shared_ptr<SharedResource> resource;
public:
  ServerThread(std::string name_, shared_ptr<SharedResource> &resource_) :
    SmartThread(name_), resource(resource_) {}

  virtual ~ServerThread() {};

  virtual void run() override = 0;

  void reach_barrier() {
    wait_until([this] {return resource->broken_cnt == resource->barrier_size;});
    unlock();
    {
      unique_lock<std::mutex> lk(resource->barrier_mutex);
      if (resource->ready_cnt + 1 == resource->barrier_size) {
        {
          lock_guard<mutex> lk2(resource->channels_mutex);
          resource->resources_for_receiving_ready = false;
          resource->ready_for_audio_receiving = true;
          resource->barrier_should_be_reached = false;
          resource->main_thread->notify();
        }
        resource->ready_cnt++;
        resource->broken_cnt = 0;
        resource->notify_threads();
      }
      else {
        resource->ready_cnt++;
        lk.unlock();
        wait_until([this] {return resource->ready_cnt == resource->barrier_size;});
        unlock();
      }
    }
    {
      lock_guard<std::mutex> lk(resource->barrier_mutex);

      if (resource->broken_cnt + 1 == resource->barrier_size) {
        resource->ready_cnt = 0;
        resource->broken_cnt++;
        resource->notify_threads();
      } else
        resource->broken_cnt++;
    }

    wait_until([this] {return resource->barrier_should_be_reached
                              || resource->resources_for_receiving_ready;
    });
    unlock();
  }
};

#endif // SIKRECEIVER_SHARED_RESOURCE_H
