#ifndef TIME_UTIL_H
#define TIME_UTIL_H

#include <ostream>
#include <ratio>
#include <chrono>
#include <iomanip>

string
display_time(std::chrono::duration<float> duration) {
  using namespace std;
  using namespace std::chrono;
  stringstream os{""};
  os.fill('0');

  auto ms = duration_cast<milliseconds>(duration);

  auto h = duration_cast<hours>(ms);
  ms -= h;
  auto m = duration_cast<minutes>(ms);
  ms -= m;
  auto s = duration_cast<seconds>(ms);
  ms -= s;
  
  os << setw(2) << m.count() << "m:"
     << setw(2) << s.count() << 's'
     << setw(2) << ms.count() << "ms";
  return os.str();
};

#endif // TIME_UTIL_H
