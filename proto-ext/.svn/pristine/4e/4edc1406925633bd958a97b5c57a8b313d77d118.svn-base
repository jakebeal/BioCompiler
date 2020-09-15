/* Master header for creating plugins for Proto compiler and simulator
Copyright (C) 2005-2010, Jonathan Bachrach, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef PROTO_SHARED_PLUGIN_H
#define PROTO_SHARED_PLUGIN_H

#include <string>

// FIXME: Provisional kludge, to keep compatibility with existing
// plugin source.
using namespace std;

// We generic calls and type string-matching to make extensibility easier
#define LAYER_PLUGIN "Layer"
#define DISTRIBUTION_PLUGIN "Distribution"
#define TIMEMODEL_PLUGIN "TimeModel"
#define EMITTER_PLUGIN "Emitter"

class Args;
class Compiler;

// Compiler plugins may not know the content of SCs.
class SpatialComputer;

// TODO: document this
class ProtoPluginLibrary {
 public:
  // FIXME: The string arguments here should be const string &, but
  // this changes the plugin API.

  /// Used to get simulator Layers, Distributions, and TimeModels.
  virtual void *get_sim_plugin(std::string type, std::string name, Args *args,
      SpatialComputer *cpu, int n)
    { return 0; }

  /// Used to get compiler extensions.
  virtual void *get_compiler_plugin(std::string type, std::string name,
      Args *args, Compiler *c)
    { return 0; }

  /// This defines the format that is required for registry entries
  static std::string registry_entry(std::string type, std::string name,
      std::string dll)
    { return type + " " + name + " = " + dll + "\n"; }
};

// Hook functions for external access to dynamically loaded library.
extern "C" {
  ProtoPluginLibrary *get_proto_plugin_library(void);
  const char *get_proto_plugin_inventory(void);
}

// Types for these functions, for casting the result of dlsym.
typedef ProtoPluginLibrary *(*get_library_func)(void);
typedef const char *(*get_inventory_func)(void);

#endif  // PROTO_SHARED_PLUGIN_H
