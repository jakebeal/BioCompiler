/* Plugin providing emulation of some common I/O devices on Mica2 Motes
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <sstream>
#include "config.h"
#include "Mica2MotePlugin.h"
#include "visualizer.h"
using namespace std;

#define SPEAK_OP "speak scalar scalar"
#define LIGHT_OP "light scalar"
#define SOUND_OP "sound scalar"
#define TEMP_OP "temp scalar"
#define CONDUCTIVE_OP "conductive scalar"
#define BUTTON_OP "button scalar scalar"
#define SLIDER_OP "slider scalar scalar scalar scalar scalar scalar scalar"

/*****************************************************************************
 *  TESTBED MOTE IO                                                          *
 *****************************************************************************/
MoteIO::MoteIO(Args* args, SpatialComputer* parent) : Layer(parent) {
  ensure_colors_registered("MoteIO");
  args->undefault(&can_dump,"-Dmoteio","-NDmoteio");
  // register patches
  parent->hardware.registerOpcode(new OpHandler<MoteIO>(this, &MoteIO::speak_op, SPEAK_OP));
  parent->hardware.registerOpcode(new OpHandler<MoteIO>(this, &MoteIO::light_op, LIGHT_OP));
  parent->hardware.registerOpcode(new OpHandler<MoteIO>(this, &MoteIO::sound_op, SOUND_OP));
  parent->hardware.registerOpcode(new OpHandler<MoteIO>(this, &MoteIO::temp_op, TEMP_OP));
  parent->hardware.registerOpcode(new OpHandler<MoteIO>(this, &MoteIO::conductive_op, CONDUCTIVE_OP));
  parent->hardware.registerOpcode(new OpHandler<MoteIO>(this, &MoteIO::button_op, BUTTON_OP));
  parent->hardware.registerOpcode(new OpHandler<MoteIO>(this, &MoteIO::slider_op, SLIDER_OP));
}

Color* MoteIO::BUTTON_COLOR;
void MoteIO::register_colors() {
#ifdef WANT_GLUT
  BUTTON_COLOR = palette->register_color("BUTTON_COLOR", 0, 1.0, 0.5, 0.8);
#endif
}

void MoteIO::speak_op(Machine* machine) {
  set_speak(machine->stack.peek().asNumber());
}

void MoteIO::light_op(Machine* machine) {
  machine->stack.push(read_light_sensor());
}

void MoteIO::sound_op(Machine* machine) {
  machine->stack.push(read_microphone());
}

void MoteIO::temp_op(Machine* machine) {
  machine->stack.push(read_temp());
}

void MoteIO::conductive_op(Machine* machine) {
  machine->stack.push(read_short());
}

void MoteIO::button_op(Machine* machine) {
  machine->stack.push(read_button((int) machine->stack.popNumber()));
}

void MoteIO::slider_op(Machine* machine) {
  Number max  = machine->stack.popNumber();
  Number min  = machine->stack.popNumber();
  Number incr = machine->stack.popNumber();
  Number init = machine->stack.popNumber();
  int     ikey = (int)machine->stack.popNumber();
  int     dkey = (int)machine->stack.popNumber();
  machine->stack.push(read_slider(dkey, ikey, init, incr, min, max));
}

void MoteIO::add_device(Device* d) {
  d->layers[id] = new DeviceMoteIO(this,d);
}
bool MoteIO::handle_key(KeyEvent* event) {
  return false; // right now, there's no keys that affect these globally
}

void MoteIO::dump_header(FILE* out) {
  if(can_dump) fprintf(out," \"SOUND\" \"TEMP\" \"BUTTON\"");
}

// hardware emulation
void MoteIO::set_speak (Number period) {
  // right now, setting the speaker does *nothing*, as in the old sim
}

extern Machine * machine;

Number MoteIO::read_light_sensor()
{ return ((DeviceMoteIO*)device->layers[id])->light; }
Number MoteIO::read_microphone ()
{ return ((DeviceMoteIO*)device->layers[id])->sound; }
Number MoteIO::read_temp ()
{ return ((DeviceMoteIO*)device->layers[id])->temperature; }
Number MoteIO::read_short () { return 0; }
Number MoteIO::read_button (uint8_t n) {
  return ((DeviceMoteIO*)device->layers[id])->button;
}
Number MoteIO::read_slider (uint8_t ikey, uint8_t dkey, Number init, 
			     Number incr, Number min, Number max) {
  // slider is not yet implemented
}

void DeviceMoteIO::dump_state(FILE* out, int verbosity) {
  Machine* m = container->vm;
  if(verbosity==0) { 
    fprintf(out," %.2f %.2f %d", sound, temperature, button);
  } else { 
    fprintf(out,"Mic = %.2f, Temp = %.2f, Button = %s\n",sound,
            temperature, bool2str(button));
  }
}

// individual device implementations
bool DeviceMoteIO::handle_key(KeyEvent* key) {
  // I think that the slider is supposed to consume keys too
  if(key->normal && !key->ctrl) {
    switch(key->key) {
    case 'N': button = !button; return true;
    }
  }
  return false;
}

#define SENSOR_RADIUS_FACTOR 4
void DeviceMoteIO::visualize(Device* d) {
#ifdef WANT_GLUT
  flo rad = d->body->display_radius();
  // draw the button being on
  if (button) {
    palette->use_color(MoteIO::BUTTON_COLOR);
    draw_disk(rad*SENSOR_RADIUS_FACTOR);
  }
#endif // WANT_GLUT
}



/*************** Plugin Library ***************/
void* Mica2MotePlugin::get_sim_plugin(string type, string name, Args* args, 
                                      SpatialComputer* cpu, int n) {
  if(type==LAYER_PLUGIN) {
    if(name==MICA2MOTE_NAME) { return new MoteIO(args,cpu); }
  }
  return NULL;
}

string Mica2MotePlugin::inventory() {
  return "# Some types of Mica2 Mote I/O\n" +
    registry_entry(LAYER_PLUGIN,MICA2MOTE_NAME,DLL_NAME);
}

extern "C" {
  ProtoPluginLibrary* get_proto_plugin_library()
  { return new Mica2MotePlugin(); }
  const char* get_proto_plugin_inventory()
  { return (new string(Mica2MotePlugin::inventory()))->c_str(); }
}
