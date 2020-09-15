/* A collection of simple common hardware packages for the simulator
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <machine.hpp>

#include "config.h"
#include "basic-hardware.h"
#include "visualizer.h"

extern Machine * machine;

/*****************************************************************************
 *  DEBUG                                                                    *
 *****************************************************************************/
DebugLayer::DebugLayer(Args* args, SpatialComputer* p) : Layer(p) {
  // pull display options
#ifdef WANT_GLUT
  palette = Palette::default_palette;
#endif
  ensure_colors_registered("DebugLayer");
  n_probes = args->extract_switch("-probes") ? (int)args->pop_number():0;
  is_show_leds = args->extract_switch("-l");
  is_led_rgb = args->extract_switch("-led-blend");
  is_led_ghost_mode = args->extract_switch("-led-ghost");
  is_led_3d_motion = !args->extract_switch("-led-flat");
  is_led_fixed_stacking = 
    (args->extract_switch("-led-stacking")) ? (int)args->pop_number():0;
  args->undefault(&can_dump,"-Ddebug","-NDdebug");
  dumpmask = args->extract_switch("-Ddebug-mask") ? (int)args->pop_number():-1;
  // register patches
  parent->hardware.patch(this,SET_PROBE_FN);
  parent->hardware.registerOpcode(new OpHandler<DebugLayer>(this, &DebugLayer::leds_op,  "leds scalar scalar"));
  parent->hardware.registerOpcode(new OpHandler<DebugLayer>(this, &DebugLayer::red_op,   "red scalar scalar"));
  parent->hardware.registerOpcode(new OpHandler<DebugLayer>(this, &DebugLayer::green_op, "green scalar scalar"));
  parent->hardware.registerOpcode(new OpHandler<DebugLayer>(this, &DebugLayer::blue_op,  "blue scalar scalar"));
  parent->hardware.registerOpcode(new OpHandler<DebugLayer>(this, &DebugLayer::rgb_op,   "rgb (vector 3) (vector 3)"));
  parent->hardware.registerOpcode(new OpHandler<DebugLayer>(this, &DebugLayer::hsv_op,   "hsv (vector 3) (vector 3)"));
  parent->hardware.registerOpcode(new OpHandler<DebugLayer>(this, &DebugLayer::sense_op, "sense scalar scalar"));
}

Color *DebugLayer::USER_SENSOR_1, *DebugLayer::USER_SENSOR_2, 
  *DebugLayer::USER_SENSOR_3, *DebugLayer::USER_SENSOR_4, *DebugLayer::RGB_LED, *DebugLayer::RED_LED,          
  *DebugLayer::GREEN_LED, *DebugLayer::BLUE_LED, *DebugLayer::DEVICE_PROBES;
void DebugLayer::register_colors() {
#ifdef WANT_GLUT
  USER_SENSOR_1 = palette->register_color("USER_SENSOR_1", 1.0, 0.5, 0, 0.8);
  USER_SENSOR_2 = palette->register_color("USER_SENSOR_2", 0.5, 0, 1.0, 0.8);
  USER_SENSOR_3 = palette->register_color("USER_SENSOR_3", 1.0, 0, 0.5, 0.8);
  USER_SENSOR_4 = palette->register_color("USER_SENSOR_4", 0, 1, 0.5, 0.8);                               
  RGB_LED = palette->register_color("RGB_LED", 1, 1, 1, 0.8);
  RED_LED = palette->register_color("RED_LED", 1, 0, 0, 0.8);
  GREEN_LED = palette->register_color("GREEN_LED", 0, 1, 0, 0.8);
  BLUE_LED = palette->register_color("BLUE_LED", 0, 0, 1, 0.8);
  DEVICE_PROBES = palette->register_color("DEVICE_PROBES", 0, 1, 0, 0.8);
#endif
}

void DebugLayer::leds_op(Machine* machine) {
  Number val = machine->stack.peek().asNumber();
  set_b_led((val > 0.25) != 0 ? 1.0 : 0);
  set_g_led((val > 0.50) != 0 ? 1.0 : 0);
  set_r_led((val > 0.75) != 0 ? 1.0 : 0);
}

void DebugLayer::red_op(Machine* machine) {
  set_r_led(machine->stack.peek().asNumber());;
}

void DebugLayer::green_op(Machine* machine) {
  set_g_led(machine->stack.peek().asNumber());;
}

void DebugLayer::blue_op(Machine* machine) {
  set_b_led(machine->stack.peek().asNumber());;
}

void DebugLayer::rgb_op(Machine* machine) {
  Tuple t = machine->stack.peek().asTuple();
  set_r_led(t[0].asNumber());
  set_g_led(t[1].asNumber());
  set_b_led(t[2].asNumber());
}

// CLIP forces the number x into the range [min,max]
#define CLIP(x, minimum, maximum) max((minimum), min((maximum), (x)))

// Convert hue/saturation/value to red/green/blue.  Output returned in args.
void my_hsv_to_rgb (flo h, flo s, flo v, flo *r, flo *g, flo *b) {
  flo rt, gt, bt;
  s = CLIP(s, static_cast<flo>(0), static_cast<flo>(1));
  if (s == 0.0) {
    rt = gt = bt = v;
  } else {
    int i;
    flo h_temp = (h == 360.0) ? 0.0 : h;
    flo f, p, q, t; 
    h_temp /= 60.0;
    i = (int)h_temp;
    f = h_temp - i;
    p = v*(1-s);
    q = v*(1-(s*f));
    t = v*(1-(s*(1-f)));
    switch (i) {
    case 0: rt = v; gt = t; bt = p; break;
    case 1: rt = q; gt = v; bt = p; break;
    case 2: rt = p; gt = v; bt = t; break;
    case 3: rt = p; gt = q; bt = v; break;
    case 4: rt = t; gt = p; bt = v; break;
    case 5: rt = v; gt = p; bt = q; break;
    }
  }
  //! Why are these commented out? --jsmb 5/12/06
  // rt = CLIP(rt, 0, 255);
  // gt = CLIP(gt, 0, 255);
  // bt = CLIP(bt, 0, 255);
  *r = rt*255; *g = gt*255; *b = bt*255;
}

void DebugLayer::hsv_op(Machine* machine) {
  machine->nextInt8();
  Tuple hsv = machine->stack.popTuple();
  flo r, g, b;
  my_hsv_to_rgb(hsv[0].asNumber(), hsv[1].asNumber(), hsv[2].asNumber(), &r, &g, &b);
  Tuple rgb(3);
  rgb.push(r);
  rgb.push(g);
  rgb.push(b);
  machine->stack.push(rgb);
}

void DebugLayer::sense_op(Machine* machine) {
  machine->stack.push(read_sensor((uint8_t) machine->stack.popNumber()));
}  

bool DebugLayer::handle_key(KeyEvent* key) {
  if(key->normal && !key->ctrl) {
    switch(key->key) {
    case 'p': n_probes = (n_probes + 1)%(MAX_PROBES+1); return true;
    case 'L': is_show_leds = !is_show_leds; return true;
    case '1': is_led_ghost_mode = !is_led_ghost_mode; return true;
    case '2': is_led_3d_motion = !is_led_3d_motion; return true;
    case '3': is_led_fixed_stacking = (is_led_fixed_stacking+1)%3; return true;
    case '4': is_led_rgb = !is_led_rgb; return true;
    }
  }
  return false;
}
void DebugLayer::add_device(Device* d) {
  d->layers[id] = new DebugDevice(this,d);
}

void DebugLayer::dump_header(FILE* out) {
  if(can_dump) {
    // there was another sensor here, but it's been removed
    if(dumpmask & 0x02) fprintf(out," \"USER1\"");
    if(dumpmask & 0x04) fprintf(out," \"USER2\"");
    if(dumpmask & 0x08) fprintf(out," \"USER3\"");                          
    if(dumpmask & 0x10) fprintf(out," \"USER4\"");
    if(dumpmask & 0x20) fprintf(out," \"R_LED\"");
    if(dumpmask & 0x40) fprintf(out," \"G_LED\"");
    if(dumpmask & 0x80) fprintf(out," \"B_LED\"");
  }
}

// actuators
void DebugLayer::set_probe (Data val, uint8_t index) { 
  if(index >= MAX_PROBES) return; // sanity check index
   ((DebugDevice*)device->layers[id])->probes[index] = val;
}
void DebugLayer::set_r_led (Number val) 
{ ((DebugDevice*)device->layers[id])->actuators[R_LED] = val; }
void DebugLayer::set_g_led (Number val)
{ ((DebugDevice*)device->layers[id])->actuators[G_LED] = val; }
void DebugLayer::set_b_led (Number val)
{ ((DebugDevice*)device->layers[id])->actuators[B_LED] = val; }
Number DebugLayer::read_sensor (uint8_t n)
{ return (n<N_SENSORS) ? ((DebugDevice*)device->layers[id])->sensors[USER_A+n-1] : NAN;}

// per-device interface, used primarily for visualization
DebugDevice::DebugDevice(DebugLayer* parent, Device* d) : DeviceLayer(d) {
  for(int i=0;i<MAX_PROBES;i++) probes[i] = 0; // probes start clear
  for(int i=0;i<N_ACTUATORS;i++) actuators[i] = 0; // actuators start clear
  for(int i=0;i<N_SENSORS;i++) sensors[i] = 0; // sensors start clear
  this->parent = parent;
}

void DebugDevice::dump_state(FILE* out, int verbosity) {
  Machine* m = container->vm;
  if(verbosity==0) {
    uint32_t dumpmask = parent->dumpmask; // shorten the name
    // there was another sensor here, but it's been removed
    if(dumpmask & 0x02) fprintf(out," %.2f",sensors[USER_A]);
    if(dumpmask & 0x04) fprintf(out," %.2f",sensors[USER_B]);
    if(dumpmask & 0x08) fprintf(out," %.2f",sensors[USER_C]);
    if(dumpmask & 0x10) fprintf(out," %.2f",sensors[USER_D]);
    if(dumpmask & 0x20) fprintf(out," %.3f",actuators[R_LED]);
    if(dumpmask & 0x40) fprintf(out," %.3f",actuators[G_LED]);
    if(dumpmask & 0x80) fprintf(out," %.3f",actuators[B_LED]);
    // probes can't be output gracefully since we don't know what they contain
  } else {
    fprintf(out,"Sensors: User-1=%.2f User-2=%.2f User-3=%.2f User-4=%.2f\n",
            sensors[USER_A], sensors[USER_B], sensors[USER_C], sensors[USER_D]);
    fprintf(out,"LEDs: R=%.3f G=%.3f B=%.3f\n", actuators[R_LED],
            actuators[G_LED], actuators[B_LED]);
    fprintf(out,"Probes:");
    char buf[1000];
    for(int i=0;i<MAX_PROBES;i++) {
      post_data_to(buf,probes[i]); fprintf(out,"%s ",buf);
    }
    fprintf(out,"\n");
  }
}

bool DebugDevice::handle_key(KeyEvent* key) {
  if(key->normal && !key->ctrl) {
    switch(key->key) {
    case 't': 
      sensors[USER_A] = sensors[USER_A] ? 0:1; return true;
    case 'y': 
      sensors[USER_B] = sensors[USER_B] ? 0:1; return true;
    case 'u': 
      sensors[USER_C] = sensors[USER_C] ? 0:1; return true;
    case 'o':
      sensors[USER_D] = sensors[USER_D] ? 0:1; return true;
    }
  }
  return false;
}

void DebugDevice::preupdate() {
  for(int i=0;i<MAX_PROBES;i++) { probes[i] = 0; }
  for(int i=0;i<N_ACTUATORS;i++) { actuators[i] = 0; }
}

#define SENSOR_RADIUS_FACTOR 4
#define N_USER_SENSORS 4
void DebugDevice::visualize() {
#ifdef WANT_GLUT
  static Color* user[N_USER_SENSORS] = {DebugLayer::USER_SENSOR_1, DebugLayer::USER_SENSOR_2,
                           DebugLayer::USER_SENSOR_3, DebugLayer::USER_SENSOR_4}; 
  flo rad = container->body->display_radius();
  // draw user sensors
  for(int i=0;i<N_USER_SENSORS;i++) {
    if(sensors[USER_A+i] > 0) { 
      palette->use_color(user[i]);
      draw_disk(rad*SENSOR_RADIUS_FACTOR);
    }
  }
  // draw LEDs
  if (parent->is_show_leds) {
    static Color* led_color[3] = 
      {DebugLayer::RED_LED, DebugLayer::GREEN_LED, DebugLayer::BLUE_LED};
    flo led[3] = { actuators[R_LED], actuators[G_LED],
		   actuators[B_LED] };
    glPushMatrix();
    if (parent->is_led_rgb) {
      if (led[0] || led[1] || led[2]) {
	palette->scale_color(DebugLayer::RGB_LED, led[0],led[1],led[2],1);
        if(parent->is_led_3d_motion) { glPushMatrix(); glTranslatef(0,0,1); }
        draw_disk(rad*2); // double size because the legacy code sez so
        if(parent->is_led_3d_motion) { glPopMatrix(); }
      }
    } else {
      for(int i=0;i<3;i++) {
	if(led[i]==0) continue;
        if(parent->is_led_fixed_stacking) { 
	  glPushMatrix();
	  if(parent->is_led_fixed_stacking==1) glTranslatef(0,0,i); 
	}
        if(parent->is_led_ghost_mode)
	  palette->scale_color(led_color[i],1,1,1,led[i]);
	else
	  palette->scale_color(led_color[i],led[i],led[i],led[i],1);
        if(parent->is_led_3d_motion) glTranslatef(0,0,led[i]);
        draw_disk(rad); // actually draw the damned thing
        if(parent->is_led_fixed_stacking) { glPopMatrix(); }
      }
    }
    glPopMatrix();
  }
  // draw probes
  if (parent->n_probes > 0) {
    glPushMatrix();
    container->text_scale(); // prepare to draw text
    char buf[1024];
    glTranslatef(0, 0.5625, 0);
    for (int i = 0; i < parent->n_probes; i++) {
      post_data_to(buf, probes[i]);
      palette->use_color(DebugLayer::DEVICE_PROBES);
      draw_text(1, 1, buf);
      glTranslatef(1.125, 0, 0);
    }
    glPopMatrix();
  }
#endif // WANT_GLUT
}



/*****************************************************************************
 *  PERFECT LOCALIZER                                                        *
 *****************************************************************************/
PerfectLocalizer::PerfectLocalizer(SpatialComputer* parent) : Layer(parent) {
  parent->hardware.registerOpcode(new OpHandler<PerfectLocalizer>(this, &PerfectLocalizer::coord_op, "coord (vector 3)"));
  parent->hardware.patch(this,READ_SPEED_FN);
}

void PerfectLocalizer::coord_op(Machine* machine) {
  machine->stack.push(read_coord_sensor());
}

vector<HardwareFunction> PerfectLocalizer::getImplementedHardwareFunctions()
{
    vector<HardwareFunction> hardwareFunctions;
    hardwareFunctions.push_back(READ_SPEED_FN);
    return hardwareFunctions;
}

void PerfectLocalizer::add_device(Device* d) {
  d->layers[id] = new PerfectLocalizerDevice(d);
}

Tuple PerfectLocalizer::read_coord_sensor() {
  PerfectLocalizerDevice* d = (PerfectLocalizerDevice*)device->layers[id];
  if(!d->coord_sense.isSet()) {
    Tuple c(3);
    c.push(0);
    c.push(0);
    c.push(0);
    d->coord_sense = c;
  }
  const METERS* p = device->body->position();
  for(int i=0;i<3;i++) d->coord_sense.asTuple()[i] = p[i];
  return d->coord_sense.asTuple();
}

Number PerfectLocalizer::read_speed() {
  const METERS* v = device->body->velocity();
  return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}


/*****************************************************************************
 *  LEFTOVER LAYER                                                           *
 *****************************************************************************/
// Default used to patch the otherwise unpatched but required functions
// Currently unused.

LeftoverLayer::LeftoverLayer(SpatialComputer* parent) : Layer(parent) {
/*  parent->hardware.registerOpcode(new OpHandler<LeftoverLayer>(this, &LeftoverLayer::ranger_op, "ranger (vector 3)"));
  parent->hardware.registerOpcode(new OpHandler<LeftoverLayer>(this, &LeftoverLayer::mouse_op, "mouse (vector 3)"));
  parent->hardware.registerOpcode(new OpHandler<LeftoverLayer>(this, &LeftoverLayer::local_fold_op, "local-fold scalar scalar scalar"));
  parent->hardware.registerOpcode(new OpHandler<LeftoverLayer>(this, &LeftoverLayer::fold_complete_op, "fold-complete scalar boolean"));
  parent->hardware.registerOpcode(new OpHandler<LeftoverLayer>(this, &LeftoverLayer::channel_op, "new-channel scalar scalar"));
  parent->hardware.registerOpcode(new OpHandler<LeftoverLayer>(this, &LeftoverLayer::drip_op, "drip scalar scalar scalar"));
  parent->hardware.registerOpcode(new OpHandler<LeftoverLayer>(this, &LeftoverLayer::concentration_op, "concentration scalar scalar"));
  parent->hardware.registerOpcode(new OpHandler<LeftoverLayer>(this, &LeftoverLayer::channel_grad_op, "channel-grad (vector 3) scalar"));
  parent->hardware.registerOpcode(new OpHandler<LeftoverLayer>(this, &LeftoverLayer::cam_op, "cam scalar scalar"));*/
}
/*
void LeftoverLayer::ranger_op(Machine* machine) {
  machine->stack.push(read_ranger());
}

void LeftoverLayer::mouse_op(Machine* machine) {
  machine->stack.push(read_mouse_sensor());
}

void LeftoverLayer::local_fold_op(MACHINE* machine) {
  int k = (int)NUM_PEEK(0);
  int val = (bool)NUM_PEEK(1);
  set_is_folding((int) NUM_PEEK(1), k);
  NPOP(2);
  NUM_PUSH(val);
}

void LeftoverLayer::fold_complete_op(MACHINE* machine) {
  int k = (int) NUM_PEEK(0);
  NPOP(1);
  NUM_PUSH(read_fold_complete(k));
}

void LeftoverLayer::channel_op(MACHINE* machine) {
  int n = NXT_OP(machine);
  set_channel((int)NUM_POP(), n);
  NUM_PUSH(n);
}

void LeftoverLayer::drip_op(MACHINE* machine) {
  Number a = NUM_PEEK(1);
  Number c = NUM_PEEK(0);
  NPOP(2);
  NUM_PUSH(drip_channel((int)a, (int) c));
}

void LeftoverLayer::concentration_op(MACHINE* machine) {
  NUM_PUSH(read_channel((int) NUM_POP()));
}

void LeftoverLayer::channel_grad_op(MACHINE* machine) {
  VEC_PUSH(grad_channel((int) NUM_POP()));
}

void LeftoverLayer::cam_op(MACHINE* machine) {
  NUM_PUSH(cam_get((int) NUM_POP()));
}

Tuple LeftoverLayer::read_ranger() {
  hardware_error("read_ranger");
}

Tuple LeftoverLayer::read_mouse_sensor() {
  hardware_error("read_mouse_sensor");
}

bool LeftoverLayer::set_is_folding(int n, int k) {
  hardware_error("set_is_folding");
}

bool LeftoverLayer::read_fold_complete(int n) {
  hardware_error("read_fold_complete");
}

Number LeftoverLayer::set_channel(int n, int k) {
  hardware_error("set_channel");
}

Number LeftoverLayer::drip_channel(int n, int k) {
  hardware_error("drip_channel");
}

Number LeftoverLayer::read_channel(int n) {
  hardware_error("drip_channel");
}

Tuple LeftoverLayer::grad_channel(int n) {
  hardware_error("drip_channel");
}

Number LeftoverLayer::cam_get(int n) {
  hardware_error("drip_channel");
}*/
