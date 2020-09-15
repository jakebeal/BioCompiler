/* Simple radio simulation
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __GRAPHLINKRADIO__
#define __GRAPHLINKRADIO__

#include "spatialcomputer.h"
#include "radio.h"

class GraphLinkDevice;
class GraphLinkRadio : public RadioSim {
 public:
  // display options
  bool is_fast_prune_hood;  // prune the VM neighborhood on removal?
  
 public:
  GraphLinkRadio(Args* args, SpatialComputer* parent, int n);
  ~GraphLinkRadio();
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
  
  friend class GraphLinkDevice;
 protected:
  // List of all links for each element
  map<int,set<int> > linkmap;
  vector<GraphLinkDevice*> device_list;

  void connect_device(Device *d); // create all connections
  void disconnect_device(Device *d); // delete all connections

  virtual void register_colors();
  void parse_graph_file(const char* filename, bool warnfail=true);
};

class GraphLinkDevice : public DeviceLayer {
 public:
  GraphLinkRadio* parent;
  // these values are actually managed by the GraphLinkRadio
  Population neighbors; // collection of NbrRecord* (internal definition)

  GraphLinkDevice(GraphLinkRadio* parent, Device* container);
  ~GraphLinkDevice();
  void visualize();
  void copy_state(DeviceLayer* src) {} // to be called during cloning
};

#endif // __GRAPHLINKRADIO__
