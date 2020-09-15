/* An extremely simple physics package
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __SIMPLEDYNAMICS__
#define __SIMPLEDYNAMICS__

#include "spatialcomputer.h"

class SimpleDynamics;

/* These math-related declarations are a kludgey holdover and need neatening */
#define N_WALLS 6
struct Vek {
  flo x,y,z;
  Vek() {}
  Vek(const flo* v) { x=v[0]; y=v[1]; z=v[2]; }
  Vek(flo x, flo y, flo z) { this->x=x; this->y=y; this->z=z; }
};
typedef Vek Point;

/*****************************************************************************
 *  SIMPLE BODY                                                              *
 *****************************************************************************/
class SimpleBody : public Body {
  friend class SimpleDynamics;
 protected:
  SimpleDynamics* parent; int parloc; // back pointers
  flo p[3],v[3]; // position, velocity, force
  flo radius; // bodies are spherical
  bool wall_touch; // is the device currently being affected by a wall?
  
 public:
  const flo* position() { return p; }
  const flo* velocity() { return v; }
  void set_position(flo x, flo y, flo z) { p[0]=x; p[1]=y; p[2]=z; }
  void set_velocity(flo dx, flo dy, flo dz)  { v[0]=dx; v[1]=dy; v[2]=dz; }
  // simple bodies don't have an orientation or angular velocity
  const flo* orientation();
  const flo* ang_velocity();
  void set_orientation(const flo *q) {}
  void set_ang_velocity(flo dx, flo dy, flo dz) {}
  flo display_radius() { return radius; }
  
  SimpleBody(SimpleDynamics *parent, Device* container, flo x, flo y, 
             flo z, flo r) : Body(container) { 
    this->parent=parent; moved=false; 
    set_position(x,y,z); set_velocity(0,0,0); radius=r; wall_touch = false;
  }
  ~SimpleBody();
  void preupdate();
  void visualize();
  void render_selection();
  void dump_state(FILE* out, int verbosity); // print state to file
};

/*****************************************************************************
 *  SIMPLE DYNAMICS                                                          *
 *****************************************************************************/

class SimpleDynamics : public BodyDynamics, HardwarePatch {
  friend class SimpleBody;
 protected:
  Population bodies;
  flo body_radius;
  flo act_err; // fraction by which actuation varies
  Point walls[N_WALLS];

 public:
  bool is_show_heading; // heading direction tick
  bool is_walls; // put force boundaries on area
  bool is_hard_floor; // put a hard floor at Z=0
  bool is_show_bot; // display this layer at all
  bool is_mobile; // evolve layer forward
  flo speed_lim; // maximum speed (defaults to infinity)
  uint32_t dumpmask;
  
  SimpleDynamics(Args* args, SpatialComputer* parent,int n);
  bool evolve(SECONDS dt);
  bool handle_key(KeyEvent* key);
  void visualize();
  Body* new_body(Device* d, flo x, flo y, flo z);
  void dump_header(FILE* out); // list log-file fields

  // hardware emulation
  void mov(Tuple val);

  // returns a list of function  that it patches/ provides impementation for
  static vector<HardwareFunction> getImplementedHardwareFunctions();

  static Color* SIMPLE_BODY;
  virtual void register_colors();
 private:
  void radius_set_op(Machine* machine);
  void radius_get_op(Machine* machine);
  Number radius_set (Number val);
  Number radius_get ();
  void wall_bump_op(Machine* machine);
  
  // not yet implemented:
  //Tuple read_ranger (VOID);
  //Number read_bump (VOID);
};

#endif //__SIMPLEDYNAMICS__
