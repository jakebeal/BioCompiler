/* ProtoKernel opcodes; also version info
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __PROTO_OPCODES__
#define __PROTO_OPCODES__

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_GLO_REF_OPS     4
#define MAX_REF_OPS         4
#define MAX_LET_OPS         4
#define MAX_DEF_FUN_OPS     6
#define MAX_DEF_NUM_VEC_OPS 3
#define MAX_LIT_OPS         5

typedef enum {
  #define X(a) a,
  #include "opcodes.def"
  #undef X
  CORE_CMD_OPS,
} CORE_OPCODES;



char const* const CORE_OPCODES_STR[] = {
  #define X(a) #a,
  #include "opcodes.def"
  #undef X
  0
};

typedef uint8_t OPCODE;

#ifdef __cplusplus
}
#endif
#endif


