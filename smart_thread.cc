#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <memory>
#include <functional>
#include <iostream>
#include "smart_thread.h"
#include "debug.h"

using namespace std;

void SmartThread::join(std::shared_ptr<SmartThread> thread_ptr) {
  if (thread_ptr != nullptr)
    thread_ptr->join();
}

void SmartThread::terminate(std::shared_ptr<SmartThread> thread_ptr) {
  if (thread_ptr != nullptr)
    thread_ptr->terminate();
}


SmartThread::SmartThread(string name_,
  chrono::milliseconds interval_duration_) :
  name(name_), lk(cv_m), interval_duration(interval_duration_) {
  unlock();
  PRINTLNFC(DB, "Tworzę wątek")
}

SmartThread::~SmartThread() {
  try {
    PRINTLNFC(DB, "Destructing " << name)
    if (!terminated)
      terminate();
    // joining:
    if (thread_sh != nullptr && thread_sh->joinable()) {
      thread_sh->join();
    } else {
      PRINTLNFC(DB, "nie ma co joinowac w " << name)
    }
    terminated = true;
  }
  catch (exception &e) {
    PRINTLNFC(DB, "ERROR while destroying thread " << name << ": " << e.what())
  }
}

void SmartThread::start() {
  lock_guard<mutex> lk(cv_m);
  if (!terminating) {
    running = true;
    if (thread_sh == nullptr) {
      bool started = false;
      while (!is_terminating() && !started) {
        try {
          thread_sh = shared_ptr<thread>(
            new thread([this]() {
              auto ptr = this->this_smart_thread;
              ptr->run_helper_start();
            }));
          started = true;
        }
        catch (exception const & e) {
          PRINTLNFC(DB, "ERROR " << e.what())
          sleep_for(250ms);
        }
      }
    }
  }
  cv.notify_all();
}

void SmartThread::stop() {
  lock_guard<mutex> lk(cv_m);
  if (!terminating)
    running = false;
}

void SmartThread::join() {
  unique_lock<mutex> lk(cv_m);
  PRINTLNFC(DB, "Czekam do zakończenia wątku " << name)
  cv.wait(lk, [&]{return terminated == true;});
}

void SmartThread::terminate() {
  if (!terminating) {
    unique_lock<mutex> lk(cv_m);
    PRINTLNFC(DB, "Robię terminate na " << name)
    terminating = true;
    release_thread_blocking_resources_helper();
    cv.notify_all();
  }
}

void SmartThread::notify_without_lock_quard() {
  sleeping = false;
  notified = true;
  cv.notify_all();
}

void SmartThread::notify() {
  lock_guard<mutex> lk(cv_m);
  sleeping = false;
  notified = true;
  cv.notify_all();
}

void SmartThread::lock() {
  lk.lock();
  locked = true;
}
void SmartThread::unlock() {
  locked = false;
  lk.unlock();
}

void
SmartThread::set_interval_duration(chrono::milliseconds interval_duration_) {
  interval_duration = interval_duration_;
}

void SmartThread::sleep_for(chrono::milliseconds duration_) {
  chrono::system_clock::time_point end_point{chrono::system_clock::now() +
                                             duration_};
  sleeping = true;
  while (!terminating && sleeping && chrono::system_clock::now() < end_point) {
    this_thread::sleep_for(interval_duration.load());
  }
  sleeping = false;
}

void SmartThread::wait_until_notifying(function<bool()> predicate) {
  wait_until([this, predicate] {
    return notified && predicate();
  });
}

void SmartThread::wait_until(function<bool()> predicate) {
  lock();
  auto pred = [this, predicate] {
    return terminating || predicate();
  };
  if (!pred())
    cv.wait(lk, pred);
  notified = false;
  // we inherits critical section (lk, cv_m)
  if (terminating) {
    unlock();
    throw TerminateException();
  }
}

bool change_atomic_bool_value(atomic<bool> &atom, bool expected_) {
  bool expected = expected_;
  return atom.compare_exchange_strong(expected, true);
}

void SmartThread::set_thread_name_for_db() {
  SET_THREAD_NAME(name)
}

void SmartThread::run_helper_start() {
  set_thread_name_for_db();
  try {
    PRINTLNFC(DB, "Jestem nowym wątkiem")
    run();
  }
  catch (TerminateException const &e) {
    terminating = true;
    PRINTLNFC(DB, "Złapałem terminate")
  }
  catch (exception const &e) {
    cerr << "ERROR in " << name << ": " << e.what() << "\n";
  }
  terminate_helper();
}

void SmartThread::terminate_helper() {
//  if (!locked)
//    lock();
  lock_guard<mutex> lk(cv_m);
  running = true;
  sleeping = false;
  release_thread_blocking_resources_helper();
  running = false;
  cv.notify_all();
  terminated = true;
}

void SmartThread::release_thread_blocking_resources_helper() {
  if (!released_thread_blocking) {
    released_thread_blocking = true;
    PRINTLNFC(DB, "Bedzie czysto - release w " << name)
    try {
      for (auto f : releasing_functions)
        f();
    }
    catch (exception &e) {
      cerr << "ERROR while releasing in " << name << ": " << e.what() << "\n";
    }
    releasing_functions.clear();
  }
}