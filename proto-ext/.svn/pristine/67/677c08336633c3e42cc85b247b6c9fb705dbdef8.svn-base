/* Multiple radio simulation

Copyright (C) 2009, Nelson Elhage and contributors listed in the
AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory.  */

// WARNING: currently broken... but at least we can load the plugin.
#include "config.h"
#include "multiradio.h"
#include "plugin_manager.h"
#include <algorithm>

MultiRadio::MultiRadio(Args *args,SpatialComputer *p,int n) : RadioSim(args, p){
  while(args->extract_switch("-radio",false)) {
    string r = args->pop_next();
    RadioSim* rs = (RadioSim*)plugins.get_sim_plugin(LAYER_PLUGIN,r,args,p,n);
    if(rs!=NULL) add_radio(rs);
    else uerror("Unable to find radio: %s\n",r.c_str());
  }
  update_patches();
}

MultiRadio::~MultiRadio() {
  
}

void MultiRadio::add_radio(RadioSim *r) {
  if(std::find(radios.begin(), radios.end(), r)  != radios.end()) {
    post("warning: registering the same radio twice");
  }
  
  radios.push_back(r);

  r->tx_error = tx_error;
  r->rx_error = rx_error;
  r->is_show_connectivity = is_show_connectivity;
  r->is_show_backoff = is_show_backoff;
  r->connect_display_mode = connect_display_mode;

  update_patches();
}

void MultiRadio::update_patches() {
  parent->hardware.patch(this,RADIO_SEND_EXPORT_FN);
  parent->hardware.patch(this,RADIO_SEND_SCRIPT_PKT_FN);
  parent->hardware.patch(this,RADIO_SEND_DIGEST_FN);
}

bool MultiRadio::handle_key(KeyEvent *key) {
  bool handled = false;
  vector<RadioSim*>::iterator it;

  for(it = radios.begin(); it != radios.end(); it++) {
    if((*it)->handle_key(key)) {
      handled = true;
    }
  }

  return handled;
}

void MultiRadio::add_device(Device *d) {}

void MultiRadio::device_moved(Device *d) {}

int MultiRadio::radio_send_export (uint8_t version, Array<Data> const & data){
  vector<RadioSim*>::iterator it;
  for(it = radios.begin(); it != radios.end(); it++) {
    (*it)->radio_send_export(version, data);
  }
}

int MultiRadio::radio_send_script_pkt (uint8_t version, uint16_t n,
                                       uint8_t pkt_num, uint8_t *script) {
/*  vector<RadioSim*>::iterator it;
  for(it = radios.begin(); it != radios.end(); it++) {
    (*it)->radio_send_script_pkt(version, n, pkt_num, script);
  }*/
}

int MultiRadio::radio_send_digest (uint8_t version, uint16_t script_len,
                                   uint8_t *digest) {
/*  vector<RadioSim*>::iterator it;
  for(it = radios.begin(); it != radios.end(); it++) {
    (*it)->radio_send_digest(version, script_len, digest);
  }*/
}
