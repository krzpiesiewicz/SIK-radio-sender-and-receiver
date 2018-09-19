#ifndef TIME_ALARM_H
#define TIME_ALARM_H

#include <thread>
#include <condition_variable>
#include <chrono>
#include <mutex>
#include <memory>
#include <functional>

#include "debug.h"
#include "smart_thread.h"

class ConditionVariableTimeAlarm : public SmartThread {
  std::condition_variable & client_cv;
  std::mutex & client_cv_m;
  std::atomic<std::chrono::milliseconds> duration;
  std::atomic<bool> is_new_request = false;
  std::function<void()> notify_client = [this] {
    std::lock_guard<std::mutex> client_lk(client_cv_m);
    client_cv.notify_all();
  };
public:
  ConditionVariableTimeAlarm(
    std::condition_variable & cv_,
    std::mutex & cv_m_,
    std::chrono::duration<float> duration_,
    std::function<void()> notify_client_ = nullptr) :
    SmartThread(get_next_name()),
    client_cv(cv_), client_cv_m(cv_m_),
    duration(std::chrono::duration_cast<std::chrono::milliseconds>(duration_)) {

    if (notify_client_ != nullptr)
      notify_client = notify_client_;
  }

  void run() override {

    while (!is_terminating()) {
//      PRINTLNF("alarm working");
      wait_until_notifying([this]() {return is_new_request.load();});

      if (is_new_request) {
//        PRINTLNF("new request")
        is_new_request = false;
        lk.unlock();
        sleep_for(duration);
        notify_client();
      }
      else
        lk.unlock();
    }
  }

  void start_counting(std::chrono::duration<float> duration_) {
    std::lock_guard<std::mutex> lock(cv_m);
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(duration_);
    is_new_request = true;
    notify_without_lock_quard();
  }

  void start_counting() {
    std::lock_guard<std::mutex> lock(cv_m);
    is_new_request = true;
    notify_without_lock_quard();
  }
private:
  static std::string get_next_name() {
    static std::atomic<int> id = 0;
    id++;
    return "alarm id=" + std::to_string(id) + " created by " + GET_THREAD_NAME();
  }
};

#endif // TIME_ALARM_H
