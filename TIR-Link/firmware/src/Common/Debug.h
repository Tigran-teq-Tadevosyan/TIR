#ifndef _DEBUG_H
#define _DEBUG_H

#include <inttypes.h>

#define TIR_LINK_DEBUG_LEVEL0

void init_Debug(void);
void printDebug (const char * format, ... );
void printDebugArray(const char *prepend, const void *data, uint32_t length);

#endif
