#ifndef TELNET_H
#define TELNET_H

namespace telnet {
// Telnet commands:
  extern const char WILL;   // Will option code
  extern const char WONT;   // Won't option code
  extern const char DO;   // Do option code
  extern const char DONT;   // Don't option code
  extern const char IAC;   // Interpret as command
  extern const char GA; // Go ahead

// Telnet options:
  extern const char TRANSMIT_BINARY;  // Binary Transmission (RFC856)
  extern const char ECHO;  // Echo (RFC857)
  extern const char SUPPRESS_GO_AHEAD;  // Suppress Go Ahead (RFC858)
  extern const char STATUS;  // Status (RFC859)
  extern const char TIMING_MARK;  // Timing Mark (RFC860)
  extern const char NAOCRD;  // Output Carriage-Return Disposition (RFC652)
  extern const char NAOHTS;  // Output Horizontal Tab Stops (RFC653)
  extern const char NAOHTD;  // Output Horizontal Tab Stop Disposition (RFC654)
  extern const char NAOFFD;  // Output Formfeed Disposition (RFC655)
  extern const char NAOVTS;  // Output Vertical Tabstops (RFC656)
  extern const char NAOVTD;  // Output Vertical Tab Disposition (RFC657)
  extern const char NAOLFD;  // Output Linefeed Disposition (RFC658)
  extern const char EXTEND_ASCII;  // Extended ASCII (RFC698)
  extern const char TERMINAL_TYPE;  // Terminal Type (RFC1091)
  extern const char NAWS;  // Negotiate About Window Size (RFC1073)
  extern const char TERMINAL_SPEED;  // Terminal Speed (RFC1079)
  extern const char TOGGLE_FLOW_CONTROL;  // Remote Flow Control (RFC1372)
  extern const char LINEMODE;  // Linemode (RFC1184)
  extern const char AUTHENTICATION;  // Authentication (RFC1416)
}

#endif // TELNET_H
