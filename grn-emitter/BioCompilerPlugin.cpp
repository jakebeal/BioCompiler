/* Proto plugin glue code
Copyright (C) 2009-2011, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

#include "BioCompilerPlugin.h"

using namespace grn;

/*************** Plugin Library ***************/
void* BioCompilerPlugin::get_compiler_plugin(string type,string name,Args* args,Compiler* c){
  if(type == EMITTER_PLUGIN) {
    if(name == EMITTER_NAME) { return new GRNEmitter((NeoCompiler*)c, args); }
  }
  return NULL;
}

string BioCompilerPlugin::inventory() {
  return "# TASBE BioCompiler plugin\n" +
    registry_entry(EMITTER_PLUGIN,EMITTER_NAME,DLL_NAME);
}

extern "C" {
  ProtoPluginLibrary* get_proto_plugin_library() 
  { return new BioCompilerPlugin(); }
  const char* get_proto_plugin_inventory()
  { return (new string(BioCompilerPlugin::inventory()))->c_str(); }
}
