/* Random point-to-point radio simulation

Copyright (C) 2009, Nelson Elhage and contributors listed in the
AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory.  */

#include "config.h"
#include "wormhole-radio.h"
#include "visualizer.h"

WormHoleRadio::WormHoleRadio(Args *args, SpatialComputer *p, int n) : RadioSim(args, p) {
  if(args->extract_switch("-wn")) {
    n_wormholes = (int)args->pop_number();
  } else {
    n_wormholes = 0;
  }

  devices_left = n;

  p->hardware.patch(this,READ_RADIO_RANGE_FN);
  p->hardware.patch(this,RADIO_SEND_EXPORT_FN);
  p->hardware.patch(this,RADIO_SEND_SCRIPT_PKT_FN);
  p->hardware.patch(this,RADIO_SEND_DIGEST_FN);
}

WormHoleRadio::~WormHoleRadio() {
  
}

bool WormHoleRadio::handle_key(KeyEvent *key) {
  return RadioSim::handle_key(key);
}

void WormHoleRadio::add_device(Device* d) {
  d->layers[id] = new WormHoleRadioDevice(this,d);
  if(--devices_left == 0) {attach_wormholes();} // connect when last device made
}

void WormHoleRadio::attach_wormholes() {
  int w = 0;
  while(w < n_wormholes) {
    WormHoleRadioDevice *d1 = random_device();
    WormHoleRadioDevice *d2 = random_device();

    if(d1 != d2 && d1->nbrs.find(d2) == d1->nbrs.end()) {
      connect_devices(d1, d2);
      w++;
    }
  }
}

WormHoleRadioDevice *WormHoleRadio::random_device() {
  Device *d;
  do {
    int i = rand() % parent->devices.max_id();
    d = (Device*)parent->devices.get(i);
  } while(d == NULL);
  return (WormHoleRadioDevice*)(d->layers[id]);
}

void WormHoleRadio::connect_devices(WormHoleRadioDevice *d1, WormHoleRadioDevice *d2) {
  d1->nbrs.insert(d2);
  d2->nbrs.insert(d1);
}

Number WormHoleRadio::read_radio_range () {
  /* FIXME There isn't really a reasonable value to return here.*/
  return 10;
}

int WormHoleRadio::radio_send_export (uint8_t version, Array<Data> const & data){
  if(!try_tx())  // transmission failure
    return 0;

  int src_id = device->uid;
  const flo *me = device->body->position();
  WormHoleRadioDevice *dev = (WormHoleRadioDevice*)device->layers[id];
  for(set<WormHoleRadioDevice*>::iterator it = dev->nbrs.begin();
      it != dev->nbrs.end(); it++) {
    WormHoleRadioDevice *o = *it;
    if(try_rx()) {
      const flo *them = o->container->body->position();

      Neighbour & nbr = o->container->vm->hood[src_id];
      nbr.x = me[0]-them[0];
      nbr.y = me[1]-them[1];
      nbr.z = me[2]-them[2];
      nbr.imports = data;
      nbr.data_age = 0;
    }
  }

  hardware->set_vm_context(dev->container);
}

int WormHoleRadio::radio_send_script_pkt (uint8_t version, uint16_t n,
                                          uint8_t pkt_num, uint8_t *script) {
/*  if(!try_tx())  // transmission failure
    return 0;

  int src_id = device->uid;
  WormHoleRadioDevice *dev = (WormHoleRadioDevice*)device->layers[id];
  for(set<WormHoleRadioDevice*>::iterator it = dev->nbrs.begin();
      it != dev->nbrs.end(); it++) {
    WormHoleRadioDevice *o = *it;
    if(try_rx()) {
      hardware->set_vm_context(o->container);
      radio_receive_script_pkt(version, n, pkt_num, script);
    }
  }

  hardware->set_vm_context(dev->container);*/
}

int WormHoleRadio::radio_send_digest (uint8_t version, uint16_t script_len,
                                      uint8_t *digest) {
/*  if(!try_tx())  // transmission failure
    return 0;

  int src_id = device->uid;
  WormHoleRadioDevice *dev = (WormHoleRadioDevice*)device->layers[id];
  for(set<WormHoleRadioDevice*>::iterator it = dev->nbrs.begin();
      it != dev->nbrs.end(); it++) {
    WormHoleRadioDevice *o = *it;
    if(try_rx()) {
      hardware->set_vm_context(o->container);
      radio_receive_digest(version, script_len, digest);
    }
  }

  hardware->set_vm_context(dev->container);-*/
}

WormHoleRadioDevice::WormHoleRadioDevice(WormHoleRadio *parent, Device *container)
  : DeviceLayer(container) {
  this->parent = parent;
}

WormHoleRadioDevice::~WormHoleRadioDevice() {
  
}

void WormHoleRadioDevice::visualize() {
#ifdef WANT_GLUT
  if (parent->is_show_backoff) {
    glPushMatrix();
    container->text_scale(); // prepare to draw text
    char buf[20];
    palette->use_color(RadioSim::RADIO_BACKOFF);
    sprintf(buf, "N/A");
    draw_text(1, 1, buf);
    glPopMatrix();
  }

  if(parent->is_show_connectivity) { // draw network connections
    // setup line properties
    bool local_sharp=(parent->connect_display_mode==1 && 
                      container->is_selected);
    if(parent->connect_display_mode==2 || local_sharp) {
      palette->use_color(RadioSim::NET_CONNECTION_SHARP);
      glLineWidth(1);
    } else {
      palette->use_color(RadioSim::NET_CONNECTION_FUZZY);
      glLineWidth(4);
    }
    // do the actual draw
    glBegin(GL_LINES);
    const flo *me = container->body->position();
    for(set<WormHoleRadioDevice*>::iterator it = nbrs.begin(); it != nbrs.end(); it++) {
      if((local_sharp || (*it)->container->uid > container->uid)) {
        const flo *them = (*it)->container->body->position();
        glVertex3f(0,0,0);
        glVertex3f(them[0]-me[0], them[1]-me[1], them[2]-me[2]);
      }
    }
    glEnd();
    glLineWidth(1);
  }
#endif // WANT_GLUT
}
