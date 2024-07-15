#pragma once

#ifdef __linux__
#    include <signal.h>

extern volatile sig_atomic_t* sigchld;
#endif