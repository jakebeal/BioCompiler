/* Plugin providing simple cloning and apoptosis model
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef _SIMPLELIFECYCLEPLUGIN_
#define	_SIMPLELIFECYCLEPLUGIN_

#include "proto_plugin.h"
#include "spatialcomputer.h"
#define LAYER_NAME "simple-life-cycle"
#define DLL_NAME "libsimplelifecycle"

/*************** Layer ***************/
class SimpleLifeCycle : public Layer, public HardwarePatch {
 public:
  flo clone_delay;               // minimum time between clonings

  SimpleLifeCycle(Args* args, SpatialComputer* parent);
  void add_device(Device* d);
  // hardware patch functions
  void dump_header(FILE* out); // list log-file fields
 private:
  void die_op(Machine* machine);
  void clone_op(Machine* machine);
  void die (Number val);
  void clone_machine (Number val);
};

class SimpleLifeCycleDevice : public DeviceLayer {
 public:
  SimpleLifeCycle* parent;
  bool clone_cmd;            // request for cloning is active
  flo clone_time;           // time (including delay) to trigger clone
  SimpleLifeCycleDevice(SimpleLifeCycle* parent, Device* container);
  void update();
  bool handle_key(KeyEvent* event);
  void clone_me();
  void copy_state(DeviceLayer* src) {} // to be called during cloning
  void dump_state(FILE* out, int verbosity); // print state to file
};

/*************** Plugin Interface ***************/
class SimpleLifeCyclePlugin : public ProtoPluginLibrary {
public:
  void* get_sim_plugin(string type, string name, Args* args, 
                       SpatialComputer* cpu, int n);
  static string inventory();
};

#endif	// _SIMPLELIFECYCLEPLUGIN_
