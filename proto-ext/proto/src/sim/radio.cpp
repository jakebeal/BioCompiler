/* Common functionality for most radio simulations.

Copyright (C) 2009, Nelson Elhage and contributors listed in the
AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory.  */

#include "config.h"
#include "radio.h"
#include "visualizer.h"

RadioSim::RadioSim(Args* args, SpatialComputer* p) : Layer(p) {
  ensure_colors_registered("RadioSim");
  tx_error = (args->extract_switch("-txerr"))?args->pop_number():0.0;
  rx_error = (args->extract_switch("-rxerr"))?args->pop_number():0.0;

  is_show_backoff = args->extract_switch("-show-radio-backoff");
  is_show_connectivity = args->extract_switch("-c");
  connect_display_mode = 
    (args->extract_switch("-sharp-connections")) ? 2 :
    (args->extract_switch("-sharp-neighborhood")) ? 1 : 0;
}

// register colors to use
Color *RadioSim::NET_CONNECTION_FUZZY, *RadioSim::NET_CONNECTION_SHARP, 
  *RadioSim::NET_CONNECTION_LOGICAL, *RadioSim::RADIO_BACKOFF;
void RadioSim::register_colors() {
#ifdef WANT_GLUT
  NET_CONNECTION_FUZZY = 
    palette->register_color("NET_CONNECTION_FUZZY", 0, 1, 0, 0.25);
  NET_CONNECTION_SHARP = 
    palette->register_color("NET_CONNECTION_SHARP", 0, 1, 0, 1);
  NET_CONNECTION_LOGICAL = 
    palette->register_color("NET_CONNECTION_LOGICAL", 0.5, 0.5, 1, 0.8);
  RADIO_BACKOFF = palette->register_color("RADIO_BACKOFF", 1, 0, 0, 0.8);
#endif
}

RadioSim::~RadioSim() {
  
}

bool RadioSim::handle_key(KeyEvent* key) {
  if(key->normal) {
    if(!key->ctrl) {
      switch(key->key) {
      case 'c': is_show_connectivity = !is_show_connectivity; return true;
      case 'C': connect_display_mode = (connect_display_mode+1)%3; return true;
      case 'S': is_show_backoff = !is_show_backoff; return true;
      }
    }
  }
  return false;
}

bool RadioSim::try_tx() {
  return tx_error==0 || urnd(0,1) > tx_error;
}

bool RadioSim::try_rx() {
  return rx_error==0 || urnd(0,1) > rx_error;
}
