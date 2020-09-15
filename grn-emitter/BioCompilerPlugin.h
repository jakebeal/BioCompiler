/* Proto plugin glue code
Copyright (C) 2009-2011, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

#include <proto/proto_plugin.h>
#include <proto/compiler.h>
#include "biocompiler.h"
#define EMITTER_NAME "grn"
#define DLL_NAME "libbiocompiler"

class BioCompilerPlugin : public ProtoPluginLibrary {
public:
  void* get_compiler_plugin(string type, string name, Args* args,Compiler* c);
  static string inventory();
};

