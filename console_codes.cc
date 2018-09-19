#include "console_codes.h"

namespace cons {
  // displaying:
  const char *NO_COLOR = "\e[0m";
  const char *YELOW_BACKGROUND = "\e[48;5;226m";
  const char *CYAN = "\e[1;96m";
  const char *CLEAR = "\ec\e[3J";
  const char *HIDE_CURSOR = "\e[?25l";
  const char *SHOW_CURSOR = "\e[?25h";

  // input:
  const char *KEY_UP = "\e[A";
  const char *KEY_DOWN = "\e[B";
  const char *KEY_ENTER = "\15";
  const char *KEY_PAGE_UP = "\e[5~";
}