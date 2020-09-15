/* Multiple Radio Simulation
Copyright (C) 2009, Nelson Elhage and contributors listed in the
AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory.  */

#ifndef __MULTIRADIO__
#define __MULTIRADIO__

#include "spatialcomputer.h"
#include "radio.h"
#include <vector>

class MultiRadio : public RadioSim {
public:
  MultiRadio(Args *args, SpatialComputer *parent, int n);
  ~MultiRadio();

  void add_radio(RadioSim *r);

  bool handle_key(KeyEvent* key);
  void add_device(Device* d);
  void device_moved(Device *d);

  int radio_send_export (uint8_t version, Array<Data> const & n);
  int radio_send_script_pkt (uint8_t version, uint16_t n, 
                             uint8_t pkt_num, uint8_t *script);
  int radio_send_digest (uint8_t version, uint16_t script_len, 
                         uint8_t *digest);

private:
  void update_patches();

  std::vector<RadioSim*> radios;
};

#endif
