/* Plugin containing default Distribution, TimeModel, and Layers
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef DEFAULTSPLUGIN_H_
#define DEFAULTSPLUGIN_H_

#include <string>
#include "proto_plugin.h"
#include "spatialcomputer.h"
#include "sim-hardware.h"
#include "basic-hardware.h"
#include "simpledynamics.h"
#include "unitdiscradio.h"
#include "UniformRandom.h"
#include "FixedIntervalTime.h"

class DefaultsPlugin : public ProtoPluginLibrary {
 public:
  static void register_defaults();

  void* get_sim_plugin(string type, string name, Args* args,
                       SpatialComputer* cpu, int n);

 private:
  static bool initialized;
};

#endif /* DEFAULTSPLUGIN_H_ */
