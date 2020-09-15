/* Top-level spatial computer classes
Copyright (C) 2005-2010, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "config.h"
#include <iostream>
#include <set>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>
#include <machine.hpp>
#include "spatialcomputer.h"
#include "visualizer.h"
#include "plugin_manager.h"
#include "DefaultsPlugin.h"

extern map<string,uint8_t> OPCODE_MAP;

/*****************************************************************************
 *  DEVICE                                                                   *
 *****************************************************************************/
int Device::top_uid=0; // originally, the device UIDs start at zero

Device::Device(SpatialComputer* parent, METERS *loc, DeviceTimer *timer) { 
  uid=top_uid++; this->timer = timer; this->parent = parent;
  run_time=0;  // should be reset at script-load
  body = parent->physics->new_body(this,loc[0],loc[1],loc[2]);
  // integrate w. layers, which may add devicelayers to the device
  num_layers = parent->dynamics.max_id();
  layers = (DeviceLayer**)calloc(num_layers,sizeof(DeviceLayer*));
  for(int i=0;i<num_layers;i++) 
    { Layer* l = (Layer*)parent->dynamics.get(i); if(l) l->add_device(this); }
  //vm = allocate_machine(); // unusable until script is loaded
  vm = new Machine();
  is_selected=false; is_debug=false;
  if (parent->print_stack_id == uid) {
	  is_print_stack = true;
  } else {
      is_print_stack = false;
  }
  if (parent->print_env_stack_id == uid) {
	  is_print_env_stack = true;
  } else {
	  is_print_env_stack = false;
  }
}

// copy all state
Device* Device::clone_device(METERS *loc) {
  Device* new_d = new Device(parent,loc,timer->clone_device());

  //void Device::load_script(uint8_t* script, int len) {
  //  new_machine(vm, uid, 0, 0, 0, 1, script, len);
  //uint8_t *copy_src = vm->membuf, *copy_dst = new_d->vm->membuf;
  //for(int i=0;i<vm->memlen;i++) { copy_dst[i]=copy_src[i]; }
  new_d->load_script(vm->currentScript(),vm->currentScript().size());
  // copy over state
  
  //new_d->vm->time = vm->time;  new_d->vm->last_time = vm->last_time;
  for(size_t i = 0; i < new_d->vm->threads.size(); i++) {
    new_d->vm->threads[i] = vm->threads[i];
  }
  
  new_d->run_time=run_time; new_d->is_selected=is_selected; 
  //new_d->is_debug=is_debug;
  for(int i=0;i<num_layers;i++) {
    DeviceLayer *d = layers[i], *nd = new_d->layers[i]; 
    if(d && nd) nd->copy_state(d);
  }
  return new_d;
}

Device::~Device() {
  delete timer;
  delete body;
  for(int i=0;i<num_layers;i++)
    { DeviceLayer* d = (DeviceLayer*)layers[i]; if(d) delete d; }
  free(layers);
  //deallocate_machine(&vm);
  delete vm;
}

// dump function should produce matlab-readable data at verbosity 0
// and at verbosity 1+ should make human-readable
void Device::dump_state(FILE* out, int verbosity) {
  // dump heading information
  if(verbosity==0) {
    fprintf(out,"%d %.2f %.2f",uid,1.0f/*ticks*/,vm->startTime());
  } else {
    fprintf(out,"Device %d ",uid);
    if(verbosity>=2) fprintf(out,"[in slot %d]",backptr);
    fprintf(out,"(Internal time: %.2f %.2f)\n",1.0f/*ticks*/,vm->startTime());
    if(verbosity>=2) 
      fprintf(out,"Selected = %s, Debug = %s\n",bool2str(is_selected),
              bool2str(is_debug));
  }
  // dump layers
  if(parent->physics->can_dump) body->dump_state(out,verbosity);
  for(int i=0;i<num_layers;i++) {
    DeviceLayer* d = (DeviceLayer*)layers[i]; 
    Layer* l = (Layer*)parent->dynamics.get(i);
    if(d && l->can_dump) d->dump_state(out,verbosity);
  }
  // dump output
  
  if(parent->is_dump_value) {
    char buf[1000];
    if(verbosity==0) {
      post_stripped_data_to(buf, vm->threads[0].result);
      fprintf(out," %s",buf);
    } else {
      post_data_to(buf, vm->threads[0].result);
      fprintf(out,"Output %s\n",buf);
    }
  }

  // dump network graph structure
  if(parent->is_dump_network) {
    char buf[1000];
    // First number of neighboring devices
    if(verbosity==0) { 
      fprintf(out," %i",vm->hood.size());
    } else {
      fprintf(out,"Number of neighbors: %i\n",vm->hood.size());
    }
    // Then IDs of all neighboring devices
    if(verbosity>=1) { fprintf(out,"Neighbor IDs:"); }
    for(NeighbourHood::iterator i=vm->hood.begin(); i != vm->hood.end(); i++) {
      Neighbour const & nbr = *i;
      fprintf(out," %i",nbr.id);
    }
    if(verbosity>=1) { fprintf(out,"\n"); }
  }
  
  // dump neighborhood data
  if(verbosity>=1 && parent->is_dump_hood) {
    char buf[1000];
    fprintf(out,"Export Values: "); // first export values
    for(int i=0;i<vm->thisMachine().imports.size();i++) {
      post_data_to(buf,vm->thisMachine().imports[i]); fprintf(out,"%s ",buf);
    }
    fprintf(out,"\nNeighbor Values: (%d neighbors) \n",(int)vm->hood.size()); // then neighbor data
    for(NeighbourHood::iterator i=vm->hood.begin(); i != vm->hood.end(); i++) {
      Neighbour const & nbr = *i;
      fprintf(out,
              "Neighbor %4d [X=%.2f, Y=%.2f, Z=%.2f, Range=%.2f]: ",
              nbr.id, nbr.x, nbr.y, nbr.z,
              sqrt(nbr.x*nbr.x + nbr.y*nbr.y + nbr.z*nbr.z));
	for(int j=0;j<vm->thisMachine().imports.size();j++) {
	  post_data_to(buf,nbr.imports[j]); fprintf(out,"%s ",buf);
	}
      fprintf(out,"\n");
    }
  }
  
  if(verbosity==0) fprintf(out,"\n"); // terminate line
}

void Device::load_script(uint8_t const * script, int len) {
  //new_machine(vm, uid, 0, 0, 0, 1, script, len);
  vm->id = uid;
  vm->install(Script(script,len));
  int iStep = 0;
  while(!vm->finished()) {
	 	if (is_print_stack || is_print_env_stack) {
	    	Int8 opcode = *(vm->instruction_pointer);
	    	cout << "OpCode: " << (int)opcode;
	    	Instruction i = instructions[opcode];
	   		map<string,uint8_t>::iterator it;
	   		for ( it=OPCODE_MAP.begin() ; it != OPCODE_MAP.end(); it++ ) {
	   			if ((*it).second == opcode) {
	   				cout << " " << (*it).first;
    				break;
	   			}
	   		}
	 	}
	 	if (is_print_stack) {
	 		cout << " Stack (" << iStep << "): ";
	 		vm->print_stack(&vm->stack);
	    }
	 	if (is_print_env_stack) {
	   	  cout << " Environment Stack (" << iStep << "): ";
	   	  vm->print_stack(&vm->environment);
	   	}
     	iStep++;
	    vm->step();
    }
}

// a convenient combined function
bool Device::debug() { return is_debug && parent->is_debug; }

// scale the display to draw text labels for a device
#define TEXT_SCALE 3.75 // arbitrary constant for text sizing
void Device::text_scale() {
#ifdef WANT_GLUT
  flo d = body->display_radius(); glScalef(d,d,d); // scale text to body
  d = vis_context->display_mag; glScalef(d,d,d); // then magnify as specified
  glScalef(TEXT_SCALE,TEXT_SCALE,TEXT_SCALE);
#endif // WANT_GLUT
}

extern void radio_send_export(uint8_t version, Array<Data> const & data);

void Device::internal_event(SECONDS time, DeviceEvent type) {
  int iStep = 0;
  switch(type) {
  case COMPUTE:
    body->preupdate(); // run the pre-compute update
    for(int i=0;i<num_layers;i++)
      { DeviceLayer* d = (DeviceLayer*)layers[i]; if(d) d->preupdate(); }
    {
      vm->thisMachine().data_age = 0;
      for( NeighbourHood::iterator i = vm->hood.begin(); i != vm->hood.end(); ){
        if (i->data_age > 1) {
          i = vm->hood.remove(i);
        } else {
          i->data_age++;
          ++i;
        }
      }
    }

    // double-delay kludge option: just run the VM a second time
    for(int vmrun=0;vmrun<=(2*parent->is_double_delay_kludge);vmrun++) {
      vm->run(time);
      while(!vm->finished()) {
    	if (is_print_stack || is_print_env_stack) {
          Int8 opcode = *(vm->instruction_pointer);
          cout << "OpCode: " << (int)opcode;
          Instruction i = instructions[opcode];
          map<string,uint8_t>::iterator it;
          for ( it=OPCODE_MAP.begin() ; it != OPCODE_MAP.end(); it++ ) {
            if ((*it).second == opcode) {
              cout << " " << (*it).first;
              break;
            }
          }
    	}
    	if (is_print_stack) {
    	  cout << " Stack (" << iStep << "): ";
    	  vm->print_stack(&vm->stack);
    	}
    	if (is_print_env_stack) {
    	  cout << " Environment Stack (" << iStep << "): ";
    	  vm->print_stack(&vm->environment);
    	}
    	iStep++;
    	vm->step();
      }
      if (is_print_stack || is_print_env_stack) {
    	cout << endl;
      }
    }

    body->update(); // run the post-compute update
    for(int i=0;i<num_layers;i++)
      { DeviceLayer* d = (DeviceLayer*)layers[i]; if(d) d->update(); }
    break;
  case BROADCAST:
    //export_machine();
    /*if(script_export_needed() || (((int)vm->ticks) % 10)==0) {
      export_script(); // send script every 10 rounds, or as needed
    }*/
    radio_send_export(0,vm->thisMachine().imports);
    break;
  }
}

bool Device::handle_key(KeyEvent* key) {
  for(int i=0;i<num_layers;i++) {
    DeviceLayer* d = (DeviceLayer*)layers[i]; 
    if(d && d->handle_key(key)) return true;
  }
  return false;
}

void Device::visualize() {
#ifdef WANT_GLUT
  glPushMatrix();
  // center on device
  const flo* p = body->position(); glTranslatef(p[0],p[1],p[2]);
  // draw the body & other dynamics layers
  body->visualize();
  for(int i=0;i<num_layers;i++)
    { DeviceLayer* d = (DeviceLayer*)layers[i]; if(d) d->visualize(); }
  
  if(is_selected) {
    palette->use_color(SpatialComputer::DEVICE_SELECTED);
    draw_circle(4*body->display_radius());
  }
  if(debug()) {
    palette->use_color(SpatialComputer::DEVICE_DEBUG);
    glPushMatrix();
    glTranslatef(0,0,-0.1);
    draw_disk(2*body->display_radius());
    glPopMatrix();
  }

  if (vis_context->is_show_vec) {
    Data dst = vm->threads[0].result;
    glPushMatrix();
    glLineWidth(4);
    glTranslatef(0, 0, 0);
    switch (dst.type()) {
    case Data::Type_tuple: {
      Tuple const & v = dst.asTuple();
      if (v.size() >= 2) {
        flo x = v[0].asNumber();
        flo y = v[1].asNumber();
        flo z = v.size() > 2 ? v[2].asNumber() : 0;
	palette->use_color(SpatialComputer::VECTOR_BODY);
        glBegin(GL_LINE_STRIP);
        glVertex3f(0, 0, 0);
        glVertex3f(0.8*x, 0.8*y, 0.8*z);
        glEnd();
	palette->use_color(SpatialComputer::VECTOR_TIP);
        glBegin(GL_LINE_STRIP);
        glVertex3f(0.8*x, 0.8*y, 0.8*z);
        glVertex3f(x, y, z);
        glEnd();
      }
      break; }
    }
    glLineWidth(1);
    glPopMatrix();
  }
  
  text_scale(); // prepare to draw text
  char buf[1024];
  if (vis_context->is_show_id) {
    palette->use_color(SpatialComputer::DEVICE_ID);
    sprintf(buf, "%2d", uid);
    draw_text(1, 1, buf);
  }

  if(vis_context->is_show_val) {
    Data dst = vm->threads[0].result;
    glPushMatrix();
    //glTranslatef(0, 0, 0);
    post_data_to(buf, dst);
    palette->use_color(SpatialComputer::DEVICE_VALUE);
    draw_text(1, 1, buf);
    glPopMatrix();
  }
  
  if(vis_context->is_show_version) {
    glPushMatrix();
    palette->use_color(SpatialComputer::DEVICE_ID);
    //sprintf(buf, "%2d:%s", vm->scripts[vm->cur_script].version,
	//    (vm->scripts[vm->cur_script].is_complete)?"OK":"wait");
	strcpy(buf, "0:OK");
    draw_text(4, 4, buf);
    glPopMatrix();
  }
  glPopMatrix();
#endif // WANT_GLUT
}

// Special render for OpenGL selection mode
void Device::render_selection() {
#ifdef WANT_GLUT
  glPushMatrix();
  // center on device
  const flo* p = body->position(); glTranslatef(p[0],p[1],p[2]);
  glLoadName(backptr); // set the name
  body->render_selection(); // draw the body
  glPopMatrix();
#endif // WANT_GLUT
}  

/*****************************************************************************
 *  SIMULATED DEVICE                                                         *
 *****************************************************************************/
class SimulatedDevice : public Device {
public:
  SimulatedDevice(SpatialComputer* parent, METERS* loc, DeviceTimer *timer) :
    Device(parent,loc,timer) {}
  void dump_state(FILE* out, int verbosity) {
    if(verbosity>=1) fprintf(out,"(Simulated) ");
    Device::dump_state(out,verbosity);
  }
};



/*****************************************************************************
 *  SPATIAL COMPUTER                                                         *
 *****************************************************************************/
// get the spatial layout of the computer, including node distribution
void SpatialComputer::get_volume(Args* args, int n) {
  // spatial layout
  int dimensions=2; // default to 2D world
  METERS width=132, height=100, depth=0; // sides of the network distribution
  if(args->extract_switch("-3d")) { depth=40; dimensions=3; }
  if(args->extract_switch("-dim")) { // dim X [Y [Z]]
    width=args->pop_number();
    if(str_is_number(args->peek_next())) {
      height=args->pop_number();
      if(str_is_number(args->peek_next())) { 
        depth=args->pop_number(); dimensions=3;
      }
    }
  }
  if(dimensions==2)
    vis_volume=new Rect(-width/2, width/2, -height/2, height/2);
  else
    vis_volume=new Rect3(-width/2,width/2,-height/2,height/2,-depth/2,depth/2);

  // Now get the distribution volume (defaults to same as visual)
  volume = vis_volume->clone();
  if(args->extract_switch("-dist-dim")) {
    volume->l = args->pop_number();
    volume->r = args->pop_number();
    volume->b = args->pop_number();
    volume->t = args->pop_number();
    if(volume->dimensions()==3) {
      ((Rect3*)volume)->f = args->pop_number();
      ((Rect3*)volume)->c = args->pop_number();
    }
  }
}

// add the layer to dynamics and set its ID, for use in hardware callbacks
int SpatialComputer::addLayer(Layer* layer) {
  return (layer->id = dynamics.add(layer));
}

Color *SpatialComputer::BACKGROUND, *SpatialComputer::PHOTO_FLASH,
  *SpatialComputer::DEVICE_SELECTED, *SpatialComputer::DEVICE_DEBUG,
  *SpatialComputer::DEVICE_ID, *SpatialComputer::DEVICE_VALUE, 
  *SpatialComputer::VECTOR_BODY, *SpatialComputer::VECTOR_TIP;
void SpatialComputer::register_colors() {
#ifdef WANT_GLUT
  BACKGROUND = palette->lookup_color("BACKGROUND"); // from visualizer
  PHOTO_FLASH = palette->register_color("PHOTO_FLASH", 1, 1, 1, 1);
  DEVICE_SELECTED = palette->register_color("DEVICE_SELECTED", 0.5,0.5,0.5,0.8);
  DEVICE_ID = palette->register_color("DEVICE_ID", 1, 0, 0, 0.8);
  DEVICE_VALUE = palette->register_color("DEVICE_VALUE", 0.5, 0.5, 1, 0.8);
  VECTOR_BODY = palette->register_color("VECTOR_BODY", 0, 0, 1, 0.8);
  VECTOR_TIP = palette->register_color("VECTOR_TIP", 1, 0, 1, 0.8);
  DEVICE_DEBUG = palette->register_color("DEVICE_DEBUG", 1, 0.8, 0.8, 0.5);
#endif
}

SpatialComputer::SpatialComputer(Args* args, bool own_dump) {
  ensure_colors_registered("SpatialComputer");
  sim_time=0;
  is_double_delay_kludge = !(args->extract_switch("--no-double-delay-kludge"));

  print_stack_id = (args->extract_switch("-print-stack"))?args->pop_number() : -1;
  print_env_stack_id = (args->extract_switch("-print-env-stack"))?args->pop_number() : -1;

  int n=(args->extract_switch("-n"))?(int)args->pop_number():100; // # devices
  // load dumping variables
  is_dump_default=true;
  args->undefault(&is_dump_default,"-Dall","-NDall");
  is_dump_hood=is_dump_default;
  args->undefault(&is_dump_hood,"-Dhood","-NDhood");
  is_dump_value=is_dump_default;
  args->undefault(&is_dump_value,"-Dvalue","-NDvalue");
  is_dump_network=false; // dumping the network structure is not standard
  args->undefault(&is_dump_network,"-Dnetwork","-NDnetwork");
  is_dump = args->extract_switch("-D");
  is_probe_filter = args->extract_switch("-probe-dump-filter");
  is_show_snaps = !args->extract_switch("-no-dump-snaps");
  dump_start = args->extract_switch("-dump-after") ? args->pop_number() : 0;
  dump_period = args->extract_switch("-dump-period") ? args->pop_number() : 1;
  is_own_dump_file=own_dump; // create dump files unless told otherwise
  if(own_dump) {
    dump_dir = args->extract_switch("-dump-dir") ? args->pop_next() : "dumps";
    dump_stem = args->extract_switch("-dump-stem") ? args->pop_next() : "dump";
  }
  just_dumped=false; next_dump = dump_start; snap_vis_time=0;
  // setup customization
  get_volume(args, n);
  initialize_plugins(args, n);

  scheduler = new Scheduler(n, time_model->cycle_time());
  // create the actual devices
  METERS loc[3];
  for(int i=0;i<n;i++) {
    if(distribution->next_location(loc)) {
      SECONDS start;
      Device* d = new SimulatedDevice(this,loc,time_model->next_timer(&start));
      d->backptr = devices.add(d);
      scheduler->schedule_event((void*)d->backptr,start,0,COMPUTE,d->uid);
    }
  }
  
  // load display variables
  display_mag = (args->extract_switch("-mag"))?args->pop_number():1;
  is_show_val = args->extract_switch("-v");
  is_show_vec = args->extract_switch("-sv");
  is_show_id = args->extract_switch("-i");
  is_show_version = args->extract_switch("-show-script-version");
  is_debug = args->extract_switch("-g");
  hardware.is_kernel_trace = args->extract_switch("-t");
  hardware.is_kernel_debug = args->extract_switch("-debug-kernel");
  hardware.is_kernel_debug_script = args->extract_switch("-debug-script");
}

SpatialComputer::~SpatialComputer() {
  // delete devices first, because their "death" needs dynamics to still exist
  for(int i=0;i<devices.max_id();i++)
    { Device* d = (Device*)devices.get(i); if(d) delete d; }
  // delete everything else in arbitrary order
  delete scheduler; delete volume; delete time_model; delete distribution;
  for(int i=0;i<dynamics.max_id();i++) 
    { Layer* ec = (Layer*)dynamics.get(i); if(ec) delete ec; }
}

/*****************************************************************************
 *  SPATIAL COMPUTER: PLUGINS                                                *
 *****************************************************************************/
bool checkIfLayerImplementsUnpatchedFunctions(vector<HardwareFunction>& unpatchedFuncs, vector<HardwareFunction>& layerFuncs) {
  bool ret = false;
  for(int i = 0; i < layerFuncs.size(); i++) {
    HardwareFunction f = layerFuncs[i];
    vector<HardwareFunction>::iterator it;
    it = find(unpatchedFuncs.begin(), unpatchedFuncs.end(), f);
    if(it != unpatchedFuncs.end()) {
      //          cout << "given layer implements func: " << f << endl;
      ret = true;
    }
  }
  return ret;
}

vector<HardwareFunction> getUnpatchedFuncs(SpatialComputer& sc) {
  vector<HardwareFunction> unpatchedFuncs; // core fns that must be patched

  for(int i = 0; i < sc.hardware.requiredPatches.size(); i++) {
    HardwareFunction unpatched = sc.hardware.requiredPatches[i];
    // cout << "i = " << i << " : " << hardware.patch_table[unpatched] << endl;
    if(sc.hardware.patch_table[unpatched] == &(sc.hardware.base)) {
      //cout << "Unpatched OPCODE " << unpatched << endl;
      unpatchedFuncs.push_back(unpatched);
    }
  }
  return unpatchedFuncs;
}

int SpatialComputer::addLayer(const char* layer,Args* args,int n) {
  HardwarePatch* old_mov = hardware.patch_table[MOV_FN]; 
  Layer* l = (Layer*)plugins.get_sim_plugin(LAYER_PLUGIN,layer,args,this,n);
  if(l==NULL) uerror("Could not obtain simulator layer '%s'",layer);
  if(old_mov != hardware.patch_table[MOV_FN]) { // physics -> MOV_FN changed
    if(physics!=NULL) uerror("Cannot install second physics plugin: %s",layer);
    physics=(BodyDynamics*)l; return -1;
  } else { 
    return addLayer(l);
  }
}

// String constants for default layers:
const char* DebugLayerID = "DebugLayer";
const char* PerfectLocalizerID = "PerfectLocalizer";
const char* SimpleDynamicsID = "SimpleDynamics";
const char* UnitDiscRadioID = "UnitDiscRadio";

void SpatialComputer::initialize_plugins(Args* args, int n) {
  DefaultsPlugin::register_defaults(); // make sure defaults exist
  
  const char* s;
  // If we fail to get the required plugins, we abort with an error
  // Obtain the time model
  s = (args->extract_switch("-TM"))?args->pop_next():"FixedIntervalTime";
  time_model=(TimeModel*)plugins.get_sim_plugin(TIMEMODEL_PLUGIN,s,args,this,n);
  if(time_model==NULL) uerror("Could not obtain time model '%s'",s);

  // Obtain the distribution
  s=(args->extract_switch("-DD"))?args->pop_next():"UniformRandom";
  distribution =
    (Distribution*)plugins.get_sim_plugin(DISTRIBUTION_PLUGIN,s,args,this,n);
  if(distribution==NULL) uerror("Could not obtain distribution '%s'",s);
  
  // Obtain the layers
  physics = NULL; // make sure we start w. the physics known missing
  set<string> layers;
  while(args->extract_switch("-L",false)) {
    char* layer = args->pop_next();
    if(layers.count(string(layer))) 
      post("WARNING: ignoring duplicate request for layer '%s'",layer);
    addLayer(layer,args,n); layers.insert(layer);
  }
  // simulator always uses DebugLayer:
  if(!layers.count(DebugLayerID)) { addLayer(DebugLayerID,args,n); }
  
  // Find unpatched universal sensing & actuation ops, add defaults
  vector<HardwareFunction> unpatchedFuncs = getUnpatchedFuncs(*this);
  
  vector<HardwareFunction> plfuncs = PerfectLocalizer::getImplementedHardwareFunctions();
  bool implementsUnpatchedFunctions = checkIfLayerImplementsUnpatchedFunctions(unpatchedFuncs, plfuncs);
  if(implementsUnpatchedFunctions) { addLayer(PerfectLocalizerID,args,n); }

  if(!physics) addLayer(SimpleDynamicsID,args,n);
  //if(!physics) physics = new SimpleDynamics(args,this,n);

  vector<HardwareFunction> udrfuncs = UnitDiscRadio::getImplementedHardwareFunctions();
  implementsUnpatchedFunctions = checkIfLayerImplementsUnpatchedFunctions(unpatchedFuncs, udrfuncs);
  if(implementsUnpatchedFunctions) { addLayer(UnitDiscRadioID,args,n); }

  unpatchedFuncs = getUnpatchedFuncs(*this);
  //if(unpatchedFuncs.size()) 
  //  uerror("Simulator cannot load: missing required hardware functions.");
  // TODO: deal with missing hardware question
}


/*****************************************************************************
 *  SPATIAL COMPUTER: OPERATION                                              *
 *****************************************************************************/
// for the initial loading only
void SpatialComputer::load_script(uint8_t* script, int len) {
  for(int i=0;i<devices.max_id();i++) { 
    Device* d = (Device*)devices.get(i); 
    if(d) {
      hardware.set_vm_context(d);
      d->load_script(script,len); 
    }
  }
}
// install a script by injecting it as packets w. the next version
void SpatialComputer::load_script_at_selection(uint8_t* script, int len) {
	/*
  for(int i=0;i<selection.max_id();i++) {
    Device* d = (Device*)devices.get((long)selection.get(i));
    if(d) {
      hardware.set_vm_context(d);
      version++;
      for (i=0; i < (int)floor(len  / (float)MAX_SCRIPT_PKT); i++) {
	radio_receive_script_pkt(version, len, i, &script[i*MAX_SCRIPT_PKT]);
      }
      if (len % MAX_SCRIPT_PKT) {
	radio_receive_script_pkt(version, len, i, &script[i*MAX_SCRIPT_PKT]);
      }
    }
  }
  */
}

bool SpatialComputer::handle_key(KeyEvent* key) {
  // is this a key recognized internally?
  if(key->normal && !key->ctrl) {
    switch(key->key) {
    case 'i': is_show_id = !is_show_id; return true;
    case 'v': is_show_vec = !is_show_vec; return true;
    case 'n': is_show_val = !is_show_val; return true;
    case 'j': is_show_version = !is_show_version; return true;
    case 'U': selection.clear(); update_selection(); return true;
    case 'a': hardware.is_kernel_trace = !hardware.is_kernel_trace; return true;
    case 'd': is_debug = !is_debug; return true;
    case 'D':
      for(int i=0;i<selection.max_id();i++) {
        Device* d = (Device*)devices.get((long)selection.get(i));
        if(d) d->is_debug = !d->is_debug;
      }
      return true;
    case '8': is_probe_filter=true; is_dump = true; return true;
    case '9': is_probe_filter=false; is_dump = true; return true;
    case '0': is_dump = false; return true;
    case 'Z': dump_frame(sim_time,true); return true;
    break;
    }
  }
  // is this key recognized by the selected nodes?
  // try it in the various sensor/actuator layers
  if(physics->handle_key(key)) return true;
  for(int i=0;i<dynamics.max_id();i++) {
    Layer* d = (Layer*)dynamics.get(i);
    if(d && d->handle_key(key)) return true;
  }
  bool in_selection = false;
  for(int i=0;i<selection.max_id();i++) {
    Device* d = (Device*)devices.get((long)selection.get(i));
    if(d) in_selection |= d->handle_key(key);
  }
  return false;
}

bool SpatialComputer::handle_mouse(MouseEvent* mouse) {
  return false;
}

SpatialComputer* vis_context;
extern double get_real_secs ();
#define FLASH_TIME 0.1 // time that a snap flashes the background
void SpatialComputer::visualize() {
#ifdef WANT_GLUT
  vis_context=this;
  physics->visualize();
  for(int i=0;i<dynamics.max_id();i++)
    { Layer* d = (Layer*)dynamics.get(i); if(d) d->visualize(); }
  for(int i=0;i<devices.max_id();i++)
    { Device* d = (Device*)devices.get(i); if(d) d->visualize(); }
  // show "photo flashes" when dumps have occured
  SECONDS time = get_real_secs();
  if(just_dumped) { just_dumped=false; snap_vis_time = time; }
  bool flash = is_show_snaps && (time-snap_vis_time) < FLASH_TIME;
  palette->set_background(flash ? PHOTO_FLASH : BACKGROUND);
#endif // WANT_GLUT
}

// special render for OpenGL selecting mode
void SpatialComputer::render_selection() {
  vis_context=this;
  for(int i=0;i<devices.max_id();i++)
    { Device* d = (Device*)devices.get(i); if(d) d->render_selection(); }
}
void SpatialComputer::update_selection() {
  for(int i=0;i<devices.max_id();i++) // clear old selection bits
    { Device* d = (Device*)devices.get(i); if(d) d->is_selected=false; }
  for(int i=0;i<selection.max_id();i++) { // set new selection bits
    int n = (long)selection.get(i);
    Device* d = (Device*)devices.get(n); 
    if(d) d->is_selected=true;
  }
}
void SpatialComputer::drag_selection(flo* delta) {
  if(volume->dimensions()==2) { // project back onto the surface
    delta[2]=0;
  }
  if(delta[0]==0 && delta[1]==0 && delta[2]==0) return;
  for(int i=0;i<selection.max_id();i++) { // move each selected device
    int n = (long)selection.get(i);
    Device* d = (Device*)devices.get(n); 
    if(d) { 
      const flo* p = d->body->position(); // calc new position
      d->body->set_position(p[0]+delta[0],p[1]+delta[1],p[2]+delta[2]);
      for(int j=0;j<dynamics.max_id();j++) // move the device
        { Layer* dyn = (Layer*)dynamics.get(j); if(dyn) dyn->device_moved(d); }
    }
  }
}

bool SpatialComputer::evolve(SECONDS limit) {
  SECONDS dt = limit-sim_time;
  // evolve world
  physics->evolve(dt);
  for(int i=0;i<devices.max_id();i++) { // tell layers about moving devices
    Device* d = (Device*)devices.get(i);
    if(d && d->body->moved) {
      for(int j=0;j<dynamics.max_id();j++) 
        { Layer* dyn = (Layer*)dynamics.get(j); if(dyn) dyn->device_moved(d); }
      d->body->moved=false;
    }
  }
  // evolve other layers
  for(int i=0;i<dynamics.max_id();i++) {
    Layer* d = (Layer*)dynamics.get(i);
    if(d) d->evolve(dt);
  }
  // evolve devices
  Event e; scheduler->set_bound(limit);
  while(scheduler->pop_next_event(&e)) {
    int id = (long)e.target;
    Device* d = (Device*)devices.get(id);
    if(d && d->uid==e.uid) {
      sim_time=e.true_time; // set time to new value
      hardware.set_vm_context(d); // align kernel/sim patch for this device
      d->internal_event(e.internal_time,(DeviceEvent)e.type);
      if(e.type==COMPUTE) {
        d->run_time = e.internal_time;
        SECONDS tt, it;  // true and internal time
        d->timer->next_compute(&tt,&it); tt+=sim_time; it+=d->run_time;
	scheduler->schedule_event((void*)id,tt,it,COMPUTE,d->uid);
        d->timer->next_transmit(&tt,&it); tt+=sim_time; it+=d->run_time;
        scheduler->schedule_event((void*)id,tt,it,BROADCAST,d->uid);
      }
    }
  }
  sim_time=limit;
  
  // clone or kill devices (at end of update period)
  while(!death_q.empty()) {
    int id = death_q.front(); death_q.pop(); // get next to kill
    Device* d = (Device*)devices.get(id);
    if(d) {
      // scheduled events for dead devices are ignored; need not be deleted
      if(d->is_selected) { // fix selection (if needed)
        for(int i=0;i<selection.max_id();i++) {
          int n = (long)selection.get(i);
          if(n==id) selection.remove(i);
        }
      }
      delete d;
      devices.remove(id);
    }
  }
  while(!clone_q.empty()) {
    CloneReq* cr = clone_q.front(); clone_q.pop(); // get next to clone
    Device* d = (Device*)devices.get(cr->id);
    if(d && d==cr->parent) { // check device: might have been deleted
      Device* new_d = d->clone_device(cr->child_pos);
      new_d->backptr = devices.add(new_d);
      if(new_d->is_selected) { selection.add((void*)new_d->backptr); }
      // schedule next event
      SECONDS tt, it;  // true and internal time
      new_d->timer->next_compute(&tt,&it); tt+=sim_time; it+=new_d->run_time;
      scheduler->schedule_event((void*)new_d->backptr,tt,it,COMPUTE,
                                new_d->uid);
    }
    delete cr;
  }
  
  // dump if needed
  if(is_dump && sim_time >= dump_start && sim_time >= next_dump) {
    dump_frame(next_dump,false);
    while(next_dump <= sim_time) next_dump+=dump_period;
  }
  
  return true;
}

/*****************************************************************************
 *  DUMPING FACILITY                                                         *
 *****************************************************************************/

void SpatialComputer::dump_selection(FILE* out, int verbosity) {
  for(int i=0;i<selection.max_id();i++) {
    int n = (long)selection.get(i);
    Device* d = (Device*)devices.get(n); if(d) d->dump_state(out,verbosity);
  }
}

void SpatialComputer::dump_state(FILE* out) {
  for(int i=0;i<devices.max_id();i++)
    { Device* d = (Device*)devices.get(i); if(d) d->dump_state(out,0); }
}

void SpatialComputer::dump_header(FILE* out) {
  fprintf(out,"%% \"UID\" \"TICKS\" \"TIME\""); // device fields
  physics->dump_header(out);
  for(int i=0;i<dynamics.max_id();i++)
    { Layer* d = (Layer*)dynamics.get(i); if(d) d->dump_header(out); }
  if(is_dump_value) fprintf(out," \"OUT\"");
  if(is_dump_network) fprintf(out," \"NUM NBRS\" \"NBR IDs (variable)\"");
  fprintf(out,"\n");
}

void SpatialComputer::dump_frame(SECONDS time, bool time_in_name) {
  if(is_own_dump_file) { // manage the file ourselves
    char buf[1000];
#ifdef _WIN32  
    if(mkdir(dump_dir) != 0) {
#else
    if(mkdir(dump_dir, ACCESSPERMS) != 0) {
#endif
      //ignore
    }
    // open the file
    if(time_in_name) {
      sprintf(buf,"%s/%s%.2f-%.2f.log",dump_dir,dump_stem,get_real_secs(),time);
    } else {
      sprintf(buf,"%s/%s%.2f.log",dump_dir,dump_stem,time);
    }
    dump_file = fopen(buf,"w");
    if(dump_file==NULL) { post("Unable to open dump file '%s'\n",buf); return; }
  } else {
    if(dump_file==NULL) {post("Can't dump: no output file supplied\n"); return;}
  }
  // output all the state
  dump_header(dump_file); dump_state(dump_file);
  if(is_own_dump_file) {
    fclose(dump_file); // close the file
  }
  just_dumped = true; // prime drawing to flash
}

void SpatialComputer::appendDefops(string& s) {
  hardware.appendDefops(s);
}
