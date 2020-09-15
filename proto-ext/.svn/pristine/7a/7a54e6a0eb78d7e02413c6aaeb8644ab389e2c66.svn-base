/* Plugin providing simple cloning and apoptosis model
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "SimpleLifeCyclePlugin.h"

#define DIE_OP "die scalar boolean"
#define CLONE_OP "clone scalar boolean"

/*****************************************************************************
 *  SIMPLE LIFECYCLE                                                         *
 *****************************************************************************/
// instant death, cloning after a delay
SimpleLifeCycle::SimpleLifeCycle(Args* args, SpatialComputer* p) : Layer(p) {
  this->parent=parent;
  clone_delay = args->extract_switch("-clone-delay")?(int)args->pop_number():1;
  args->undefault(&can_dump,"-Dclone","-NDclone");
  // register hardware functions
  parent->hardware.registerOpcode(new OpHandler<SimpleLifeCycle>(this, &SimpleLifeCycle::die_op, DIE_OP));
  parent->hardware.registerOpcode(new OpHandler<SimpleLifeCycle>(this, &SimpleLifeCycle::clone_op, CLONE_OP));
}

void SimpleLifeCycle::die_op(Machine* machine) {
  die(machine->stack.peek().asNumber());
}

void SimpleLifeCycle::clone_op(Machine* machine) {
  clone_machine(machine->stack.peek().asNumber());
}

void SimpleLifeCycle::add_device(Device* d) {
  d->layers[id] = new SimpleLifeCycleDevice(this,d);
}

void SimpleLifeCycle::die (Number val) {
  if(val != 0) parent->death_q.push(device->backptr);
}
void SimpleLifeCycle::clone_machine (Number val) {
  if(val != 0) ((SimpleLifeCycleDevice*)device->layers[id])->clone_cmd=true;
}

void SimpleLifeCycle::dump_header(FILE* out) {
  if(can_dump) fprintf(out," \"CLONETIME\"");
}



SimpleLifeCycleDevice::SimpleLifeCycleDevice(SimpleLifeCycle* parent,
                                             Device* d) : DeviceLayer(d) {
  this->parent=parent;
  clone_time=-1; clone_cmd=false; // start unready to clone
}

void SimpleLifeCycleDevice::dump_state(FILE* out, int verbosity) {
  if(verbosity==0) { fprintf(out," %.2f", clone_time);
  } else { fprintf(out,"Time to clone %.2f\n",clone_time);
  }
}

// choose new position w. random polar coordinates
void SimpleLifeCycleDevice::clone_me() {
  const flo* p = container->body->position();
  flo dp = 2*container->body->display_radius();
  flo theta = urnd(0,2*M_PI);
  flo phi = (parent->parent->is_3d() ? urnd(0,2*M_PI) : 0);
  METERS cp[3];
  cp[0] = p[0] + dp*cos(phi)*cos(theta);
  cp[1] = p[1] + dp*cos(phi)*sin(theta);
  cp[2] = p[2] + dp*sin(phi);
  CloneReq* cr = new CloneReq(container->backptr,container,cp);
  parent->parent->clone_q.push(cr);
}

extern Machine * machine;

void SimpleLifeCycleDevice::update() {
  if(clone_cmd) {
    if(clone_time<0) { 
      clone_time = (machine->startTime() + parent->clone_delay); 
    }
    if(machine->startTime() >= clone_time) {
      clone_cmd=false; // reset clone_cmd
      clone_time=-1;   // reset clone_time
      clone_me();
    }
  }
}

bool SimpleLifeCycleDevice::handle_key(KeyEvent* key) {
  // is this a key recognized internally?
  if(key->normal && !key->ctrl) {
    switch(key->key) {
    case 'B': clone_me(); return true;
    case 'K': parent->parent->death_q.push(container->backptr); return true;
    }
  }
  return false;
}


/*************** Plugin Library ***************/
void* SimpleLifeCyclePlugin::get_sim_plugin(string type,string name,Args* args, 
                                            SpatialComputer* cpu, int n) {
  if(type == LAYER_PLUGIN) {
    if(name == LAYER_NAME) { return new SimpleLifeCycle(args, cpu); }
  }
  return NULL;
}

string SimpleLifeCyclePlugin::inventory() {
  return "# Cloning and apoptosis\n" +
    registry_entry(LAYER_PLUGIN,LAYER_NAME,DLL_NAME);
}

extern "C" {
  ProtoPluginLibrary* get_proto_plugin_library() 
  { return new SimpleLifeCyclePlugin(); }
  const char* get_proto_plugin_inventory()
  { return (new string(SimpleLifeCyclePlugin::inventory()))->c_str(); }
}
