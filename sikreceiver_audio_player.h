#ifndef SIKRECEIVER_AUDIO_PLAYER_H
#define SIKRECEIVER_AUDIO_PLAYER_H

#include "sikreceiver_shared_resource.h"
#include "smart_thread.h"
#include "string"

class AudioPlayer : public ServerThread {
  static const bool DB1 = false, DB2 = true;
public:
  AudioPlayer(shared_ptr<SharedResource> &resource_) :
    ServerThread("audio_player", resource_) {}

  void run() override {

    while (!is_terminating()) {

      reach_barrier();

      wait_until([this] {return resource->barrier_should_be_reached
                             || resource->audio_playing;
      });
      unlock();
      if (resource->barrier_should_be_reached)
        continue;

      PRINTLNF("zaczynam grać")

      ios_base::sync_with_stdio(false);

      auto &fifo = resource->fifo;
      auto &buff = fifo.buff;
      uint64_t next_byte_num = resource->curr_byte_num;
      int64_t dt_num = next_byte_num / resource->psize;

      fifo.lock();
      int idx = fifo.idx_for_num(dt_num);
      fifo.unlock();

      while (!is_terminating() && !resource->barrier_should_be_reached
             && resource->audio_playing) {

        const auto &elem = buff[idx];
        if (elem.num != dt_num) {
          PRINTLNFC(DB2, "Brakuje paczki " << dt_num << " (byte_num " <<
                    next_byte_num << "), jest tam " << elem.num <<
                    " (byte_num " << elem.num * resource->psize << ")")
          {
            lock_guard<mutex> lk(resource->channels_mutex);
            resource->audio_playing_error = true;
            resource->barrier_should_be_reached = true;
            resource->main_thread->notify();
            break;
          }
        } else {

          PRINTLNFC(DB1, "wypiszę paczkę na AUDIO!!!!! JUPI!!! " <<
                    next_byte_num / resource->psize << "(byte " <<
                    next_byte_num << ")")

          next_byte_num += resource->psize;
          resource->curr_byte_num = next_byte_num;
          dt_num++;
          idx = fifo.idx_plus(idx);

          for (int i = 0; i < (int) resource->psize; i++) {
            cout << elem.t.audio_data()[i];
          }
          cout << flush;
        }
      }
    }
  }
};

#endif // SIKRECEIVER_AUDIO_PLAYER_H
