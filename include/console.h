#ifndef FLIP_RP2040_CONSOLE_H
#define FLIP_RP2040_CONSOLE_H
#include "fluid.h"
#include "input.h"
void console_announce(void);
/* Process at most one pending USB byte. True after a blocking benchmark;
 * caller must reset its scheduling deadline and statistics window. */
bool console_poll(Fluid *fluid, InputState *input);
#endif
