#ifndef DEBUG_H
#define DEBUG_H

#include <string>
#include <sstream>
#include <unordered_map>
#include <thread>

#ifdef DEBUG

namespace thdb {
  struct ThreadVars {
    std::string name;
    std::thread::id id;
    std::stringstream deb_sstream;
  };

  ThreadVars & get_thread_vars();
  void set_thread_name(std::string name);
  std::string get_thread_name();
}

#define SET_THREAD_NAME(name) {thdb::set_thread_name(name);}
#define GET_THREAD_NAME() thdb::get_thread_name()

#define FLUSH() {\
  auto& vars = thdb::get_thread_vars();\
  std::clog << vars.deb_sstream.str() << std::flush;\
  vars.deb_sstream.str("");\
}

#define PRINT(val) {thdb::get_thread_vars().deb_sstream << val;}

#define PRINTF(val) {PRINT(val) FLUSH()}

#define PRINTLN(val) {\
  auto& vars = thdb::get_thread_vars();\
  if (vars.name != "")\
    vars.deb_sstream << vars.name << ": ";\
  vars.deb_sstream << val << "\n";\
}

#define PRINTLNF(val) {PRINTLN(val) FLUSH()}
#define DEB(instr) {instr}

#define FLUSHC(cond) {if (cond) FLUSH()}
#define PRINTC(cond, val) {if (cond) PRINT(val)}
#define PRINTFC(cond, val) {if (cond) PRINTF(val)}
#define PRINTLNC(cond, val) {if (cond) PRINTLN(val)}
#define PRINTLNFC(cond, val) {if (cond) PRINTLNF(val)}
#define DEBC(cond, instr) {if (cond) DEB(instr)}

#else

#define SET_THREAD_NAME(name) {}
#define GET_THREAD_NAME() ""

#define FLUSH() {}
#define PRINT(val) {}
#define PRINTF(val) {}
#define PRINTLN(val) {}
#define PRINTLNF(val) {}
#define DEB(instr) {}

#define FLUSHC(cond) {}
#define PRINTC(cond, val) {}
#define PRINTFC(cond, val) {}
#define PRINTLNC(cond, val) {}
#define PRINTLNFC(cond, val) {}
#define DEBC(cond, instr) {}

#endif

#endif // DEBUG_H
