/* Plugin providing wormhole radios and radio model superposition
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef _RADIOMODELSPLUGIN_
#define	_RADIOMODELSPLUGIN_

#include "proto_plugin.h"

#define WORM_HOLES_NAME "wormholes"
#define MULTI_RADIO_NAME "multiradio"
#define DLL_NAME "libradiomodels"

// Plugin class
class RadioModelsPlugin : public ProtoPluginLibrary {
public:
  void* get_sim_plugin(string type, string name, Args* args, 
                       SpatialComputer* cpu, int n);
  static string inventory();
};

#endif	// _RADIOMODELSPLUGIN_
