/* Programmed simulation termination

Copyright (C) 2009, Nelson Elhage and contributors listed in the
AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory.  */

#include "config.h"
#include "stop-when.h"
#include "visualizer.h"

StopWhen::StopWhen(Args *args, SpatialComputer *parent) : Layer(parent) {
  stop_pct = (args->extract_switch("-stop-pct"))?args->pop_number():1.0;
  this->n_devices = 0;

  parent->hardware.registerOpcode(new OpHandler<StopWhen>(this, &StopWhen::stop_op,  "stop boolean boolean"));
}

StopWhen::~StopWhen() {
  
}

void StopWhen::add_device(Device *d) {
  n_devices++;
}

void StopWhen::stop_op(Machine* machine) {
  Number val = machine->stack.peek().asNumber();
  if(val!=0) {
    probed.insert(device);
    if(probed.size() > n_devices * stop_pct
       || (stop_pct == 1.0 && probed.size() == n_devices)) {
      post("Stopping simulation at t=%f\n", parent->sim_time);
      exit(0);
    }
  }
}



/*************** Plugin Library ***************/
#define DLL_NAME "libstopwhen"
#define LAYER_NAME "stop-when"

void* StopWhenPlugin::get_sim_plugin(string type, string name, Args* args,
                                          SpatialComputer* cpu, int n) {
  if(type==LAYER_PLUGIN) {
    if(name==LAYER_NAME) { // plain grid    
      return new StopWhen(args,cpu);
    }
  }
  return NULL;
}

string StopWhenPlugin::inventory() {
  return registry_entry(LAYER_PLUGIN,LAYER_NAME,DLL_NAME);
}

extern "C" {
  ProtoPluginLibrary* get_proto_plugin_library()
  { return new StopWhenPlugin(); }
  const char* get_proto_plugin_inventory()
  { return (new string(StopWhenPlugin::inventory()))->c_str(); }
}

