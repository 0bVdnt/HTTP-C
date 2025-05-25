#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <signal.h> // For sig_atomic_t

// Global atomic flag to control server loop
// 'extern' because it's defined in sigal_handler.c
extern volatile sig_atomic_t keep_running;

// Signal handler function
void signal_handler(int signum);

// Function to set up the signal handler
int setup_signal_handler();

#endif // SIGNAL_HANDLER_H
