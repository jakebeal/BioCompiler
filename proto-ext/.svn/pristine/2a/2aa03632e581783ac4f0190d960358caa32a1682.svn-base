/* Plugin providing emulation of some common I/O devices on Mica2 Motes
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef _MICA2MOTEPLUGIN_
#define	_MICA2MOTEPLUGIN_

#include "proto_plugin.h"
#include "spatialcomputer.h"

/*****************************************************************************
 *  TESTBED MOTE IO                                                          *
 *****************************************************************************/
class MoteIO : public Layer, public HardwarePatch {
 public:
  MoteIO(Args* args, SpatialComputer* parent);
  void add_device(Device* d);
  bool handle_key(KeyEvent* event);
  void dump_header(FILE* out); // list log-file fields

  static Color* BUTTON_COLOR;
  virtual void register_colors();
 private:
  void speak_op(Machine* machine);
  void light_op(Machine* machine);
  void sound_op(Machine* machine);
  void temp_op(Machine* machine);
  void conductive_op(Machine* machine);
  void button_op(Machine* machine);
  void slider_op(Machine* machine);

  // hardware emulation
  void set_speak (Number period);
  Number read_light_sensor();
  Number read_microphone ();
  Number read_temp ();
  Number read_short ();       // test for conductivity
  Number read_button (uint8_t n);
  Number read_slider (uint8_t ikey, uint8_t dkey, Number init, // frob knob
		       Number incr, Number min, Number max);
};

class DeviceMoteIO : public DeviceLayer {
  MoteIO* parent;
 public:
  Number light, sound, temperature;
  bool button;
  DeviceMoteIO(MoteIO* parent, Device* d) : DeviceLayer(d), light(0), sound(0), temperature(0)
    { this->parent=parent; button=false; }
  void visualize(Device* d);
  bool handle_key(KeyEvent* event);
  void copy_state(DeviceLayer* src) {} // to be called during cloning
  void dump_state(FILE* out, int verbosity); // print state to file
};

// Plugin class
#define MICA2MOTE_NAME "mote-io"
#define DLL_NAME "libmica2mote"
class Mica2MotePlugin : public ProtoPluginLibrary {
public:
  void* get_sim_plugin(string type, string name, Args* args, 
                       SpatialComputer* cpu, int n);
  static string inventory();
};

#endif	// _MICA2MOTEPLUGIN_
