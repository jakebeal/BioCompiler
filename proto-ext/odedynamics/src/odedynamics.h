/* Newtonian physics simulation using the Open Dynamics Engine
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __ODEDYNAMICS__
#define __ODEDYNAMICS__
//TODO this should be defined in config.h, for some reason it wasn't working
#define WANT_GLUT 1
#define dDOUBLE 1
//#define dSINGLE 1
#include <proto/proto_plugin.h>
#include <proto/spatialcomputer.h>
#include <proto/FixedIntervalTime.h>
#include <ode/ode.h>
#include <proto/proto_vm.h>


#include "config.h"
#include <proto/visualizer.h>
#include <sstream>


#define LAYER_NAME "odedynamics"
#define D_ODEGRID "odegrid"
#define DLL_NAME "libproto-ode"

#include "odebodyfactory.h"
#include "odebody.h"

class ODEBodyFactory;


#define WALL_DATA -1            // identifier for walls
#define RAY_DATA -2            // identifier for walls
#define MAX_CONTACTS 8		// maximum number of contact points per body
#define BUMP_OP "bump boolean"
#define FORCE_OP "force boolean scalar scalar scalar"
#define TORQUE_OP "torque boolean scalar scalar scalar"

#define K_MOVE 10

/*****************************************************************************
 *  ODE DYNAMICS                                                             *
 *****************************************************************************/

#define ODE_N_WALLS 8

class ODEDynamics : public BodyDynamics, HardwarePatch {
  static BOOL inited;  // Has dInitODE been called yet?
 public:

  ODEBodyFactory* bodyFactory;

  Population bodies;
  dWorldID  world;            // ODE body storage
  dSpaceID  space;            // ODE collision support
  dJointGroupID contactgroup; // ODE body motion constraints
  dGeomID walls[ODE_N_WALLS];
  dGeomID pen;                // the pen, for testing for escapes

  BOOL is_hard_floor; // put a hard floor at Z=0
  BOOL is_2d;
  BOOL is_show_bot;         // display this layer at all
  BOOL is_mobile;           // evolve layer forward
  BOOL is_walls;            // put force boundaries on area
  BOOL is_draw_walls;       // should the walls be seen
  BOOL is_inescapable;      // bodies are reset within walls when they escape
  BOOL is_multicolored_bots;  // draw bots rainbow colored!
  flo speed_lim;             // maximum speed (defaults to infinity)
  flo body_radius, density; // default body parameters
  flo substep, time_slop;   // managing multiple substeps per step
  flo gravity;
  void addDist();
  ODEDynamics(Args* args, SpatialComputer* parent,int n);
  ~ODEDynamics();
  BOOL evolve(SECONDS dt);
  BOOL handle_key(KeyEvent* key);
  void visualize();
  Body* new_body(Device* d, flo x, flo y, flo z);
  void dump_header(FILE* out); // list log-file fields

  // hardware emulation
  void mov(VEC_VAL *val);
  void force(VEC_VAL *val);
  NUM_VAL read_bump (VOID);
  // not yet implemented:
  //NUM_VAL radius_set (NUM_VAL val);
  //NUM_VAL radius_get (VOID);
  //VEC_VAL *read_coord_sensor(VOID);
  //VEC_VAL *read_ranger (VOID);
  //NUM_VAL read_bearing (VOID);
  //NUM_VAL read_speed (VOID);
  //layer_type get_type();

  // returns a list of function  that it patches/ provides impementation for
  static vector<HardwareFunction> getImplementedHardwareFunctions();

  // ODE colors
  static Color* ODE_SELECTED;
  static Color* ODE_DISABLED;
  static Color* ODE_BOT;
  static Color* ODE_BOT_BUMPED;
  static Color* ODE_EDGES;
  static Color* ODE_WALL;
  static Color* ODE_BRADLEY;
  static Color* ODE_RAINBOW_BASE;

  void register_colors() ;


 private:
  void make_walls();
  void reset_escapes(); // used when the walls are inescapable
  void bump_op(MACHINE* machine);
  void force_op(MACHINE* machine);
  void torque_op(MACHINE* machine);





};


class ODEDynamicsPlugin : public ProtoPluginLibrary{
public:
	  void* get_sim_plugin(string type, string name, Args* args,
	                       SpatialComputer* cpu, int n);
	  static string inventory();

};

#endif //__ODEDYNAMICS__
