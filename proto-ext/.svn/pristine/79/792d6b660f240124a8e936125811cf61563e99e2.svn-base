/* Not really a radio: just creates undirected links between device pairs
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "config.h"
#include "graph_link_radio.h"
#include "visualizer.h"

/*****************************************************************************
 *  Graph Link Radio                                                         *
 *****************************************************************************/
GraphLinkRadio::GraphLinkRadio(Args* args, SpatialComputer* p, int n) : RadioSim(args, p) {
  ensure_colors_registered("GraphLinkRadio");
  // Read graphs
  while(args->extract_switch("--graph",false))
    parse_graph_file(args->pop_next());
  // display options
  args->undefault(&can_dump,"-Dradio","-NDradio");
  is_fast_prune_hood = !args->extract_switch("-no-motion-pruning");
  // register hardware patches
  p->hardware.patch(this,READ_RADIO_RANGE_FN);
  p->hardware.patch(this,RADIO_SEND_EXPORT_FN);
  p->hardware.patch(this,RADIO_SEND_SCRIPT_PKT_FN);
  p->hardware.patch(this,RADIO_SEND_DIGEST_FN);
}

// Can be called multiple times to concatenate graph files
void GraphLinkRadio::parse_graph_file(const char* filename, bool warnfail) {
  FILE* file;
  if((file = fopen(filename, "r"))==NULL) {
    if(warnfail)
      debug("WARNING: Couldn't open graph file %s.\n",filename);
  } else {
    char buf[255]; int line=0;
    while(fgets(buf,255,file)) {
      line++;
      int id1, id2;
      if(buf[0] == '%') continue; // comment
      int n = sscanf(buf,"%i %i",&id1,&id2);
      if(n==EOF || n==0) continue; // whitespace
      if(n==2) {
        // push both directions into map
        linkmap[id1].insert(id2);
        linkmap[id2].insert(id1);
      } else {
	debug("WARNING: link at %s line %d; should be ID1 ID2\n",
	      filename,line);
      }
    }
    fclose(file);
  }
}

// register colors to use
void GraphLinkRadio::register_colors() {
#ifdef WANT_GLUT
  // no special colors
#endif
}

GraphLinkRadio::~GraphLinkRadio() {}

bool GraphLinkRadio::handle_key(KeyEvent* key) {
  if(key->normal) {
    if(key->ctrl) {
      switch(key->key) {
      }
    } else {
      switch(key->key) {
      }
    }
  }
  return RadioSim::handle_key(key);
}

struct GLNbrRecord {
  GraphLinkDevice* nbr;
  int backptr; // location of corresponding record in neighbor
  METERS dp[3]; // difference in position
  GLNbrRecord(GraphLinkDevice* nbr, const METERS* p, const METERS* np) {
    this->nbr = nbr; backptr = -1;
    for(int i=0;i<3;i++) dp[i]=np[i]-p[i];
  }
};

void GraphLinkRadio::connect_device(Device* d) {
  GraphLinkDevice* udd = (GraphLinkDevice*)d->layers[id];
  const flo* p = d->body->position();
  set<int>& link_set = linkmap[d->uid];
  for(int i=0; i<device_list.size();i++) {
    Device* nbrd = device_list[i]->container;
    if(link_set.count(nbrd->uid)) {
      GraphLinkDevice* nbr = (GraphLinkDevice*)nbrd->layers[id];
      const flo* np = nbrd->body->position();
      GLNbrRecord* nnr = new GLNbrRecord(udd,np,p);
      GLNbrRecord* nr = new GLNbrRecord(nbr,p,np);
      nr->backptr = nbr->neighbors.add(nnr);
      nnr->backptr = udd->neighbors.add(nr);
    }
  }
}

void GraphLinkRadio::disconnect_device(Device *d) {
  GraphLinkDevice* udd = (GraphLinkDevice*)d->layers[id];
  // disconnect from each neighbor
  for(int i=0;i<udd->neighbors.max_id();i++) {
    GLNbrRecord* nr = (GLNbrRecord*)udd->neighbors.get(i);
    if(nr) { 
      GLNbrRecord* nnr = (GLNbrRecord*)nr->nbr->neighbors.remove(nr->backptr);
      if(nnr->backptr!=i) debug("Bad nbr backptr: %d!=%d\n",i,nnr->backptr);
      if(nnr->nbr != udd) debug("Bad local backptr\n");
      delete nnr; delete nr;
    }
  }
  // purge local lists
  udd->neighbors.clear();
}

void GraphLinkRadio::add_device(Device* d) {
  GraphLinkDevice* new_device = new GraphLinkDevice(this,d);
  d->layers[id] = new_device;
  device_list.push_back(new_device);
  connect_device(d);
}

void GraphLinkRadio::device_moved(Device* d) {
  // just get the locations updated, if they matter... never a change in nbrs
  disconnect_device(d); connect_device(d);
}

/*****************************************************************************
 *  HARDWARE EMULATION                                                       *
 *****************************************************************************/
// No real range, just hops (= 1 "unit")
Number GraphLinkRadio::read_radio_range () { return 1; }

int GraphLinkRadio::radio_send_export (uint8_t version, Array<Data> const & data) {
  if(!try_tx())  // transmission failure
    return 0;

  // cache data
  int src_id = device->uid;
  // walk neighbors
  GraphLinkDevice* udd = (GraphLinkDevice*)device->layers[id];
  for(int i=0;i<udd->neighbors.max_id();i++) {
    GLNbrRecord* nr = (GLNbrRecord*)udd->neighbors.get(i);
    if(nr && try_rx()) { // non-failing receive
      // hardware->set_vm_context(nr->nbr->container);
      /*radio_receive_export(src_id, version, timeout, -nr->dp[0], -nr->dp[1],
                           -nr->dp[2], n, buf);*/
      Neighbour & nbr = nr->nbr->container->vm->hood[src_id];
      for(Size i = 0; i < data.size(); i++) nbr.imports[i] = data[i];
      nbr.x = -nr->dp[0];
      nbr.y = -nr->dp[1];
      nbr.z = -nr->dp[2];
      nbr.data_age = 0;
    }
  }
  // hardware->set_vm_context(udd->container); // restore context
  return 1;
}

int GraphLinkRadio::radio_send_script_pkt (uint8_t version, uint16_t n, 
                                          uint8_t pkt_num, uint8_t *script) {
  // currently disabled
}

int GraphLinkRadio::radio_send_digest (uint8_t version, uint16_t script_len, 
                                      uint8_t *digest) {
  // currently disabled
}

/*****************************************************************************
 *  UNIT DISC RADIO                                                          *
 *****************************************************************************/

GraphLinkDevice::GraphLinkDevice(GraphLinkRadio* parent, Device* container) 
  : DeviceLayer(container) { this->parent = parent; }

GraphLinkDevice::~GraphLinkDevice() {
  if(parent->is_fast_prune_hood) { // delete self from each neighbor
    for(int i=0;i<neighbors.max_id();i++) {
      GLNbrRecord* nr = (GLNbrRecord*)neighbors.get(i);
      if(nr) {
	Machine* nvm = nr->nbr->container->vm;
	NeighbourHood::iterator i = nvm->hood.find(container->uid);
	if (i != nvm->hood.end()) nvm->hood.remove(i);
	  }
    }
  }
  parent->disconnect_device(container);
  // remove from list of neighbors
  for_vec(GraphLinkDevice*,parent->device_list,i) {
    if(*i==this) { parent->device_list.erase(i); break; }
  }
}

void GraphLinkDevice::visualize() {
#ifdef WANT_GLUT
  if(parent->is_show_connectivity) { // draw network connections
    // setup line properties
    bool local_sharp=(parent->connect_display_mode==1 && 
                      container->is_selected);
    if(parent->connect_display_mode==2 || local_sharp) {
      palette->use_color(GraphLinkRadio::NET_CONNECTION_SHARP);
      glLineWidth(1);
    } else {
      palette->use_color(GraphLinkRadio::NET_CONNECTION_FUZZY);
      glLineWidth(4);
    }
    // do the actual draw
    glBegin(GL_LINES);
    for(int i=0;i<neighbors.max_id();i++) {
      GLNbrRecord* nr = (GLNbrRecord*)neighbors.get(i);
      if(nr && (local_sharp || nr->nbr->container->uid > container->uid)) {
        glVertex3f(0,0,0);
        glVertex3f(nr->dp[0],nr->dp[1],nr->dp[2]);
      }
    }
    glEnd();
    glLineWidth(1);
  }
#endif // WANT_GLUT
}
