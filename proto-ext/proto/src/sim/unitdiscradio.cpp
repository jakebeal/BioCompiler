/* Simple radio simulation
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "config.h"
#include "unitdiscradio.h"
#include "visualizer.h"

/*****************************************************************************
 *  UNIT DISC RADIO                                                          *
 *****************************************************************************/
UnitDiscRadio::UnitDiscRadio(Args* args, SpatialComputer* p, int n) : RadioSim(args, p) {
  ensure_colors_registered("UnitDiscRadio");
  if(args->extract_switch("-ns")) { // set range from neighborhood size
    flo ns = args->pop_number(); // note: counts *self* as a neighbor
    if(args->extract_switch("-r")) {
      args->pop_number();
      post("WARNING: '-r' radius overridden by '-ns' neighborhood size");
    }
    flo volume = (p->volume->r-p->volume->l)*(p->volume->t-p->volume->b);
    if(p->volume->dimensions()==2) {
      range = sqrt(ns*volume/(n*M_PI));
    } else { 
      volume *= (((Rect3*)p->volume)->c-((Rect3*)p->volume)->f);
      range = pow(ns*volume/((4.0/3.0)*n*M_PI),1.0/3.0);
    }
  } else {
    range = (args->extract_switch("-r"))?args->pop_number():15.0;
  }
  r_sqr = range*range; // cache the square for distance calcs
  // display options
  is_show_logical_nbrs = args->extract_switch("-lc");
  is_show_radio = args->extract_switch("-show-radio");
  is_fast_prune_hood = !args->extract_switch("-no-motion-pruning");
  is_debug_radio = args->extract_switch("-debug-radio");
  args->undefault(&can_dump,"-Dradio","-NDradio");
  // register hardware patches
  p->hardware.patch(this,READ_RADIO_RANGE_FN);
  p->hardware.patch(this,RADIO_SEND_EXPORT_FN);
  p->hardware.patch(this,RADIO_SEND_SCRIPT_PKT_FN);
  p->hardware.patch(this,RADIO_SEND_DIGEST_FN);

  // make internal space representation, starting with grid size
  create_cell_representation();
}

void UnitDiscRadio::create_cell_representation() {
  SpatialComputer* p = parent;
  // make internal space representation, starting with grid size
  cell_rows = (int)ceil((p->volume->r-p->volume->l)/range)+2;
  cell_cols = (int)ceil((p->volume->t-p->volume->b)/range)+2;
  cell_lvls = (p->volume->dimensions()==2)?1:
    (int)ceil((((Rect3*)p->volume)->c-(((Rect3*)p->volume)->f))/range)+2;
  // pre-calculate bounds
  cell_left = p->volume->l-range; cell_bottom = p->volume->b-range;
  cell_floor = (p->volume->dimensions()==2)?1:
    ((Rect3*)p->volume)->f-range;
  lvl_size = cell_rows*cell_cols; num_cells = lvl_size*cell_lvls;
  // make and populate the cell table
  cells = (Population**)calloc(num_cells,sizeof(Population*));
  for(int i=0;i<num_cells;i++) cells[i] = new Population();
}

void UnitDiscRadio::change_radio_range(float newrange) {
  range = newrange; r_sqr = range*range;
  // Get rid of old cell representation
  for(int i=0;i<num_cells;i++) { delete cells[i]; }
  free(cells); 
  // Replace with new representation
  create_cell_representation();
  // Fill new cells with all devices
  for(int i=0;i<parent->devices.size();i++) {
    Device* d = (Device*)parent->devices.get(i); if(d==NULL) continue;
    UnitDiscDevice* udd = (UnitDiscDevice*)d->layers[id];
    int c = udd->cell_id = device_cell(d);
    udd->cell_loc = cells[c]->add(d);
  }
}

// register colors to use
Color *UnitDiscRadio::RADIO_RANGE_RING, *UnitDiscRadio::RADIO_CELL_INFO;
void UnitDiscRadio::register_colors() {
#ifdef WANT_GLUT
  RADIO_RANGE_RING = 
    palette->register_color("RADIO_RANGE_RING", 0.25, 0.25, 0.25, 0.8);
  RADIO_CELL_INFO = palette->register_color("RADIO_CELL_INFO", 0, 1, 1, 0.8);
#endif
}

vector<HardwareFunction> UnitDiscRadio::getImplementedHardwareFunctions()
{

  vector<HardwareFunction> hardwareFunctions;
    hardwareFunctions.push_back(READ_RADIO_RANGE_FN);
    hardwareFunctions.push_back(RADIO_SEND_EXPORT_FN);
    hardwareFunctions.push_back(RADIO_SEND_SCRIPT_PKT_FN);
    hardwareFunctions.push_back(RADIO_SEND_DIGEST_FN);
    return hardwareFunctions;
}

UnitDiscRadio::~UnitDiscRadio() {
  for(int i=0;i<num_cells;i++) delete cells[i]; // populations assumed empty
  free(cells);
}

bool UnitDiscRadio::handle_key(KeyEvent* key) {
  if(key->normal) {
    if(key->ctrl) {
      switch(key->key) {
      case 3:  // Ctrl-C = logical neighbors
        is_show_logical_nbrs = !is_show_logical_nbrs; return true;
      }
    } else {
      switch(key->key) {
      case 'r': is_show_radio = !is_show_radio; return true;
      case 'R': change_radio_range(range+1); return true;
      case 'E': change_radio_range(range-1); return true;
      }
    }
  }
  return RadioSim::handle_key(key);
}

// calculate which internal storage cell contains a device
int UnitDiscRadio::device_cell(Device* d) {
  const flo* p = d->body->position();
  int row = (int)floor((p[0]-cell_left)/range);
  int col = (int)floor((p[1]-cell_bottom)/range);
  row = BOUND(row,0,cell_rows-1); col = BOUND(col,0,cell_cols-1);
  int n = row*cell_cols + col;
  if(cell_lvls>1) { // 3D
    int lvl = (int)floor((p[2]-cell_floor)/range);
    lvl = BOUND(lvl,0,cell_lvls-1);
    n += lvl_size*lvl;
  }
  return n;
}

// squared distance between two points
flo range3sqr(const flo* a, const flo* b) {
  flo dx = a[0]-b[0], dy = a[1]-b[1], dz = a[2]-b[2];
  return dx*dx + dy*dy + dz*dz;
}

struct NbrRecord {
  UnitDiscDevice* nbr;
  int backptr; // location of corresponding record in neighbor
  METERS dp[3]; // difference in position
  NbrRecord(UnitDiscDevice* nbr, const METERS* p, const METERS* np) {
    this->nbr = nbr; backptr = -1;
    for(int i=0;i<3;i++) dp[i]=np[i]-p[i];
  }
};

// handle the actual connections of a device into a cell
void UnitDiscRadio::connect_to_cell(Device* d,int cell_id) {
  if(cell_id<0 || cell_id>=num_cells) return; // bounds check
  Population* c = cells[cell_id];
  UnitDiscDevice* udd = (UnitDiscDevice*)d->layers[id];
  const flo* p = d->body->position();
  bool debug = is_debug_radio && d->debug();
  for(int i=0;i<c->max_id();i++) {
    Device* nbrd = (Device*)c->get(i);
    if(nbrd) {
      const flo* nbrp = nbrd->body->position();
      if(debug) post("Nbr? %d (dist=%f)\n",nbrd->uid,sqrt(range3sqr(p,nbrp)));
      if(range3sqr(p,nbrp)<r_sqr) { // connect if close enough
        UnitDiscDevice* nbr = (UnitDiscDevice*)nbrd->layers[id];
        const flo* np = nbrd->body->position();
        NbrRecord* nnr = new NbrRecord(udd,np,p);
        NbrRecord* nr = new NbrRecord(nbr,p,np);
        nr->backptr = nbr->neighbors.add(nnr);
        nnr->backptr = udd->neighbors.add(nr);
        if(debug) post("Accepted nbr %d\n",nbrd->uid);
      } else {
        if(debug) post("Rejected possible nbr %d\n",nbrd->uid);
      }
    }
  }
}
// connect a device to its 2D-adjacent cells
// Note that this will check for connections wrapping around the edge
// of the world, but that should not greatly increase cost unless many
// nodes are outside the original boundaries
void UnitDiscRadio::connect_device2(Device* d, int base) { 
  bool debug = is_debug_radio && d->debug();
  const flo* p = d->body->position();
  if(debug) post("id=%d Pos=[%f,%f,%f], in cell %d [of %d x %d x %d] , check:",
                 base,cell_rows,cell_cols,cell_lvls,d->uid,p[0],p[1],p[2]);
  for(int i=-cell_cols;i<=cell_cols;i+=cell_cols)
    for(int j=-1;j<=1;j++) {
      if(debug) post("cell %d\n",base+i+j);
      connect_to_cell(d,base+i+j);
    }
  if(debug) {
    post("Final nbr collection:");
    UnitDiscDevice* udd = (UnitDiscDevice*)d->layers[id];
    for(int i=0;i<udd->neighbors.max_id();i++) {
      NbrRecord* nr = (NbrRecord*)udd->neighbors.get(i);
      if(nr) { post(" %d",nr->nbr->container->uid); }
    }
    post("\n");
  }
}

void UnitDiscRadio::connect_device(Device* d) {
  UnitDiscDevice* udd = (UnitDiscDevice*)d->layers[id];
  int c = udd->cell_id = device_cell(d);
  // find neighbors
  connect_device2(d,c);
  if(cell_lvls>1) { // 3D
    if(c-lvl_size>=0) connect_device2(d,c-lvl_size);
    if(c+lvl_size<num_cells) connect_device2(d,c+lvl_size);
  }
  // insert into cell
  udd->cell_loc = cells[c]->add(d);
}
void UnitDiscRadio::disconnect_device(Device *d) {
  UnitDiscDevice* udd = (UnitDiscDevice*)d->layers[id];
  // disconnect from each neighbor
  for(int i=0;i<udd->neighbors.max_id();i++) {
    NbrRecord* nr = (NbrRecord*)udd->neighbors.get(i);
    if(nr) { 
      NbrRecord* nnr = (NbrRecord*)nr->nbr->neighbors.remove(nr->backptr);
      if(nnr->backptr!=i) debug("Bad nbr backptr: %d!=%d\n",i,nnr->backptr);
      if(nnr->nbr != udd) debug("Bad local backptr\n");
      delete nnr; delete nr;
    }
  }
  // purge local lists & remove from cell
  udd->neighbors.clear();
  Device* removed = (Device*)cells[udd->cell_id]->remove(udd->cell_loc);
  if(removed!=d) { debug("Bad back cell reference!\n"); }
}

void UnitDiscRadio::add_device(Device* d) {
  d->layers[id] = new UnitDiscDevice(this,d);
  connect_device(d);
}
/*
inline int find_nbr_index (Machine *m, uint16_t src_id) {
  int left  = 0;
  int right = m->n_hood-1;
  // POST("M%d LOOKING UP NBR %d N_HOOD %d: ", m->id, src_id, m->n_hood);
  // post_nbrs(m); POST("\n");
  while (left <= right) {
    int mid = (left + right)>>1;
    NBR *nbr = m->hood[mid];
    // POST("M%d LOOKING AT %d M%d\n", m->id, mid, nbr->id);
    if (nbr->id == src_id) {
      // POST("M%d FOUND NBR AT %d\n", m->id, mid);
      return mid;
    } else if (src_id < nbr->id)
      right = mid - 1;
    else
      left  = mid + 1;
  }
  // POST("M%d LOST NBR %d\n", m->id, src_id);
  return -1;
}
*/
void UnitDiscRadio::device_moved(Device* d) {
  // now do the actual neighborhood update
  disconnect_device(d); connect_device(d);
  if(is_fast_prune_hood) { // delete the VM hood entries that are lost
    for(NeighbourHood::iterator i = d->vm->hood.begin(); i != d->vm->hood.end(); i++){
      i->in_range = false;
    }
    d->vm->thisMachine().in_range = true;
    UnitDiscDevice* udd = (UnitDiscDevice*)d->layers[id];
    for(int i=0;i<udd->neighbors.max_id();i++) {
      NbrRecord* nr = (NbrRecord*)udd->neighbors.get(i);
      if(nr) {
        MachineId nid = nr->nbr->container->uid;
        NeighbourHood::iterator nbr = d->vm->hood.find(nid);
        if (nbr != d->vm->hood.end()) nbr->in_range = true;
      }
    }
    for(NeighbourHood::iterator i = d->vm->hood.begin(); i != d->vm->hood.end(); ){
      if (i->in_range) {
        i++;
      } else {
        i = d->vm->hood.remove(i);
      }
    }
  }
}

/*****************************************************************************
 *  HARDWARE EMULATION                                                       *
 *****************************************************************************/
Number UnitDiscRadio::read_radio_range () { return range; }

int UnitDiscRadio::radio_send_export (uint8_t version, Array<Data> const & data) {
  if(!try_tx())  // transmission failure
    return 0;

  // cache data
  int src_id = device->uid;
  // walk neighbors
  UnitDiscDevice* udd = (UnitDiscDevice*)device->layers[id];
  for(int i=0;i<udd->neighbors.max_id();i++) {
    NbrRecord* nr = (NbrRecord*)udd->neighbors.get(i);
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

int UnitDiscRadio::radio_send_script_pkt (uint8_t version, uint16_t n, 
                                          uint8_t pkt_num, uint8_t *script) {
/*  if(!try_tx())  // transmission failure
    return 0;

  // walk neighbors
  UnitDiscDevice* udd = (UnitDiscDevice*)device->layers[id];
  for(int i=0;i<udd->neighbors.max_id();i++) {
    NbrRecord* nr = (NbrRecord*)udd->neighbors.get(i);
    if(nr && try_rx()) { // non-failing receive
      hardware->set_vm_context(nr->nbr->container);
      //radio_receive_script_pkt(version, n, pkt_num, script);
    }
  }
  hardware->set_vm_context(udd->container); // restore context
  //script_pkt_callback(pkt_num); */
}

int UnitDiscRadio::radio_send_digest (uint8_t version, uint16_t script_len, 
                                      uint8_t *digest) {
/*  if(!try_tx())
    return 0;

  // walk neighbors
  UnitDiscDevice* udd = (UnitDiscDevice*)device->layers[id];
  for(int i=0;i<udd->neighbors.max_id();i++) {
    NbrRecord* nr = (NbrRecord*)udd->neighbors.get(i);
    if(nr && try_rx()) { // non-failing receive
      hardware->set_vm_context(nr->nbr->container);
      //radio_receive_digest(version, script_len, digest);
    }
  }
  hardware->set_vm_context(udd->container); // restore context*/
}

/*****************************************************************************
 *  UNIT DISC RADIO                                                          *
 *****************************************************************************/

UnitDiscDevice::UnitDiscDevice(UnitDiscRadio* parent, Device* container) 
  : DeviceLayer(container) { this->parent = parent; }

UnitDiscDevice::~UnitDiscDevice() {
  if(parent->is_fast_prune_hood) { // delete self from each neighbor
    for(int i=0;i<neighbors.max_id();i++) {
      NbrRecord* nr = (NbrRecord*)neighbors.get(i);
      if(nr) {
	Machine* nvm = nr->nbr->container->vm;
	NeighbourHood::iterator i = nvm->hood.find(container->uid);
	if (i != nvm->hood.end()) nvm->hood.remove(i);
	  }
    }
  }
  parent->disconnect_device(container);
}

void UnitDiscDevice::visualize() {
#ifdef WANT_GLUT
  if (parent->is_debug_radio && container->debug()) {
    glPushMatrix();
    container->text_scale(); // prepare to draw text
    char buf[20];
    glTranslatef(0, -1, 0);
    sprintf(buf, "%2d", parent->device_cell(container));
    palette->use_color(UnitDiscRadio::RADIO_CELL_INFO);
    draw_text(2, 2, buf);
    glPopMatrix();
  }
  if (parent->is_show_backoff) {
    glPushMatrix();
    container->text_scale(); // prepare to draw text
    char buf[20];
    palette->use_color(UnitDiscRadio::RADIO_BACKOFF);
    sprintf(buf, "N/A");
    draw_text(1, 1, buf);
    glPopMatrix();
  }
  
  if(parent->is_show_radio) { // draw radio range
    palette->use_color(UnitDiscRadio::RADIO_RANGE_RING);
    draw_circle(parent->range);
  }
  if(parent->is_show_connectivity) { // draw network connections
    // setup line properties
    bool local_sharp=(parent->connect_display_mode==1 && 
                      container->is_selected);
    if(parent->connect_display_mode==2 || local_sharp) {
      palette->use_color(UnitDiscRadio::NET_CONNECTION_SHARP);
      glLineWidth(1);
    } else {
      if(parent->cell_lvls>1) {
        palette->scale_color(UnitDiscRadio::NET_CONNECTION_FUZZY,1,1,1,0.1);
      } else {
        palette->use_color(UnitDiscRadio::NET_CONNECTION_FUZZY);
      }
      glLineWidth(4);
    }
    // do the actual draw
    glBegin(GL_LINES);
    for(int i=0;i<neighbors.max_id();i++) {
      NbrRecord* nr = (NbrRecord*)neighbors.get(i);
      if(nr && (local_sharp || nr->nbr->container->uid > container->uid)) {
        glVertex3f(0,0,0);
        glVertex3f(nr->dp[0],nr->dp[1],nr->dp[2]);
      }
    }
    glEnd();
    glLineWidth(1);
  }
  // hood connectivity
  if(parent->is_show_logical_nbrs) {
    glLineWidth(2);
    palette->use_color(UnitDiscRadio::NET_CONNECTION_LOGICAL);
    glBegin(GL_LINES);
    Machine * m = container->vm;
    for(NeighbourHood::iterator i = m->hood.begin(); i != m->hood.end(); i++){
    	glVertex3f(0,0,0);
    	glVertex3f(i->x, i->y, i->z);
    }
    glLineWidth(1);
    glEnd();
  }
#endif // WANT_GLUT
}
