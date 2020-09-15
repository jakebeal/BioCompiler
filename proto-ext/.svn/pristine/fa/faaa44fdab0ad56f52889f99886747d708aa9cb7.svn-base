/* Newtonian physics simulation using the Open Dynamics Engine
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */


#include "odedynamics.h"


/*****************************************************************************
 *  COLLISION HANDLING                                                       *
 *****************************************************************************/

static int isWall (dGeomID g) { return dGeomGetData(g)==((void*)WALL_DATA); }
static int isRay (dGeomID g) { return dGeomGetData(g)==((void*)RAY_DATA); }


//Create test rays
bool rayAdded = false;
dGeomID rayId;

static void nearCallback (void *data, dGeomID o1, dGeomID o2) {
  int i, numc;
  ODEDynamics *dyn = (ODEDynamics*)data;



  // exit without doing anything if the two bodies are connected by a joint
  dBodyID b1 = dGeomGetBody(o1); dBodyID b2 = dGeomGetBody(o2);
  if (b1 && b2 && dAreConnectedExcluding(b1,b2,dJointTypeContact)) return;
  // exit without doing anything if the walls are off and one object is a wall
  if(!dyn->is_walls && (isWall(o1) || isWall(o2))) { return; }

  dContact contact[MAX_CONTACTS];   // up to MAX_CONTACTS contacts per pair
  if (int numc = dCollide (o1,o2,MAX_CONTACTS,&contact[0].geom, sizeof(dContact))) {
    for (i=0; i<numc; i++) {
      contact[i].surface.mode = dContactBounce | dContactSoftCFM;
//    	contact[i].surface.mode = dContactBounce;
//    	contact[i].surface.mu = 0;
//      contact[i].surface.mu = dInfinity;
      contact[i].surface.mu = 30;
//      contact[i].surface.mu = 0.85;
      contact[i].surface.mu2 = 0.85	;
      contact[i].surface.bounce = 0.1;
      //contact[i].surface.bounce = 0.5;
      contact[i].surface.bounce_vel = 0.1;
      contact[i].surface.bounce_vel = 0.9;
      contact[i].surface.soft_cfm = 0.001; // give
      dJointID c=dJointCreateContact(dyn->world,dyn->contactgroup,&contact[i]);
      dJointAttach(c,b1,b2);
    }
    // record bumps
    if(b1 && (b2 || isWall(o2))) ((ODEBody*)dBodyGetData(b1))->did_bump=TRUE;
    if(b2 && (b1 || isWall(o1))) ((ODEBody*)dBodyGetData(b2))->did_bump=TRUE;
    if(b1 && (isRay(o2))) ((ODEBody*)dBodyGetData(b1))->did_bump=TRUE;
    if(b2 && (isRay(o1))) ((ODEBody*)dBodyGetData(b2))->did_bump=TRUE;
//
//    if( dCollide (rayId,o1,MAX_CONTACTS,&contact[0].geom, sizeof(dContact)) ){
//    	 ((ODEBody*)dBodyGetData(b1))->did_bump=TRUE;
//    }
  }
}
/*****************************************************************************
 *  ODE DYNAMICS                                                             *
 *****************************************************************************/

#define DENSITY (0.125/2)       // default density
#define MAX_V 100               // default ceiling on velocity
#define SUBSTEP 0.001           // default sub-step size
#define K_BODY_RAD 0.0870 // constant matched against previous visualization
#define GRAVITY -9.81
void ODEDynamics::make_walls() {
  flo pen_w=parent->volume->r - parent->volume->l;
  flo pen_h=parent->volume->t - parent->volume->b;
  flo wall_width = 5;
  dQuaternion Q;
  
  pen = dCreateBox(0,pen_w+2*wall_width,pen_h+2*wall_width,10*pen_h);
  
  // side walls
  dQFromAxisAndAngle (Q,0,0,1,M_PI/2);
  walls[0] = dCreateBox(space, pen_h+2*wall_width, wall_width, 10*pen_h);
  dGeomSetQuaternion (walls[0],Q);
  dGeomSetPosition (walls[0], pen_w/2+wall_width/2, 0, 0);
  dGeomSetData(walls[0], (void*)WALL_DATA);
  
  walls[1] = dCreateBox(space, pen_h+2*wall_width, wall_width, 10*pen_h);
  dGeomSetQuaternion (walls[1],Q);
  dGeomSetPosition (walls[1], -pen_w/2-wall_width/2, 0, 0);
  dGeomSetData(walls[1], (void*)WALL_DATA);
  
  dQFromAxisAndAngle (Q,0,0,1,0);
  walls[2] = dCreateBox(space, pen_w+2*wall_width, wall_width, 10*pen_h);
  dGeomSetQuaternion (walls[2],Q);
  dGeomSetPosition (walls[2], 0, pen_h/2+wall_width/2, 0);
  dGeomSetData(walls[2], (void*)WALL_DATA);
  
  walls[3] = dCreateBox(space, pen_w+2*wall_width, wall_width, 10*pen_h);
  dGeomSetQuaternion (walls[3],Q);
  dGeomSetPosition (walls[3], 0, -pen_h/2-wall_width/2, 0);
  dGeomSetData(walls[3], (void*)WALL_DATA);

  // corner walls
  dQFromAxisAndAngle (Q,0,0,1,M_PI/4);

  walls[4] = dCreateBox(space, 2*wall_width, 2*wall_width, 10*pen_h);
  dGeomSetQuaternion (walls[4],Q);
  dGeomSetPosition (walls[4], -pen_w/2, -pen_h/2, 0);
  dGeomSetData(walls[4], (void*)WALL_DATA);

  walls[5] = dCreateBox(space, 2*wall_width, 2*wall_width, 10*pen_h);
  dGeomSetQuaternion (walls[5],Q);
  dGeomSetPosition (walls[5],  pen_w/2, -pen_h/2, 0);
  dGeomSetData(walls[5], (void*)WALL_DATA);

  walls[6] = dCreateBox(space, 2*wall_width, 2*wall_width, 10*pen_h);
  dGeomSetQuaternion (walls[6],Q);
  dGeomSetPosition (walls[6],  pen_w/2,  pen_h/2, 0);
  dGeomSetData(walls[6], (void*)WALL_DATA);

  walls[7] = dCreateBox(space, 2*wall_width, 2*wall_width, 10*pen_h);
  dGeomSetQuaternion (walls[7],Q);
  dGeomSetPosition (walls[7], -pen_w/2,  pen_h/2, 0);
  dGeomSetData(walls[7], (void*)WALL_DATA);
}

// Note: ODE's manual claims that dCloseODE is optional
BOOL ODEDynamics::inited = FALSE;
ODEDynamics::ODEDynamics(Args* args, SpatialComputer* p, int n)
  : BodyDynamics(p) {
	  post("ODE Dynamics online!\n");
  if(!inited) {   dInitODE2(0);
				  dAllocateODEDataForThread(dAllocateMaskAll);
				  inited=TRUE; } // initialize if not yet done so



  time_slop=0;
  flo width=parent->volume->r - parent->volume->l;
  flo height=parent->volume->t - parent->volume->b;
  // read options
  body_radius = (args->extract_switch("-rad")) ? args->pop_number()
    : K_BODY_RAD*sqrt((width*height)/(flo)n);
  density = (args->extract_switch("-density")) ? args->pop_number() : DENSITY;
  speed_lim = (args->extract_switch("-S"))?args->pop_number():MAX_V;
  substep = (args->extract_switch("-substep"))?args->pop_number() : SUBSTEP;
  if(substep <= 0) { 
    post("-substep must be greater than zero; using default %f",SUBSTEP);
    substep = SUBSTEP;
  }
  is_mobile = args->extract_switch("-m");
  is_show_bot = !args->extract_switch("-hide-body");
  is_walls = ((args->extract_switch("-w") & !args->extract_switch("-nw")))
    ? TRUE : FALSE;
  is_draw_walls = args->extract_switch("-draw-walls");
  is_inescapable = args->extract_switch("-inescapable");
  is_multicolored_bots = args->extract_switch("-rainbow-bots");
  is_hard_floor = args->extract_switch("-floor"); // equiv to "stalk"
  const char* xml_body_file = ( args->extract_switch("-body") ) ? args->pop_next() : "";

  gravity = (args->extract_switch("-gravity"))?args->pop_number() : GRAVITY;

  args->undefault(&can_dump,"-Ddynamics","-NDdynamics");
  // register to simulate hardware
  parent->hardware.patch(this,MOV_FN);
  parent->hardware.registerOpcode(new OpHandler<ODEDynamics>(this, &ODEDynamics::bump_op, BUMP_OP ));
  parent->hardware.registerOpcode(new OpHandler<ODEDynamics>(this, &ODEDynamics::force_op, FORCE_OP ));
  parent->hardware.registerOpcode(new OpHandler<ODEDynamics>(this, &ODEDynamics::torque_op, TORQUE_OP ));

  // Initialize ODE and make the walls
  world = dWorldCreate();
  space = dHashSpaceCreate(0);
  contactgroup = dJointGroupCreate(0);
//  if(p->volume->dimensions()==2)
//	  dWorldSetGravity(world,0,0,-9.81);

  dWorldSetGravity(world,0,0,gravity);
  dWorldSetCFM(world,1e-9);
  dWorldSetERP(world, 0.8);
  dWorldSetAutoDisableFlag(world,0); // nothing ever disables

  dWorldSetAutoDisableAverageSamplesCount(world, 10);
  dWorldSetContactMaxCorrectingVel(world,0.1);
  dWorldSetContactSurfaceLayer(world,0.01);
//  dWorldSetDamping (world, 0.02, 0.02);
   is_2d = (p->volume->dimensions()==2);
//   is_2d = false;
//   if(p->volume->dimensions()==2){
//  	  dCreatePlane(space,0,0,1,-body_radius);
//   }
   if(is_hard_floor){
	   dCreatePlane(space,0,0,1,0);
   }
  make_walls();
  //TODO I don't think this should be called from here
  register_colors();
//  xml_body_file = "test.xml";
  if(strcmp(xml_body_file, "") == 0){
	  bodyFactory = NULL;
  }else{
    bodyFactory = new ODEBodyFactory(xml_body_file,this);

	  if(bodyFactory->numBodies() != n){
		  printf("Warning! Number of bodies specified with switch \'-n\' does not match number of bodies specified in %s configuration file.\n Please use \'-n %d\'\n ",xml_body_file, bodyFactory->numBodies());
		  throw "Bad number of bodies";
	  }
  }

}

ODEDynamics::~ODEDynamics() {
  dJointGroupDestroy(contactgroup);
  for(int i=0;i<ODE_N_WALLS;i++) dGeomDestroy(walls[i]);
  dGeomDestroy(pen);
  dSpaceDestroy(space);
  dWorldDestroy(world);
}

void ODEDynamics::bump_op(MACHINE* machine) {
  NUM_PUSH(read_bump());
}

void ODEDynamics::force_op(MACHINE* machine){

	ODEBody* b = (ODEBody*)device->body;
	dReal fz =  NUM_POP();
	dReal fy = NUM_POP();
	dReal fx = NUM_POP();
	dBodyAddRelForce( b->body, fx, fy, fz );

	NUM_PUSH(true);

}


void ODEDynamics::torque_op(MACHINE* machine){

	  ODEBody* b = (ODEBody*)device->body;
	dReal fz =  NUM_POP();
	dReal fy = NUM_POP();
	dReal fx = NUM_POP();
	dBodyAddRelTorque( b->body, fx, fy, fz );

	NUM_PUSH(true);

}

void ODEDynamics::force(VEC_VAL *v){
	dReal fx = NUM_GET(&v->elts[0]);
	dReal fy = NUM_GET(&v->elts[1]);
	dReal fz = v->n > 2 ? NUM_GET(&v->elts[2]) : 0.0;
	dBodyAddRelForce( ((ODEBody*)device)->body, fx, fy, fz );

}

BOOL ODEDynamics::handle_key(KeyEvent* key) {
  if(key->normal && !key->ctrl) {
    switch(key->key) {
    case 'w': is_walls = !is_walls; return TRUE;
    case 'b': is_show_bot = !is_show_bot; return TRUE;
    case 'm': is_mobile = !is_mobile; return TRUE;
    }
  }
  return FALSE;
}

void ODEDynamics::visualize() {
#ifdef WANT_GLUT
  if(is_walls && is_draw_walls) {
    for(int i=0; i<ODE_N_WALLS; i++) {
      const dReal *pos = dGeomGetPosition(walls[i]);
      const dReal *R = dGeomGetRotation(walls[i]);
      dVector3 sides; dGeomBoxGetLengths(walls[i],sides);
//      palette->use_color(ODE_WALL);
//      draw_box(pos,R,sides);
//      palette->use_color(ODE_EDGES);
//      draw_wire_box(pos,R,sides);
    }
  }
    if(is_hard_floor){
    	double width = 1000;
    	int subdivision = 100;
    	  glLineWidth(1);
    	  glColor3f(0.5, 0.5, 0.5);
    	  glBegin(GL_LINES);

    	  for(int i = 0; i<=subdivision; i++){
    	    float stepsize = width/((float) (subdivision));

    	    glVertex3f(-width/2 + i*stepsize,width/2, 0);
    	    glVertex3f(-width/2 + i*stepsize,-width/2, 0);

    	    glVertex3f(width/2,-width/2 + i*stepsize, 0);
    	    glVertex3f(-width/2,-width/2 + i*stepsize, 0);

    	  }

    	  glEnd();
    }
#endif // WANT_GLUT
}

Body* ODEDynamics::new_body(Device* d, flo x, flo y, flo z) {
	ODEBody* b ;
	if(bodyFactory != NULL){
		b = bodyFactory->next_body(d);
	}else{
		b = new ODEBox(this, d, x, y, z, this->body_radius*2, this->body_radius*2, this->body_radius*2, 10.0);

	}
		b->parloc = bodies.add(b);

  return b;
}

void ODEDynamics::dump_header(FILE* out) {
  if(can_dump) {
    fprintf(out," \"X\" \"Y\" \"Z\"");
    fprintf(out," \"V_X\" \"V_Y\" \"V_Z\"");
    fprintf(out," \"Q_X\" \"Q_Y\" \"Q_Z\" \"Q_W\"");
    fprintf(out," \"W_X\" \"W_Y\" \"W_Z\"");
  }
}

BOOL ODEDynamics::evolve(SECONDS dt) {
	if (!is_mobile)
		return FALSE;
	time_slop += dt;
	while (time_slop > 0) {
		dSpaceCollide(space, this, &nearCallback);
		// add forces
		for (int i = 0; i < bodies.max_id(); i++) {
			ODEBody* b = (ODEBody*) bodies.get(i);
			if (b)
				b->drive();
		}
		dWorldQuickStep(world, substep);
		dJointGroupEmpty(contactgroup);
		if (is_walls && is_inescapable)
			reset_escapes();
		time_slop -= substep;
	}
	for (int i = 0; i < bodies.max_id(); i++) // mark everything as moved
	{
		Body* b = (Body*) bodies.get(i);
		if (b)
			b->moved = TRUE;
	}
	return TRUE;
}

// return escaped bots to a new starting position
void ODEDynamics::reset_escapes() {
  for(int i=0;i<bodies.max_id();i++) { 
    ODEBody* b = (ODEBody*)bodies.get(i); 
    if(b) {
      dContact contact;
      if(dCollide(pen,b->geom,1,&contact.geom,sizeof(dContact))) continue;
      // if not in pen, reset
      const flo *p = b->position();
      //post("You cannot escape! [%.2f %.2f %.2f]\n",p[0],p[1],p[2]);
      METERS loc[3];
      if(parent->distribution->next_location(loc)) {
        b->set_position(loc[0],loc[1],loc[2]);
        b->set_velocity(0,0,0); b->set_ang_velocity(0,0,0); // start still
      } else { // if there's no starting position available, die instead
        hardware->set_vm_context(b->container);
        die(TRUE);
      }
    }
  }
}



// hardware emulation
void ODEDynamics::mov(VEC_VAL *v) {
  ODEBody* b = (ODEBody*)device->body;
  b->desired_v[0] = NUM_GET(&v->elts[0]);
  b->desired_v[1] = NUM_GET(&v->elts[1]);
  b->desired_v[2] = v->n > 2 ? NUM_GET(&v->elts[2]) : 0.0;
  dReal len = sqrt(b->desired_v[0]*b->desired_v[0] + 
                   b->desired_v[1]*b->desired_v[1] +
                   b->desired_v[2]*b->desired_v[2]);
  if(len>speed_lim) {
    for(int i=0;i<3;i++) b->desired_v[i] *= speed_lim/len;
  }
}

NUM_VAL ODEDynamics::read_bump (VOID) {
  return (float)((ODEBody*)device->body)->did_bump;
}


Color* ODEDynamics::ODE_SELECTED;
Color* ODEDynamics::ODE_DISABLED;
Color* ODEDynamics::ODE_BOT;
Color* ODEDynamics::ODE_BOT_BUMPED;
Color* ODEDynamics::ODE_EDGES;
Color* ODEDynamics::ODE_WALL;
Color* ODEDynamics::ODE_BRADLEY;
Color* ODEDynamics::ODE_RAINBOW_BASE;

void ODEDynamics::register_colors() {
#ifdef WANT_GLUT
	  ODE_SELECTED = palette->register_color("ODE_SELECTED", 0, 0.7, 1, 1);
	  ODE_DISABLED = palette->register_color("ODE_DISABLED", 0.8, 0, 0, 0.7);
	  ODE_BOT = palette->register_color("ODE_BOT", 1, 0, 0, 0.7);
	  ODE_BOT_BUMPED = palette->register_color("ODE_BOT_BUMP", 0, 1, 0, 0.7);
	  ODE_EDGES = palette->register_color("ODE_EDGES", 0, 0, 1, 1);
	  ODE_WALL = palette->register_color("ODE_WALL", 1, 0, 0, 0.1);
	  ODE_BRADLEY = palette->register_color("ODE_BRADLEY", 0.29, 0.325, 0.125, 1.0);
	  ODE_RAINBOW_BASE = palette->register_color("ODE_RAINBOW_BASE", 1.0, 1.0, 1.0, 0.7);
#endif
}

void* ODEDynamicsPlugin::get_sim_plugin(string type, string name, Args* args,
                      SpatialComputer* cpu, int n){
	  if(type == LAYER_PLUGIN) {
	    if(name == LAYER_NAME) {
	    	return new ODEDynamics(args, cpu, n);
	    }
	  }
	    return NULL;
}


string ODEDynamicsPlugin::inventory(){
	 return "# ODE dynamics plugin\n" +
	    registry_entry(LAYER_PLUGIN,LAYER_NAME,DLL_NAME) +
	    registry_entry(DISTRIBUTION_PLUGIN, D_ODEGRID, DLL_NAME);
}

vector<HardwareFunction> ODEDynamics::getImplementedHardwareFunctions()
{
    vector<HardwareFunction> hardwareFunctions;
    hardwareFunctions.push_back(MOV_FN);
    return hardwareFunctions;
}

extern "C" {
  ProtoPluginLibrary* get_proto_plugin_library()
  { return new ODEDynamicsPlugin(); }

  const char* get_proto_plugin_inventory()
  {	  return (new string(ODEDynamicsPlugin::inventory()))->c_str(); }
}
