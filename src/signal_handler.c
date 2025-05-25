#include "../include/signal_handler.h"
#include <stdio.h>  // For fprintf
#include <string.h> // For memset

// Global atomic flag to control server loop
volatile sig_atomic_t keep_running = 1;

// Signal handler function
void signal_handler(int signum) {
  if (signum == SIGINT) {
    fprintf(stderr, "\nSIGINT received. Shutting down server...\n");
    keep_running = 0; // Set the flag to stop the main loop
  }
}

// Function to set up the signal handler
int setup_signal_handler() {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = signal_handler; // Set our custom handler
  sigemptyset(&sa.sa_mask);       // Clear mask of blocked signals
  sa.sa_flags = 0;                // No special flags
  if (sigaction(SIGINT, &sa, NULL) ==
      -1) { // Register handler for SIGINT (Ctrl+C)
    perror("sigaction failed");
    return -1;
  }
  return 0;
}
