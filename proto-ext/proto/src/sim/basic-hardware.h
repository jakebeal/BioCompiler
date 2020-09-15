/* A collection of simple common hardware packages for the simulator
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __BASIC_HARDWARE__
#define __BASIC_HARDWARE__

#include "spatialcomputer.h"
#include <vector>
using namespace std;

/*****************************************************************************
 *  DEBUG                                                                    *
 *****************************************************************************/
// probes, LEDs
class DebugLayer : public Layer, public HardwarePatch {
 public:
  // display parameters
  int n_probes;               // how many debug probes to show?
  bool is_show_leds;           // should LEDs be drawn?
  bool is_led_rgb;            // true: 1 LED of mixed color; false: 3 LEDs
  bool is_led_ghost_mode;     // if rgb=false, LED value sets alpha channel
  bool is_led_3d_motion;      // if rgb=false, LED value sets Z displacement
  // is_led_fixed_stacking applies only when rgb=false:
  // 0: LED Z displacement is relative to prev LED
  // 1: LED Z displacement starts at 0,1,2;  2: LED Z displacement starts at 0
  int is_led_fixed_stacking;
  uint32_t dumpmask;

public:
  DebugLayer(Args* args, SpatialComputer* parent);
  void add_device(Device* d);
  bool handle_key(KeyEvent* event);
  // hardware emulation
  void set_probe (Data val, uint8_t index); // debugging data probe
  void dump_header(FILE* out); // list log-file fields
 private:
  void leds_op(Machine* machine);
  void red_op(Machine* machine);
  void green_op(Machine* machine);
  void blue_op(Machine* machine);
  void rgb_op(Machine* machine);
  void hsv_op(Machine* machine);
  void sense_op(Machine* machine);
  void set_r_led (Number val);
  void set_g_led (Number val);
  void set_b_led (Number val);
  Number read_sensor (uint8_t n); // for "user" sensors                                       

 public:
  void register_colors();
  static Color *USER_SENSOR_1, *USER_SENSOR_2, *USER_SENSOR_3, *USER_SENSOR_4, *RGB_LED,
    *RED_LED, *GREEN_LED, *BLUE_LED, *DEVICE_PROBES;
};

#define MAX_PROBES 3

enum Actuator {
	R_LED,
	G_LED,
	B_LED,
	N_ACTUATORS
};

enum Sensor {
	USER_A,
	USER_B,
        USER_C,
        USER_D,
	N_SENSORS
};

class DebugDevice : public DeviceLayer {
  DebugLayer* parent;
public:
  Data probes[MAX_PROBES];          // debugging probes
  Number actuators[N_ACTUATORS];
  Number sensors[N_SENSORS];
  
  DebugDevice(DebugLayer* parent, Device* container);
  void preupdate();
  void visualize();
  bool handle_key(KeyEvent* event);
  void copy_state(DeviceLayer* src) {} // to be called during cloning
  void dump_state(FILE* out, int verbosity); // print state to file
};

/*****************************************************************************
 *  PERFECT LOCALIZER                                                        *
 *****************************************************************************/
class PerfectLocalizer : public Layer, public HardwarePatch {
    
 public:
  PerfectLocalizer(SpatialComputer* parent);
  void add_device(Device* d);
  Number read_speed ();

  // returns a list of function  that it patches/ provides impementation for
  static vector<HardwareFunction> getImplementedHardwareFunctions();
 private:
  void coord_op(Machine* machine);
  Tuple read_coord_sensor();
};
class PerfectLocalizerDevice : public DeviceLayer {
 public:
  Data coord_sense; // data location for kernel to access coordinates
  PerfectLocalizerDevice(Device* container) : DeviceLayer(container) { }
  void copy_state(DeviceLayer*) {} // no state worth copying
};

class LeftoverLayer : public Layer {
 public:
  LeftoverLayer(SpatialComputer* parent);
 private:
 /*
  void ranger_op(Machine* machine);
  void mouse_op(Machine* machine);
  void local_fold_op(Machine* machine);
  void fold_complete_op(Machine* machine);
  void channel_op(Machine* machine);
  void drip_op(Machine* machine);
  void concentration_op(Machine* machine);
  void channel_grad_op(Machine* machine);
  void cam_op(Machine* machine);

  Tuple read_ranger();
  Tuple read_mouse_sensor();
  bool set_is_folding(int n, int k);
  bool read_fold_complete(int n);
  Number set_channel(int n, int k);
  Number drip_channel(int n, int k);
  Number read_channel(int n);
  Tuple grad_channel(int n);
  Number cam_get(int n);

  void hardware_error(const char* name) {
    uerror("Attempt to use unimplemented hardware function '%s'",name);
  }*/
};

#endif // __BASIC_HARDWARE__


/* STILL UNHANDLED HARDWARE
  virtual Number cam_get (int k) { hardware_error("cam_get"); }
  
  virtual void set_is_folding (bool val, int k) 
  { hardware_error("set_is_folding"); }
  virtual bool read_fold_complete (int val) 
  { hardware_error("read_fold_complete"); }
  
  virtual Number set_channel (Number diffusion, int k) 
  { hardware_error("set_channel"); }
  virtual Number read_channel (int k) { hardware_error("read_channel"); }
  virtual Number drip_channel (Number val, int k) 
  { hardware_error("drip_channel"); }
  virtual Tuple grad_channel (int k) { hardware_error("grad_channel"); }
*/
