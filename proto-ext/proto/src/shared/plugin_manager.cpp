/* Mechanisms for handling plugin registry and loading
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <ltdl.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "config.h"

#include "plugin_manager.h"
#include "utils.h"

using namespace std;

// Registry location:
const string ProtoPluginManager::PLUGIN_DIR = string(PLUGINDIR);
const string ProtoPluginManager::REGISTRY_FILE_NAME = "registry.txt";
const string ProtoPluginManager::PLATFORM_DIR = string(PROTOPLATDIR);
const string ProtoPluginManager::PLATFORM_OPFILE = "platform_ops.proto";

ProtoPluginManager plugins; // global manager, initializes on first use

ProtoPluginManager::ProtoPluginManager() {
  initialized = false;
}

const PluginInventory *
ProtoPluginManager::get_plugin_inventory()
{
  ensure_initialized(NULL);
  return &registry;
}

void
ProtoPluginManager::register_lib(string type, string name, string key,
    ProtoPluginLibrary *lib)
{
  registry[type][name] = key;
  open_libs[key] = lib;
}

void
ProtoPluginManager::ensure_initialized(Args *args)
{
  if (initialized >= 2)
    return;                     // Idempotent.

  if (initialized == 0) {
    if (!read_registry_file())
      cerr << "WARNING: Only default plugins will be loaded.\n";
    lt_dlinit();                // Begin using libltdl library tool.
    initialized = 1;            // Mark initialization complete.
  }

  if (args != NULL) {
    args->save_ptr();
    // Check command line to see if any other DLLs are being included locally.
    while (args->extract_switch("--DLL", false)) {
      const string &dll_name = args->pop_next();
      if (!read_dll(dll_name))
        { cerr << "Unable to include DLL " << dll_name << endl; }
    }
    initialized = 2;            // Mark command line processing complete.
    args->restore_ptr();
  }
}

void
split(const string &s, const string &token, vector<string> &segments)
{
  size_t i = 0;
  size_t j = 0;
  while (string::npos != (j = s.find(token, i))) {
    segments.push_back(s.substr(i, j - i));
    i = j + token.length();
  }
  segments.push_back(s.substr(i, s.length()));
}

bool
ProtoPluginManager::parse_registry(istream &reg, string overridename)
{
  bool parseok = true;
  while (!reg.eof()) {
    string line; getline(reg, line, '\n');
    // Ignore comment lines and empty lines.
    if (line.empty() || '#' == line.at(0)) continue;
    // If the line is precisely equal to the desired structure, slurp it.
    vector<string> segments; split(line, " ", segments);
    if (segments.size() == 4 && segments[2] == "=") {
      string libname = (overridename == "") ? segments[3] : overridename;
      registry[segments[0]][segments[1]] = libname;
    } else {
      cerr << "Unable to interpret registry line '" << line << "'" << endl;
      parseok = false;
    }
  }
  return parseok;
}

bool
ProtoPluginManager::read_dll(string libfile)
{
  // Open the library.
  lt_dlhandle handle = lt_dlopenext(libfile.c_str());
  if (handle == NULL) {
    cerr << "Could not load plugin library " + libfile + "\n";
    return false;
  }

  // Parse the inventory.
  void *fp = lt_dlsym(handle, "get_proto_plugin_inventory");
  if (fp == NULL) {
    cerr << libfile << "not a Proto plugin (no inventory)\n";
    return false;
  }

  string reg((*((get_inventory_func)fp))());
  istringstream iss(reg);
  parse_registry(iss, libfile);

  // Register the library.
  fp = lt_dlsym(handle, "get_proto_plugin_library");
  if (fp == NULL) {
    cerr << "Could not get get_proto_plugin_library from " + libfile + "\n";
    return false;
  }

  // Run the entry point.
  ProtoPluginLibrary *lib = (*((get_library_func)fp))();
  open_libs[libfile] = lib;

  return true;
}

bool
ProtoPluginManager::read_registry_file(void)
{
  string registryFile = PLUGIN_DIR + REGISTRY_FILE_NAME;
  ifstream fin(registryFile.c_str());
  if(!fin.good()) {
    cerr << "Unable to open registry file: " << registryFile << endl;
    return false;
  }
  if(!parse_registry(fin))
    return false;
  return true;
}

ProtoPluginLibrary *
ProtoPluginManager::get_plugin_lib(string type, string name, Args *args)
{
  ensure_initialized(args);

  if (!registry[type].count(name)) {
    cerr << "No registered library contains " + type + " " + name + "\n";
    return NULL;
  }

  string libfile = registry[type][name];
  ProtoPluginLibrary *lib;
  if (open_libs.count(libfile)) {
    // Use the library...
    lib = open_libs[libfile];
  } else {
    // ...or load it in if it's not yet loaded.
    string fullname = PLUGIN_DIR + libfile;
    lt_dlhandle handle = lt_dlopenext(fullname.c_str());
    if (handle == NULL) {
      cerr << "Could not load plugin library " + fullname + "\n";
      // Enable (and add right extension) if we need better debug info
      // dlopen(fullname.c_str(), RTLD_LAZY); cerr << dlerror() << endl;
      return NULL;
    }
    void *fp = lt_dlsym(handle, "get_proto_plugin_library");
    if (fp == NULL) {
      cerr << "Could not get get_proto_plugin_library from " + libfile;
      return NULL;
    }
    // Run the entry point.
    lib = (*((get_library_func)fp))();
    open_libs[libfile] = lib;
  }
  return lib;
}

void *
ProtoPluginManager::get_sim_plugin(string type, string name, Args *args,
    SpatialComputer *cpu,int n)
{
  ProtoPluginLibrary *lib = get_plugin_lib(type,name,args);
  if (lib == NULL) return NULL;
  return lib->get_sim_plugin(type, name, args, cpu, n);
}

void *
ProtoPluginManager::get_compiler_plugin(string type, string name, Args *args,
    Compiler *c)
{
  ProtoPluginLibrary *lib = get_plugin_lib(type,name,args);
  if (lib == NULL) return NULL;
  return lib->get_compiler_plugin(type, name, args, c);
}
