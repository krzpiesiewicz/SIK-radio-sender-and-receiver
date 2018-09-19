#ifndef SMART_THREAD_H
#define SMART_THREAD_H

#include <thread>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <list>
#include "debug.h"

class SmartThread {
  std::atomic<bool>
    sleeping = false,
    running = false,
    terminating = false,
    released_thread_blocking = false,
    locked = true,
    terminated = false;
  bool DB = false;
public:
  std::condition_variable cv;
  std::mutex cv_m;
protected:
  class TerminateException : std::exception {};

  std::shared_ptr<SmartThread> this_smart_thread;
  std::string name;
  std::unique_lock<std::mutex> lk;
  std::atomic<std::chrono::milliseconds> interval_duration;
  std::atomic<bool> notified = false;
  std::shared_ptr<std::thread> thread_sh;
  std::list<std::function<void()>> releasing_functions;

  SmartThread(std::string name_,
    std::chrono::milliseconds interval_duration_ = std::chrono::milliseconds(50));

  template <typename ThreadType,
    typename std::enable_if<std::is_base_of<SmartThread, ThreadType>::value>::type* = nullptr>
  std::shared_ptr<ThreadType> get_this_thread() {
    return std::static_pointer_cast<ThreadType>(this_smart_thread);
  };
public:
  template <typename ThreadType,
    typename std::enable_if<std::is_base_of<SmartThread, ThreadType>::value>::type* = nullptr,
    class ... Types>
  static std::shared_ptr<ThreadType> create(Types ... args) {
    auto ptr = std::make_shared<ThreadType>(args...);
    ptr->this_smart_thread = std::static_pointer_cast<SmartThread>(ptr);
    return ptr;
  }

  static void join(std::shared_ptr<SmartThread> thread_ptr);
  static void terminate(std::shared_ptr<SmartThread> thread_ptr);

  virtual ~SmartThread();

  virtual void run() = 0;

  void start();
  void stop();
  void join();
  void terminate();
  void notify_without_lock_quard();
  void notify();

  void lock();
  void unlock();

  bool is_sleeping() {return sleeping;}
  bool is_running() {return running;}
  bool is_terminating() {return terminating;}
  bool is_terminated() {return terminated;}

  void set_interval_duration(std::chrono::milliseconds interval_duration_);
protected:
  void sleep_for(std::chrono::milliseconds duration_);

  void wait_until(std::function<bool()> predicate);

  void wait_until_notifying(std::function<bool()> predicate = [] {return true;});

  void add_release_function(std::function<void()> f) {
    releasing_functions.push_back(f);
  }
private:
  void change_atomic_bool_value(std::atomic<bool> &atom, bool expected);
  void set_thread_name_for_db();
  void run_helper_start();
  void terminate_helper();
  void release_thread_blocking_resources_helper();
};

#endif // SMART_THREAD_H
