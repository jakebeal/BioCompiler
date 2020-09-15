/* Proto compiler
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef PROTO_COMPILER_PALEOCOMPILER_H
#define PROTO_COMPILER_PALEOCOMPILER_H

#include <inttypes.h>

#include <string>

#include "compiler.h"
#include "proto_opcodes.h"
#include "utils.h"

extern void init_compiler(void);
extern uint8_t* compile_script (char *str, int *len, int is_dump_ast);
extern void dump_instructions(int is_c, int n, uint8_t *bytes);

class PaleoCompiler : public Compiler {
 public:
  bool is_show_code;
  bool is_dump_code;
  bool is_dump_ast;
  bool is_echo_defops;
  bool is_json_format;
  const char *last_script;

  PaleoCompiler(Args *args);
  ~PaleoCompiler();
  void init_standalone(Args *args); // setup output files as standalone app
  uint8_t *compile(const char *str, int* len); // len is filled in w. output length
  void visualize();
  bool handle_key(KeyEvent *key);
  void set_platform(const std::string &path) { uerror("Set platform not used any more in paleocompiler."); }
  void setDefops(const std::string &defops);
};

#endif  // PROTO_COMPILER_PALEOCOMPILER_H
