/* Mechanisms for handling plugin registry and loading
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef PROTO_SHARED_PLUGIN_MANAGER_H
#define PROTO_SHARED_PLUGIN_MANAGER_H

#include <istream>
#include <map>
#include <string>

#include "proto_plugin.h"

/// PluginTypeInventory is a map from plugin name -> containing library name
typedef std::map<std::string, std::string> PluginTypeInventory;

/// PluginInventory is a map from plugin type -> type inventory
typedef std::map<std::string, PluginTypeInventory> PluginInventory;

/// LibraryCollection is a map from open library name -> object
typedef std::map<std::string, ProtoPluginLibrary *> LibraryCollection;

class Args;
class Compiler;
class SpatialComputer;

class ProtoPluginManager {
 public:
  ProtoPluginManager();

  /**
   * Getters return null if they fail, and as a side effect print failure msgs
   * MUST CHECK FOR NULL ON RETURN
   * Used to get simulator Layers, Distributions, and TimeModels
   */
  void *get_sim_plugin(std::string type, std::string name, Args *args,
      SpatialComputer *cpu, int n);

  /// Used to get compiler extensions, or just the opcode part of a sim plugin
  void *get_compiler_plugin(std::string type, std::string name, Args *args,
      Compiler *c);

  /// Accessor for get_plugin_inventory
  const PluginInventory *get_plugin_inventory();

  /// Where DLLs go
  static const std::string PLUGIN_DIR;

  /// Where the registry goes
  static const std::string REGISTRY_FILE_NAME;

  /// Where .proto extension files go
  static const std::string PLATFORM_DIR;

  /// The default .proto file for a platform
  static const std::string PLATFORM_OPFILE;

  void register_lib(std::string type, std::string name, std::string key,
      ProtoPluginLibrary *lib);
  void ensure_initialized(Args *args);

 private:
  bool initialized;
  PluginInventory registry;
  LibraryCollection open_libs;
  ProtoPluginLibrary *get_plugin_lib(std::string type, std::string name,
      Args *args);
  bool read_dll(std::string libfile);
  bool read_registry_file(void);
  bool parse_registry(std::istream &reg, std::string overridename="");
};

/// global manager, initializes on first use
extern ProtoPluginManager plugins;

#endif  // PROTO_SHARED_PLUGIN_MANAGER_H
