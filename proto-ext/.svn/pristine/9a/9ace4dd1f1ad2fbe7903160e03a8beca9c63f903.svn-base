/* Collection of the default plugins built into the simulator 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "DefaultsPlugin.h"
#include "plugin_manager.h"

#define DLL_NAME "ProtoSimDefaults"
#define FIXTIME "FixedIntervalTime"
#define URANDOM "UniformRandom"
#define DEBUGLAYER "DebugLayer"
#define SIMPLEDYNAMICS "SimpleDynamics"
#define PERFECTLOCALIZER "PerfectLocalizer"
#define UNITDISCRADIO "UnitDiscRadio"

bool DefaultsPlugin::initialized = false;

void DefaultsPlugin::register_defaults() {
  if(initialized) return;
  ProtoPluginLibrary* lib = new DefaultsPlugin();
  plugins.register_lib(TIMEMODEL_PLUGIN,FIXTIME,DLL_NAME,lib);
  plugins.register_lib(DISTRIBUTION_PLUGIN,URANDOM,DLL_NAME,lib);
  plugins.register_lib(LAYER_PLUGIN,DEBUGLAYER,DLL_NAME,lib);
  plugins.register_lib(LAYER_PLUGIN,SIMPLEDYNAMICS,DLL_NAME,lib);
  plugins.register_lib(LAYER_PLUGIN,PERFECTLOCALIZER,DLL_NAME,lib);
  plugins.register_lib(LAYER_PLUGIN,UNITDISCRADIO,DLL_NAME,lib);
  initialized = true;
}

void* DefaultsPlugin::get_sim_plugin(string type, string name, Args* args, 
                                     SpatialComputer* cpu, int n) {
  if(type==TIMEMODEL_PLUGIN) {
    if(name==FIXTIME) { return new FixedIntervalTime(args, cpu); }
  } else if(type==DISTRIBUTION_PLUGIN) {
    if(name==URANDOM) { return new UniformRandom(n, cpu->volume); }
  } else if(type==LAYER_PLUGIN) {
    if(name==DEBUGLAYER) { return new DebugLayer(args,cpu); }
    if(name==SIMPLEDYNAMICS) { return new SimpleDynamics(args,cpu,n); }
    if(name==UNITDISCRADIO) { return new UnitDiscRadio(args,cpu,n); }
    if(name==PERFECTLOCALIZER) { return new PerfectLocalizer(cpu); }
  }
  return NULL;
}
