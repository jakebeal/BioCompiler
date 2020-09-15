/* Plugin providing wormhole radios and radio model superposition
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "RadioModelsPlugin.h"
#include "multiradio.h"
#include "wormhole-radio.h"

void* RadioModelsPlugin::get_sim_plugin(string type,string name,Args* args, 
                                        SpatialComputer* cpu, int n) {
  if(type == LAYER_PLUGIN) {
    if(name == WORM_HOLES_NAME) { return new WormHoleRadio(args, cpu, n); }
    if(name == MULTI_RADIO_NAME) { return new MultiRadio(args, cpu, n); }
  }
  return NULL;
}

string RadioModelsPlugin::inventory() {
  return "# More complex radio models\n" +
    registry_entry(LAYER_PLUGIN,WORM_HOLES_NAME,DLL_NAME) +
    registry_entry(LAYER_PLUGIN,MULTI_RADIO_NAME,DLL_NAME);
}

extern "C" {
  ProtoPluginLibrary* get_proto_plugin_library()
  { return new RadioModelsPlugin(); }
  const char* get_proto_plugin_inventory()
  { return (new string(RadioModelsPlugin::inventory()))->c_str(); }
}
