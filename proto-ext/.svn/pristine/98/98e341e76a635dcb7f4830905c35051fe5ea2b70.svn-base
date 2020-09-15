/* Simple radio simulation
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __UNITDISCRADIO__
#define __UNITDISCRADIO__

#include "spatialcomputer.h"
#include "radio.h"

class UnitDiscRadio : public RadioSim {
 public:
  // model options
  float range, r_sqr;        // radius of transmission (in meters) [and sq]
  // display options
  bool is_show_logical_nbrs;
  bool is_show_radio;
  bool is_debug_radio;      // turn on radio debugging
  bool is_fast_prune_hood;  // prune the VM neighborhood on movement?
  
 public:
  UnitDiscRadio(Args* args, SpatialComputer* parent, int n);
  ~UnitDiscRadio();
  bool handle_key(KeyEvent* key);
  void add_device(Device* d);
  void device_moved(Device* d);

  // hardware emulation
  Number read_radio_range ();
  int radio_send_export (uint8_t version, Array<Data> const & data);
  int radio_send_script_pkt (uint8_t version, uint16_t n, 
			     uint8_t pkt_num, uint8_t *script);
  int radio_send_digest (uint8_t version, uint16_t script_len, 
			 uint8_t *digest);
  
  // returns a list of function  that it patches/ provides impementation for
  static vector<HardwareFunction> getImplementedHardwareFunctions();

  friend class UnitDiscDevice;
 protected:
  // storage: gridded in range-size squares to cover screen
  // an additional layer of cells coats the edges, covering all outside area
  // it performs badly when devices are not well dispersed in the screen area
  Population** cells; // collections of Device pointers
  int cell_rows, cell_cols, cell_lvls, num_cells, lvl_size;
  METERS cell_left, cell_bottom, cell_floor;
  int device_cell(Device* d); // which cell is a device in?
  void connect_to_cell(Device* d,int cell_id); // handle connections
  void connect_device2(Device* d, int base); // iterate connection over 9 cells
  void connect_device(Device *d); // create all connections
  void disconnect_device(Device *d); // delete all connections

  void create_cell_representation();
  void change_radio_range(float newrange);

  virtual void register_colors();
  static Color *RADIO_RANGE_RING, *RADIO_CELL_INFO;
};

class UnitDiscDevice : public DeviceLayer {
 public:
  UnitDiscRadio* parent;
  // these values are actually managed by the UnitDiscRadio
  Population neighbors; // collection of NbrRecord* (internal definition)
  int cell_id; // which cell the device was last in (before motion)
  int cell_loc; // where is the device in its cell's list

  UnitDiscDevice(UnitDiscRadio* parent, Device* container);
  ~UnitDiscDevice();
  void visualize();
  void copy_state(DeviceLayer* src) {} // to be called during cloning
};

#endif // __UNITDISCRADIO__
