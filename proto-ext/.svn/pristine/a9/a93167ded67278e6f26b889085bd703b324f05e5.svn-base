/* Platform-specific definitions for protobo
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __PROTO_PLATFORM__
#define __PROTO_PLATFORM__

#define INLINE static inline
#define MAYBE_INLINE static inline
#define ATOMIC
#define VOID void

#define MAX_HOOD 4

#define MAX_MEM_SIZE (2048L - 256L)

#define IS_WORD_ALIGN 0

#define IS_COMPRESSED_COM_DATA 1

#define are_exports_serialized 1

#define MAX_SCRIPT_LEN 256L
#define MAX_SCRIPT_PKT 9
#define NUM_SCRIPT_PKTS (MAX_SCRIPT_LEN / MAX_SCRIPT_PKT)
#define MAX_DIGEST_PKT 4

#define MAX_SCRIPTS 1

#define MAX_PROBES 1

#define HUGE_VAL 1000000

extern float atan2f(float x, float y);

extern void * lookup_op_by_code (int code, char **name);

static inline void POST(char* pstring, ...) { }

static const int debug_id = -1;
static const int is_debugging_val = 0;
static const int is_tracing_val = 0;
static const int is_script_debug_val = 0;

#include "platform_ops.h"

extern VEC_VAL *read_coord_sensor (VOID);
extern NUM_VAL read_button (uint8_t n);
extern void set_r_led (NUM_VAL val);
extern void set_g_led (NUM_VAL val);
extern void set_b_led (NUM_VAL val);

#endif // __PROTO_PLATFORM
