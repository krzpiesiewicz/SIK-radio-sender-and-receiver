#ifndef SIKRECEIVER_MAIN_THREAD_H
#define SIKRECEIVER_MAIN_THREAD_H

#include "smart_thread.h"
#include "time_alarm.h"
#include "sikreceiver_shared_resource.h"
#include "sikreceiver_lookup_sender.h"
#include "sikreceiver_replies_receiver.h"
#include "sikreceiver_tcp_manager.h"
#include "sikreceiver_audio_receiver.h"
#include "sikreceiver_audio_player.h"
#include "sikreceiver_rexmit_requester.h"

class MainThread : public ServerThread {

  const static bool DB = true;
  shared_ptr<LookupSender> lookup_sender;
  shared_ptr<RepliesReceiver> replies_receiver;
  shared_ptr<TCPmanager> tcp_manager;
  shared_ptr<ConditionVariableTimeAlarm> alarm;
  shared_ptr<AudioReceiver> audio_receiver;
  shared_ptr<AudioPlayer> audio_player;
  shared_ptr<RexmitRequester> rexmit_requester;

  chrono::system_clock::time_point earliest_reply_time =
    chrono::system_clock::now();

  chrono::milliseconds channel_expiration =
    chrono::duration_cast<chrono::milliseconds>(20s);

  bool ui_update_is_needed, channel_was_changed, any_channel_needed_to_play;

public:
  MainThread(shared_ptr<SharedResource> &resource_) :
    ServerThread("main", resource_) {
    
    add_release_function([this] {
      this->release();
    });

    UDPsocket tmp_sock;
    tmp_sock.set_reusing_address();
    tmp_sock.bind_to_any_port();

    resource->local_addr = tmp_sock.get_local_addr();

    lookup_sender = SmartThread::create<LookupSender>(ref(resource));
    replies_receiver = SmartThread::create<RepliesReceiver>(ref(resource));
    tcp_manager = SmartThread::create<TCPmanager>(ref(resource));
    alarm = SmartThread::create<ConditionVariableTimeAlarm>
      (ref(cv), ref(cv_m), channel_expiration, [this]() {
        lock_guard<mutex> lk(this->cv_m);
        this->resource->was_alarm = true;
        this->notify_without_lock_quard();
      });

    rexmit_requester = SmartThread::create<RexmitRequester>(resource);
    resource->rexmit_requester = rexmit_requester;

    audio_player = SmartThread::create<AudioPlayer>(resource);
    resource->audio_player = audio_player;

    audio_receiver = SmartThread::create<AudioReceiver>(resource);
    resource->audio_receiver = audio_receiver;
  }
  
  void release() {
    SmartThread::terminate(lookup_sender);
    SmartThread::terminate(replies_receiver);
    SmartThread::terminate(tcp_manager);
    SmartThread::terminate(audio_receiver);
    SmartThread::terminate(audio_player);
    SmartThread::terminate(rexmit_requester);
    SmartThread::terminate(alarm);
  }

  void run() override {
    lookup_sender->start();
    replies_receiver->start();
    tcp_manager->start();
    alarm->start();
    rexmit_requester->start();
    audio_player->start();
    audio_receiver->start();

    auto wakeup_predicate = [this] {
      return resource->channels.size() > 0
             && (resource->is_new_channel
                 || earliest_reply_time + channel_expiration >
                    chrono::system_clock::now()
                 || resource->was_alarm
                 || resource->curr_channel_change != 0
                 || resource->audio_playing_error
                 || (resource->audio_receiving_should_be_initialized
                     && resource->ready_for_audio_receiving));
    };

    while (!is_terminating()) {
      wait_until_notifying([&] {return wakeup_predicate();});
      unlock();

      lock_guard<mutex> channel_lk(resource->channels_mutex);

      ui_update_is_needed = resource->is_new_channel;
      channel_was_changed = false;
      any_channel_needed_to_play = resource->chosen_channel == resource->channels.end();

      // Checking channels:
      if (resource->is_new_channel || resource->was_alarm)
        check_channels_expiration_times();

      if (resource->name != "" && resource->is_new_channel)
        check_new_presence_of_exepected_channel();

      if (any_channel_needed_to_play)
        set_any_channel_to_play();

      if (resource->chosen_channel != resource->channels.end()
          && resource->curr_channel_change != 0)
        check_channel_change_by_user();

      if (resource->chosen_channel != resource->channels.end())
        PRINTLNFC(DB, "chosen = " << resource->chosen_channel->name)
      else
        PRINTLNFC(DB, "chosen = END");

      if (channel_was_changed || resource->audio_playing_error)
        stop_receiving_and_playing();

      if (resource->audio_receiving_should_be_initialized
          && resource->ready_for_audio_receiving)
        init_audio_receiving();

      if (ui_update_is_needed)
        update_tcp_clients();

      if (resource->was_alarm) {
        resource->was_alarm = false;
        set_alarm();
      }

      resource->is_new_channel = false;
      FLUSHC(DB)
    }
  }
private:

  void set_any_channel_to_play() {
    resource->chosen_channel = resource->channels.begin();
    channel_was_changed = true;
    ui_update_is_needed = true;
  }

  void check_channels_expiration_times() {
    auto now = chrono::system_clock::now();

    for (auto it = resource->channels.begin(); it != resource->channels.end(); it++)
      if (now - it->last_reply > channel_expiration) {
        PRINTLNC(DB, "RÓŻNICA W CZASIE = " <<
                                           chrono::duration_cast<chrono::milliseconds>
                                             (now - it->last_reply).count())
        if (resource->chosen_channel == it) {
          any_channel_needed_to_play = true;
          PRINTLNC(DB, "WYRZUCAM AKTYWNY KANAŁ")
        } else PRINTLNC(DB, "WYRZUCAM JAKIŚ KANAŁ")
        resource->channels.erase(it);
        ui_update_is_needed = true;
      }
  }

  void check_channel_change_by_user() {
    auto old_channel = resource->chosen_channel;

    PRINTLNC(DB, "cuur_channel_change " << resource->curr_channel_change);

    while (resource->curr_channel_change > 0) {
      resource->chosen_channel++;

      resource->curr_channel_change--;
      if (resource->chosen_channel == resource->channels.end()) {
        resource->chosen_channel--;
        resource->curr_channel_change = 0;
      }
    }

    while (resource->curr_channel_change < 0) {
      if (resource->chosen_channel == resource->channels.begin())
        resource->curr_channel_change = 0;
      else {
        resource->chosen_channel--;
        resource->curr_channel_change++;
      }
    }

    if (old_channel != resource->chosen_channel)
      channel_was_changed = true;
  }

  void check_new_presence_of_exepected_channel() {
    for (auto it = resource->channels.begin();
         it != resource->channels.end(); it++) {
      if (it->name == resource->name && it->is_new) {
        it->is_new = false;
        if (it == resource->chosen_channel)
          break;
        else {
          resource->chosen_channel = it;
          channel_was_changed = true;
          PRINTLNFC(DB, "wykryłem oczekiwany kanał.")
        }
      }
    }
  }

  void stop_receiving_and_playing() {
    PRINTLNC(DB, "stop receiving and playing")
    ui_update_is_needed = true;
    if (channel_was_changed)
      PRINTLNC(DB, "channel was change")

    if (resource->audio_playing_error)
      PRINTLNC(DB, "AUDIO PLAYING ERROR")

    resource->first_byte_num_was_set = false;
    resource->audio_receiving = false;
    resource->audio_playing = false;
    resource->audio_playing_error = false;

    audio_receiver->release_sock();
    rexmit_requester->release_sock();

    if (!resource->ready_for_audio_receiving)
      resource->barrier_should_be_reached = true;

    audio_player->notify();
    audio_receiver->notify();
    rexmit_requester->notify();

    if (resource->chosen_channel != resource->channels.end())
      resource->audio_receiving_should_be_initialized = true;
  }

  void init_audio_receiving() {
    PRINTLNC(DB, "init audio receiving")
    resource->audio_receiving_should_be_initialized = false;
    resource->ready_for_audio_receiving = false;

    resource->audio_addr = resource->chosen_channel->audio_addr;
    resource->ctrl_addr = resource->chosen_channel->ctrl_addr;

    resource->fifo.clear();

    resource->audio_playing_error = false;
    resource->first_byte_num_was_set = false;
    resource->audio_playing = false;
    resource->audio_receiving = true;
    resource->resources_for_receiving_ready = true;

    audio_player->notify();
    audio_receiver->notify();
    rexmit_requester->notify();
  }

  void update_tcp_clients() {
    for (auto it = resource->channels.begin(); it != resource->channels.end(); it++)
      PRINTLNC(DB, it->name << "  " << it->address << "  " << it->port << "\n");
    PRINTC(DB, "\n");

    lock_guard<mutex> lk_tcp(tcp_manager->cv_m);
    tcp_manager->create_menu();
    PRINTLNC(DB, "ZARAZ OBUDZĘ tcp_managera");
    tcp_manager->notify_without_lock_quard();
  }

  void set_alarm() {
    resource->was_alarm = false;
    earliest_reply_time = chrono::system_clock::now();
    for (auto it = resource->channels.begin();
         it != resource->channels.end(); it++)
      earliest_reply_time = min(earliest_reply_time, it->last_reply);

    auto now = chrono::system_clock::now();

    alarm->start_counting(channel_expiration - (now - earliest_reply_time));
  }
};

#endif // SIKRECEIVER_MAIN_THREAD_H