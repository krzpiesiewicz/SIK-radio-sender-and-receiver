#include "telnet.h"

namespace telnet {
// Telnet commands:
  const char WILL = 251;   // Will option code
  const char WONT = 252;   // Won't option code
  const char DO = 253;   // Do option code
  const char DONT = 254;   // Don't option code
  const char IAC = 255;   // Interpret as command
  extern const char GA = 249; // Go ahead

// Telnet options:
  const char TRANSMIT_BINARY = 0;  // Binary Transmission (RFC856)
  const char ECHO = 1;  // Echo (RFC857)
  const char SUPPRESS_GO_AHEAD = 3;  // Suppress Go Ahead (RFC858)
  const char STATUS = 5;  // Status (RFC859)
  const char TIMING_MARK = 6;  // Timing Mark (RFC860)
  const char NAOCRD = 10;  // Output Carriage-Return Disposition (RFC652)
  const char NAOHTS = 11;  // Output Horizontal Tab Stops (RFC653)
  const char NAOHTD = 12;  // Output Horizontal Tab Stop Disposition (RFC654)
  const char NAOFFD = 13;  // Output Formfeed Disposition (RFC655)
  const char NAOVTS = 14;  // Output Vertical Tabstops (RFC656)
  const char NAOVTD = 15;  // Output Vertical Tab Disposition (RFC657)
  const char NAOLFD = 16;  // Output Linefeed Disposition (RFC658)
  const char EXTEND_ASCII = 17;  // Extended ASCII (RFC698)
  const char TERMINAL_TYPE = 24;  // Terminal Type (RFC1091)
  const char NAWS = 31;  // Negotiate About Window Size (RFC1073)
  const char TERMINAL_SPEED = 32;  // Terminal Speed (RFC1079)
  const char TOGGLE_FLOW_CONTROL = 33;  // Remote Flow Control (RFC1372)
  const char LINEMODE = 34;  // Linemode (RFC1184)
  const char AUTHENTICATION = 37;  // Authentication (RFC1416)
}