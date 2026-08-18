#ifndef _CRUSH_PRIVATE_H
#define _CRUSH_PRIVATE_H

#include <stdbool.h>

//   true from the moment `console`'s interactive (stdin, tty) path registers its own periodic
// task until that task ends the session -- see console.c's _interactive_step(). crush_app_main()
// checks this before its usual "run once then shut down": that task needs more than one tick to
// do its job, and it is registered AFTER crush_app_main's own periodic task, so an unconditional
// shutdown on crush_app_main's first call would end the whole process before the console task
// ever ran once.
extern bool crush_console_is_active(void);

#endif
