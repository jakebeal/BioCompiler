#ifndef __ODEBODY__
#define __ODEBODY__


//TODO this should be defined in config.h, for some reason it wasn't working
#define WANT_GLUT 1
#define dDOUBLE 1
//#define dSINGLE 1
//#include "odedynamics.h"
#include "config.h"
#include <ode/ode.h>
#include <proto/visualizer.h>
#include <proto/spatialcomputer.h>
#include <sstream>
/*****************************************************************************
 *  ODE BODY                                                                 *
 *****************************************************************************/

class ODEDynamics;

class ODEBody : public Body {

private:
 flo* f_position;
 flo* f_orientation;
 flo* f_velocity;
 flo* f_ang_velocity;

 public:
  ODEDynamics* parent; int parloc; // back pointers
  dBodyID body;
  dGeomID geom;      // the basic ODEBody is always a cube
  flo       did_bump;              // bump sensor
  flo       desired_v[3];   // the velocity that's wanted
  Population joints;
  ODEBody(ODEDynamics* parent, Device* container);
  ODEBody(ODEDynamics* parent, Device* container, flo x, flo y, flo z, flo r);
  ODEBody(ODEDynamics* parent, Device* container, flo x, flo y, flo z, flo w, flo l, flo h, flo mass);
  ~ODEBody();
  virtual void visualize() = 0;
  virtual void render_selection() = 0;
  void preupdate() { for(int i=0;i<3;i++) desired_v[i]=0; }
  void update() { did_bump=false; }
  void dump_state(FILE* out, int verbosity); // print state to file
  void drive(); // internal: called to apply force toward the desired velocity

  // accessors
  const flo* position() {
	  const dReal* p = dBodyGetPosition(body);
	  for(int i=0; i < 3; i++) f_position[i] = (flo) p[i];
	  return f_position;
  }

  const flo* orientation() {
	  const dReal* q =  dBodyGetQuaternion(body);
	  for(int i=0; i < 4; i++) f_orientation[i] = (flo) q[i];
	  return f_orientation;
  }

  const flo* velocity() {
	  const dReal* v = dBodyGetLinearVel(body);
	  for(int i=0; i < 3; i++) f_velocity[i] = (flo) v[i];
	  return f_velocity;
  }
  const flo* ang_velocity() {
	  const dReal* omega = dBodyGetAngularVel(body);
	  for(int i=0; i < 3; i++) f_ang_velocity[i] = (flo) omega[i];
	  return f_ang_velocity;
  }
  void set_position(flo x, flo y, flo z) { dBodySetPosition(body,x,y,z); }
  void set_orientation(const flo *q) {
	  dReal qu[4];
	  for(int i=0; i < 0; i++) qu[i] = (flo) q[i];
	  dBodySetQuaternion(body, (dReal*)&qu);
  }
  void set_velocity(flo dx, flo dy, flo dz)
    { dBodySetLinearVel(body,dx,dy,dz); }
  void set_ang_velocity(flo dx, flo dy, flo dz)
    { dBodySetAngularVel(body,dx,dy,dz); }
  virtual flo display_radius() = 0;

};

class ODEBox : public ODEBody{
public:

//ODEBox(ODEDynamics* parent, Device* container, flo *pos, flo *quat, flo* dim, flo mass);
	  ODEBox(ODEDynamics* parent, Device* container, flo x, flo y, flo z, flo w, flo l, flo h, flo mass);
	  void visualize();
	  void render_selection();
	  flo display_radius()
	     { dVector3 len; dGeomBoxGetLengths(geom,len); return (flo) len[0]/2.0; }

};


class ODESphere : public ODEBody{
public:

	  ODESphere(ODEDynamics* parent, Device* container, double* pos, double* quat, double radius, double mass);
	  void visualize();
	  void render_selection();
	  flo display_radius()
	     { dVector3 len; return (flo) dGeomSphereGetRadius(geom);  }

};


class ODECylinder : public ODEBody{
public:

	  ODECylinder(ODEDynamics* parent, Device* container, double* pos, double* quat, double radius, double length, double mass);
	  void visualize();
	  void render_selection();
	  flo display_radius()
	  { 	dReal radius;
			dReal height;
			dGeomCylinderGetParams(geom, &radius, &height);
			return height;
	  }

};

class ODECapsule : public ODEBody{
public:

	ODECapsule(ODEDynamics* parent, Device* container, double* pos, double* quat, double radius, double length, double mass);
	  void visualize();
	  void render_selection();
	  flo display_radius()
	  { 	dReal radius;
			dReal height;
			dGeomCapsuleGetParams(geom, &radius, &height);
			return height;
	  }

};

#endif // __ODEBODY__
