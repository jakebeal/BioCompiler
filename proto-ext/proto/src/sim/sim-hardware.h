/* Code to manage the interface between the simulator and the kernel.
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __SIM_HARDWARE__
#define __SIM_HARDWARE__

#include <vector>
#include <string>
#include <stdint.h>
#include "machine.hpp"

using namespace std;
// posts data without pretty nesting
void post_data_to(char * str, Data data);
void post_stripped_data_to(char * str, Data data);
void post_data(Data data);

// HardwarePatch is a class that Layers should extend in order to make
// their dynamics available to the kernel.
class HardwarePatch {
public:
  void hardware_error(const char* name) {
    //TODO
    //uerror("Attempt to use unimplemented hardware function '%s'",name);
  }

  virtual void mov (Tuple val) { hardware_error("mov"); }
  virtual void flex (Number val) { hardware_error("flex"); }
  virtual void die (Number val) { hardware_error("die"); }
  virtual void clone_machine (Number val) { hardware_error("clone_machine"); }
  
  virtual Number cam_get (int k) { hardware_error("cam_get"); return 0; }
  virtual Number radius_get () { hardware_error("radius_get"); return 0; }
  virtual Number radius_set (Number val) { hardware_error("radius_set"); return 0; }
  
  virtual void set_r_led (Number val) { hardware_error("set_r_led"); }
  virtual void set_g_led (Number val) { hardware_error("set_g_led"); }
  virtual void set_b_led (Number val) { hardware_error("set_b_led"); }
  virtual void set_probe (Data d, uint8_t p) { hardware_error("set_probe"); }
  virtual void set_speak (Number period) { hardware_error("set_speak"); }
  
  virtual Number set_dt (Number dt) { hardware_error("set_dt"); return 0; }

  virtual void set_is_folding (bool val, int k) 
  { hardware_error("set_is_folding"); }
  virtual bool read_fold_complete (int val) 
  { hardware_error("read_fold_complete"); return 0; }
  
  virtual Number set_channel (Number diffusion, int k) 
  { hardware_error("set_channel"); return 0; }
  virtual Number read_channel (int k)
  { hardware_error("read_channel"); return 0;}
  virtual Number drip_channel (Number val, int k) 
  { hardware_error("drip_channel"); return 0; }
  virtual Tuple grad_channel (int k)
  { hardware_error("grad_channel"); return Tuple(); }
  
  virtual Number read_radio_range () 
  { hardware_error("read_radio_range"); return 0; }
  virtual Number read_light_sensor()
  { hardware_error("read_light_sensor"); return 0; }
  virtual Number read_microphone ()
  { hardware_error("read_microphone"); return 0; }
  virtual Number read_temp () { hardware_error("read_temp"); return 0; }
  virtual Number read_short () { hardware_error("read_short"); return 0; }
  virtual Number read_sensor (uint8_t n) 
  { hardware_error("read_sensor"); return 0; }
  virtual Tuple read_coord_sensor()
  { hardware_error("read_coord_sensor"); return Tuple(); }
  virtual Tuple read_mouse_sensor()
  { hardware_error("read_mouse_sensor"); return Tuple(); }
  virtual Tuple read_ranger () { hardware_error("read_ranger"); return Tuple(); }
  virtual Number read_bearing () { hardware_error("read_bearing"); return 0;}
  virtual Number read_speed () { hardware_error("read_speed"); return 0; }
  virtual Number read_bump () { hardware_error("read_bump"); return 0; }
  virtual Number read_button (uint8_t n) { hardware_error("read_button"); return 0; }
  virtual Number read_slider (uint8_t ikey, uint8_t dkey, Number init, 
                               Number incr, Number min, Number max) 
  { hardware_error("read_slider"); return 0; }
  
  virtual int radio_send_export (uint8_t version, Array<Data> const & data)
  { hardware_error("radio_send_export"); return 0; }
  virtual int radio_send_script_pkt (uint8_t version, uint16_t n, 
                                      uint8_t pkt_num, uint8_t *script) 
  { hardware_error("radio_send_script_pkt"); return 0; }
  virtual int radio_send_digest (uint8_t version, uint16_t script_len, 
                                 uint8_t *digest) 
  { hardware_error("radio_send_digest"); return 0; }
};

// A list of all the functions that can be supplied with a HardwarePatch,
// for use in applying the patches in a SimulatedHardware
enum HardwareFunction {
  MOV_FN, FLEX_FN,
  SET_PROBE_FN,
  SET_DT_FN,
  READ_RADIO_RANGE_FN,
  READ_BEARING_FN,
  READ_SPEED_FN,
  RADIO_SEND_EXPORT_FN,
  RADIO_SEND_SCRIPT_PKT_FN,
  RADIO_SEND_DIGEST_FN,
  NUM_HARDWARE_FNS
};

class OpHandlerBase {
 public:
  virtual void operator()(Machine* machine){}
  int opcode;
  const char* defop;
};

template <class Tclass> class OpHandler : public OpHandlerBase {
 private:
  void (Tclass::*fnp)(Machine* machine);
  Tclass* pThis;
 public:
  OpHandler(Tclass* _pThis, void (Tclass::*_fnp)(Machine* machine), const char* _defop) {
    fnp = _fnp;
    pThis = _pThis;
    defop = _defop;
  }
  virtual void operator()(Machine* machine) {
    (*pThis.*fnp)(machine);
  }
};

#define PLATFORM_OPCODE_MAX_COUNT 256
#define PLATFORM_OPCODE_OFFSET 200

// This class dispatches kernel hardware calls to the appropriate patches
class Device;
class SimulatedHardware {
  OpHandlerBase* opHandlers[PLATFORM_OPCODE_MAX_COUNT];
  int opHandlerCount;

public:
  bool is_kernel_debug, is_kernel_trace, is_kernel_debug_script;
  HardwarePatch base;
  HardwarePatch* patch_table[NUM_HARDWARE_FNS];
  vector<HardwareFunction> requiredPatches; // corresponding to // Universal sensing & actuation ops
  vector<const char*> requiredOpcodes;
  SimulatedHardware();
  void patch(HardwarePatch* p, HardwareFunction fn); // instantiate a fn
  void set_vm_context(Device* d); // prepare globals for kernel execution
  void dumpPatchTable();
  int registerOpcode(OpHandlerBase* opHandler);
  void dispatchOpcode(uint8_t op);
  void appendDefops(string& defops);
};

// globals that carry the VM context for kernel hardware calls
// HardwarePatch classes can count on them being set to correct values
extern SimulatedHardware* hardware;
extern Device* device;
extern Machine* machine;

Device* current_device();
SimulatedHardware* current_hardware();
Machine* current_machine();

#endif //__SIM_HARDWARE__
