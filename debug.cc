#include "debug.h"

#include <unordered_map>
#include <string>
#include <thread>

#ifdef DEBUG

using namespace std;

unordered_map<thread::id, thdb::ThreadVars> & vars_map() {
  static unordered_map<thread::id, thdb::ThreadVars> map;
  return map;
};

thdb::ThreadVars & thdb::get_thread_vars() {
  auto it = vars_map().find(this_thread::get_id());
  if (it == vars_map().end()) {
    auto key = this_thread::get_id();
    vars_map().insert({key, thdb::ThreadVars()});
    return get_thread_vars();
  }
  else
    return it->second;
}

void thdb::set_thread_name(string name) {
  thdb::get_thread_vars().name = name;
}

string thdb::get_thread_name() {
  return thdb::get_thread_vars().name;
}

#endif