/* An extremely simple physics package
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "config.h"
#include "simpledynamics.h"
#include "visualizer.h"

/*****************************************************************************
 *  VEKTOR OPS                                                               *
 *****************************************************************************/
#define ND 3
#define elt_at(x, i)   ((flo*)x)[i]
#define elt_ati(x, i) &((flo*)x)[i]
// This structure can be used to represent either a 3D point or 3D vector

static inline Vek* vek_fil (Vek *x, flo val) {
  int i;
  for (i=0; i<ND; i++)
    *elt_ati(x, i) = val;
  return x;
}

static inline Vek* vek_mul (Vek* x, flo val) {
  int i;
  for (i=0; i<ND; i++)
    *elt_ati(x, i) *= val;
  return x;
}

static void vek_out (Vek* x) {
  int i;
  for (i=0; i<ND; i++)
    post(" %.2f", elt_at(x, i));
}

static inline Vek* vek_cpy (Vek* x, Vek* y) {
  int i;
  for (i=0; i<ND; i++)
    *elt_ati(x, i) = elt_at(y, i);
  return x;
}

static inline Vek* vek_set (Vek* v, flo x, flo y, flo z) {
  v->x = x;
  v->y = y;
  v->z = z;
  return v;
}

static inline Vek* vek_add (Vek* d, Vek* s) {
  int i;
  for (i=0; i<ND; i++)
    *elt_ati(d, i) += elt_at(s, i);
  return d;
}

static inline Vek* vek_sub (Vek* d, Vek* s) {
  int i;
  for (i=0; i<ND; i++)
    *elt_ati(d, i) -= elt_at(s, i);
  return d;
}

static inline flo vek_dot (Vek* x, Vek* y) {
  int i;
  flo d = 0.0;
  for (i=0; i<ND; i++)
    d += elt_at(x, i) * elt_at(y, i);
  return d;
}

static inline flo vek_len_sqr (Vek* x) {
  return vek_dot(x, x);
}

static inline flo vek_len (Vek* x) {
  return (flo)sqrt((double)vek_len_sqr(x));
}

static inline flo vek_dst_sqr (Vek* x, Vek* y) {
  Vek t;
  vek_cpy(&t, x);
  vek_sub(&t, y);
  return vek_len_sqr(&t);
}

static inline flo vek_dst (Vek* x, Vek* y) {
  return (flo)sqrt(vek_dst_sqr(x, y));
}

/*****************************************************************************
 *  SIMPLE BODY                                                              *
 *****************************************************************************/
static flo q[4] = {1,0,0,0}; // null orientation quaternion
static flo w[3] = {0,0,0}; // null angular velocity vector
const flo* SimpleBody::orientation() {return q; }
const flo* SimpleBody::ang_velocity() { return w; }

SimpleBody::~SimpleBody() { parent->bodies.remove(parloc); }

void SimpleBody::preupdate() { set_velocity(0,0,0); }

// assumes display is centered
void SimpleBody::visualize() {
#ifdef WANT_GLUT
  flo x, y;
  if(!parent->is_show_bot) return; // don't display unless should be shown
  palette->use_color(SimpleDynamics::SIMPLE_BODY);
  glPushMatrix();
  glScalef(radius, radius, radius);
  if (parent->is_mobile) {
    glLineWidth(2);
    if (parent->parent->volume->dimensions()==3) {
      glPushMatrix();
      glRotatef(90.0, 1.0, 0.0, 0.0);
      draw_circle(1);
      glPopMatrix();
      glPushMatrix();
      glRotatef(90.0, 0.0, 1.0, 0.0);
      draw_circle(1);
      glPopMatrix();
    }
    draw_circle(1);
    if (parent->is_show_heading) {
      glBegin(GL_LINES);
      glVertex2f(0, 0);
      Vek vec(v); 
      flo l=vek_len(&vec);
      if(l>0) vek_mul(&vec,1/l); // normalize vel
      glVertex3f(vec.x, vec.y, vec.z);
      glEnd();
    }
  } else {
    glBegin(GL_POINTS);
    glVertex2f(0, 0);
    glEnd();
  }
  glPopMatrix();
#endif // WANT_GLUT
}

void SimpleBody::render_selection() {
#ifdef WANT_GLUT
  flo x, y;
  glScalef(radius, radius, radius);
  if (parent->parent->volume->dimensions()==3) {
    glPushMatrix();
    glRotatef(90.0, 1.0, 0.0, 0.0);
    draw_disk(1);
    glPopMatrix();
    glPushMatrix();
    glRotatef(90.0, 0.0, 1.0, 0.0);
    draw_disk(1);
    glPopMatrix();
  }
  draw_disk(1);
#endif // WANT_GLUT
}

void SimpleBody::dump_state(FILE* out, int verbosity) {
  if(verbosity==0) {
    if(parent->dumpmask & 0x01) fprintf(out," %.2f",p[0]);
    if(parent->dumpmask & 0x02) fprintf(out," %.2f",p[1]);
    if(parent->dumpmask & 0x04) fprintf(out," %.2f",p[2]);
    if(parent->dumpmask & 0x08) fprintf(out," %.2f",v[0]);
    if(parent->dumpmask & 0x10) fprintf(out," %.2f",v[1]);
    if(parent->dumpmask & 0x20) fprintf(out," %.2f",v[2]);
    if(parent->dumpmask & 0x40) fprintf(out," %.2f",radius);
  } else {
    fprintf(out,"Position=[%.2f %.2f %.2f], ",p[0],p[1],p[2]);
    fprintf(out,"Velocity=[%.2f %.2f %.2f] (Speed=%.2f), ",v[0],v[1],v[2],
            sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]));
    fprintf(out,"Radius=%.2f\n",radius);
  }
}

/*****************************************************************************
 *  BOUNDARIES                                                               *
 *****************************************************************************/
Point north_wall_normal (  0.0, -1.0,  0.0 );
Point south_wall_normal (  0.0,  1.0,  0.0 );
Point east_wall_normal  ( -1.0,  0.0,  0.0 );
Point west_wall_normal  (  1.0,  0.0,  0.0 );
Point top_wall_normal   (  0.0,  0.0, -1.0 );
Point bottom_wall_normal(  0.0,  0.0,  1.0 );

Point* wall_normals[N_WALLS]
  = { &north_wall_normal, &south_wall_normal, &east_wall_normal, 
      &west_wall_normal, &top_wall_normal, &bottom_wall_normal };

Point north_wall_point  (  0.0,  1.0,  0.0 );
Point south_wall_point  (  0.0, -1.0,  0.0 );
Point east_wall_point   (  1.0,  0.0,  0.0 );
Point west_wall_point   ( -1.0,  0.0,  0.0 );
Point top_wall_point    (  0.0,  0.0,  1.0 );
Point bottom_wall_point (  0.0,  0.0, -1.0 );

Point* wall_points[N_WALLS]
  = { &north_wall_point, &south_wall_point, &east_wall_point, 
      &west_wall_point, &top_wall_point, &bottom_wall_point };

/*****************************************************************************
 *  SIMPLE DYNAMICS                                                          *
 *****************************************************************************/
#define K_BODY_RAD 0.0870 // constant matched against previous visualization
#define MAX_V 100 // default ceiling on velocity
SimpleDynamics::SimpleDynamics(Args* args, SpatialComputer* parent, int n) 
  : BodyDynamics(parent) {
  ensure_colors_registered("SimpleDynamics");
  flo width=parent->volume->r - parent->volume->l;
  flo height=parent->volume->t - parent->volume->b;
  // read options
  body_radius = (args->extract_switch("-rad")) ? args->pop_number()
    : K_BODY_RAD*sqrt((width*height)/(flo)n);
  act_err = (args->extract_switch("-act-err")) ? args->pop_number() : 0.0;
  is_show_heading = args->extract_switch("-h"); // heading direction tick
  speed_lim = (args->extract_switch("-S"))?args->pop_number():MAX_V;
  is_walls = ((args->extract_switch("-w") & !args->extract_switch("-nw")))
    ? true : false;
  is_hard_floor = args->extract_switch("-floor"); // equiv to "stalk"
  is_mobile = args->extract_switch("-m");
  is_show_bot = !args->extract_switch("-hide-body");
  args->undefault(&can_dump,"-Ddynamics","-NDdynamics");
  dumpmask = args->extract_switch("-Ddynamics-mask") ? 
    (int)args->pop_number() : -1;
  // make the walls
  for (int j=0; j<N_WALLS; j++) {
    walls[j].x = wall_points[j]->x*width*0.4;
    walls[j].y = wall_points[j]->y*height*0.4;
    if(parent->volume->dimensions()==3) {
      Rect3* r = (Rect3*)parent->volume;
      walls[j].z = wall_points[j]->z*(r->c-r->f)*0.4;
    } else {
      walls[j].z = wall_points[j]->z;
    }
  }
  // register to simulate hardware
  parent->hardware.patch(this,MOV_FN);
  parent->hardware.registerOpcode(new OpHandler<SimpleDynamics>(this, &SimpleDynamics::radius_set_op, "radius-set scalar scalar"));
  parent->hardware.registerOpcode(new OpHandler<SimpleDynamics>(this, &SimpleDynamics::radius_get_op, "radius scalar"));
  parent->hardware.registerOpcode(new OpHandler<SimpleDynamics>(this, &SimpleDynamics::wall_bump_op, "wall-bump scalar"));
}

Color* SimpleDynamics::SIMPLE_BODY;
void SimpleDynamics::register_colors() {
#ifdef WANT_GLUT
  SIMPLE_BODY = palette->register_color("SIMPLE_BODY", 1.0, 0.25, 0.0, 0.8);
#endif
}

void SimpleDynamics::radius_set_op(Machine* machine) {
  radius_set(machine->stack.peek(0).asNumber());
}

void SimpleDynamics::radius_get_op(Machine* machine) {
  machine->stack.push(radius_get());
}

void SimpleDynamics::wall_bump_op(Machine* machine) {
  bool bump = ((SimpleBody*)device->body)->wall_touch;
  machine->stack.push(bump);
}

vector<HardwareFunction> SimpleDynamics::getImplementedHardwareFunctions()
{
    vector<HardwareFunction> hardwareFunctions;
    hardwareFunctions.push_back(MOV_FN);
    return hardwareFunctions;
}

bool SimpleDynamics::handle_key(KeyEvent* key) {
  if(key->normal && !key->ctrl) {
    switch(key->key) {
    case 'w': is_walls = !is_walls; return true;
    case 'b': is_show_bot = !is_show_bot; return true;
    case 'm': is_mobile = !is_mobile; return true;
    case 'h': is_show_heading = !is_show_heading; return true;
    }
  }
  return false;
}

void SimpleDynamics::visualize() {
  // only the walls need any drawing here: all else goes through Device
  // and we aren't drawing walls, either
}

Body* SimpleDynamics::new_body(Device* d, flo x, flo y, flo z) {
  SimpleBody* b = new SimpleBody(this,d,x,y,z,body_radius);
  b->parloc = bodies.add(b);
  return b;
}

void SimpleDynamics::dump_header(FILE* out) {
  if(can_dump) {
    if(dumpmask & 0x01) fprintf(out," \"X\"");
    if(dumpmask & 0x02) fprintf(out," \"Y\"");
    if(dumpmask & 0x04) fprintf(out," \"Z\"");
    if(dumpmask & 0x08) fprintf(out," \"V_X\"");
    if(dumpmask & 0x10) fprintf(out," \"V_Y\"");
    if(dumpmask & 0x20) fprintf(out," \"V_Z\"");
    if(dumpmask & 0x40) fprintf(out," \"RADIUS\"");
  }
}


/*****************************************************************************
 *  INCREMENTAL EVOLUTION & ACTUATION                                        *
 *****************************************************************************/
// Note: although the old sim-bot physics spoke of force, they act only at
//  the level of velocity.  

// in simple dynamics, just step based on velocity and position
#define K_BOUND   0.75  // restoring force from walls
bool SimpleDynamics::evolve(SECONDS dt) {
  if(!is_mobile) return false;
  for(int i=0;i<bodies.max_id();i++) {
    Body* b = (Body*)bodies.get(i);
    if(b) { 
      // device moves itself
      Vek dp(b->velocity());
      flo len = vek_dot(&dp,&dp);
      if(act_err) {
        dp.x += len*act_err*urnd(-0.5,0.5); 
        dp.y += len*act_err*urnd(-0.5,0.5); 
        dp.z += len*act_err*urnd(-0.5,0.5);
      }
      if (len > speed_lim*speed_lim) vek_mul(&dp,speed_lim/sqrt(len)); // limit velocity
      vek_mul(&dp,dt);
      // device is moved by walls
      ((SimpleBody*)b)->wall_touch = false;
      if (is_walls) {
	for (int j=0; j<N_WALLS; j++) {
	  Vek vec(b->position());
	  vek_sub(&vec, &walls[j]);
	  flo d = vek_dot(&vec, wall_normals[j]);
	  if (d < 0.0) {
	    vek_cpy(&vec, wall_normals[j]);
	    vek_mul(&vec, -d * K_BOUND * dt);
	    vek_add(&dp, &vec);
            ((SimpleBody*)b)->wall_touch = true;
	  }
	}
      }
      // adjust the position
      Vek pos(b->position());
      b->moved = (dp.x||dp.y||dp.z);
      vek_add(&pos,&dp);
      if(parent->volume->dimensions()==2) { pos.z=0; }
      // Hard floor at z=0: if the calculated pos has z < 0, reset to 0
      else if(is_hard_floor && pos.z<0) { pos.z=0; b->moved=true; }
      b->set_position(pos.x,pos.y,pos.z);
    }
  }
  return true;
}


// There is no subtlety here: mov just sets velocity directly
void SimpleDynamics::mov(Tuple v) {
  flo x = v[0].asNumber();
  flo y = v[1].asNumber();
  flo z = v.size() > 2 ? v[2].asNumber() : 0;
  device->body->set_velocity(x,y,z);
}
// sensing & actuation of body radius
Number SimpleDynamics::radius_set (Number val)
{ return ((SimpleBody*)device->body)->radius = val; }
Number SimpleDynamics::radius_get () 
{ return ((SimpleBody*)device->body)->radius; }
