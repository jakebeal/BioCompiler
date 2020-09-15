#include "odedynamics.h"
#include "odebody.h"
/*****************************************************************************
 *  GEOM DRAWING                                                             *
 *****************************************************************************/
// Note: this largely duplicates some of the code in drawing_primitives.cpp
static void normalizeVector3 (float v[3])
{
  float len = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
  if (len <= 0.0f) {
    v[0] = 1;
    v[1] = 0;
    v[2] = 0;
  }
  else {
    len = 1.0f / (float)sqrt(len);
    v[0] *= len;
    v[1] *= len;
    v[2] *= len;
  }
}
static void drawTriangle (const float *v0, const float *v1, const float *v2, int solid)
{
  float u[3],v[3],normal[3];
  u[0] = v1[0] - v0[0];
  u[1] = v1[1] - v0[1];
  u[2] = v1[2] - v0[2];
  v[0] = v2[0] - v0[0];
  v[1] = v2[1] - v0[1];
  v[2] = v2[2] - v0[2];
  dCROSS (normal,=,u,v);
  normalizeVector3 (normal);

  glBegin(solid ? GL_TRIANGLES : GL_LINE_STRIP);
  glNormal3fv (normal);
  glVertex3fv (v0);
  glVertex3fv (v1);
  glVertex3fv (v2);
  glEnd();
}


static void set_transform(const dReal pos[3], const dReal R[12]) {
#ifdef WANT_GLUT
  GLfloat matrix[16];
  matrix[0]=R[0];    matrix[1]=R[4];    matrix[2]=R[8];    matrix[3]=0;
  matrix[4]=R[1];    matrix[5]=R[5];    matrix[6]=R[9];    matrix[7]=0;
  matrix[8]=R[2];    matrix[9]=R[6];    matrix[10]=R[10];  matrix[11]=0;
  matrix[12]=pos[0]; matrix[13]=pos[1]; matrix[14]=pos[2]; matrix[15]=1;
  glPushMatrix();
  glMultMatrixf(matrix);
#endif // WANT_GLUT
}

static void shift_transform(const dReal pos[3]) {
#ifdef WANT_GLUT
  GLfloat matrix[16];
  matrix[0]=1;    matrix[1]=0;    matrix[2]=0;    matrix[3]=0;
  matrix[4]=0;    matrix[5]=1;    matrix[6]=0;    matrix[7]=0;
  matrix[8]=0;    matrix[9]=0;    matrix[10]=1;  matrix[11]=0;
  matrix[12]=pos[0]; matrix[13]=pos[1]; matrix[14]=pos[2]; matrix[15]=1;
  glPushMatrix();
  glMultMatrixf(matrix);
#endif // WANT_GLUT
}

static void do_draw_wire_box(const dReal sides[3]) {
#ifdef WANT_GLUT
  float w = sides[0]/2, h = sides[1]/2, d = sides[2]/2;
  glBegin( GL_LINE_STRIP );
  // top
  glVertex3d(-w, -h, -d);
  glVertex3d(-w,  h, -d);
  glVertex3d( w,  h, -d);
  glVertex3d( w, -h, -d);
  glVertex3d(-w, -h, -d);
  // bot
  glVertex3d(-w, -h,  d);
  glVertex3d(-w,  h,  d);
  glVertex3d( w,  h,  d);
  glVertex3d( w, -h,  d);
  glVertex3d(-w, -h,  d);
  // sides
  glVertex3d( w, -h,  d);
  glVertex3d( w, -h, -d);
  glVertex3d( w,  h, -d);
  glVertex3d( w,  h,  d);
  glVertex3d(-w,  h,  d);
  glVertex3d(-w,  h, -d);
  glEnd();
#endif // WANT_GLUT
}


static void do_draw_box(const dReal sides[3]) {
#ifdef WANT_GLUT
  float lx = sides[0]*0.5f, ly = sides[1]*0.5f, lz = sides[2]*0.5f;
  // sides
  glBegin (GL_TRIANGLE_STRIP);
  glNormal3f (-1,0,0);
  glVertex3f (-lx,-ly,-lz);
  glVertex3f (-lx,-ly,lz);
  glVertex3f (-lx,ly,-lz);
  glVertex3f (-lx,ly,lz);
  glNormal3f (0,1,0);
  glVertex3f (lx,ly,-lz);
  glVertex3f (lx,ly,lz);
  glNormal3f (1,0,0);
  glVertex3f (lx,-ly,-lz);
  glVertex3f (lx,-ly,lz);
  glNormal3f (0,-1,0);
  glVertex3f (-lx,-ly,-lz);
  glVertex3f (-lx,-ly,lz);
  glEnd();
  // top face
  glBegin (GL_TRIANGLE_FAN);
  glNormal3f (0,0,1);
  glVertex3f (-lx,-ly,lz);
  glVertex3f (lx,-ly,lz);
  glVertex3f (lx,ly,lz);
  glVertex3f (-lx,ly,lz);
  glEnd();
  // bottom face
  glBegin (GL_TRIANGLE_FAN);
  glNormal3f (0,0,-1);
  glVertex3f (-lx,-ly,-lz);
  glVertex3f (-lx,ly,-lz);
  glVertex3f (lx,ly,-lz);
  glVertex3f (lx,-ly,-lz);
  glEnd();
#endif // WANT_GLUT
}


static void do_draw_wire_sphere(const dReal radius) {
#ifdef WANT_GLUT
	  glutWireSphere(radius, 12, 12);
#endif // WANT_GLUT
}


static void do_draw_sphere(const dReal radius) {
#ifdef WANT_GLUT
  glutSolidSphere(radius, 12, 12);
#endif // WANT_GLUT
}
void draw_box(const dReal *pos, const dReal *R, const dReal *sides) {
#ifdef WANT_GLUT
  set_transform(pos, R);
  do_draw_box(sides);
  glPopMatrix();
#endif // WANT_GLUT
}

void draw_wire_box(const dReal *pos, const dReal *R, const dReal *sides) {
#ifdef WANT_GLUT
  set_transform(pos, R);
  do_draw_wire_box(sides);
  glPopMatrix();
#endif // WANT_GLUT
}

void draw_sphere(const dReal *pos, const dReal *R, dReal radius) {
#ifdef WANT_GLUT
  set_transform(pos, R);
  do_draw_sphere(radius);
  glPopMatrix();
#endif // WANT_GLUT
}

void draw_wire_sphere(const dReal *pos, const dReal *R, dReal radius) {
#ifdef WANT_GLUT
  set_transform(pos, R);
  do_draw_wire_sphere(radius);
  glPopMatrix();
#endif // WANT_GLUT
}


void draw_cylinder(const dReal *pos, const dReal *R, dReal radius, dReal height) {
#ifdef WANT_GLUT
  set_transform(pos, R);
  const dReal pos2[3] = {0,0,-height/2.0};
  shift_transform(pos2);
  //TODO I should be reusing this quadric
  GLUquadric *quad = gluNewQuadric();
  gluQuadricDrawStyle(quad,GLU_FILL);
  gluCylinder(quad, radius, radius, height, 24,24);



  gluDisk(quad, 0, radius, 10, 10);
  glPopMatrix();

  const dReal pos3[3] = {0,0,height/2.0};
  shift_transform(pos3);
  gluDisk(quad, 0, radius, 10, 10);
  glPopMatrix();

  gluDeleteQuadric(quad);
  glPopMatrix();
#endif // WANT_GLUT
}

void draw_wire_cylinder(const dReal *pos, const dReal *R, dReal radius, dReal height) {
#ifdef WANT_GLUT
  set_transform(pos, R);
  const dReal pos2[3] = {0,0,-height/2.0};
  shift_transform(pos2);
  //TODO I should be reusing this quadric
   GLUquadric *quad = gluNewQuadric();
   gluQuadricDrawStyle(quad,GLU_LINE);
   gluCylinder(quad, radius, radius, height, 24,24);


  gluDisk(quad, 0, radius, 10, 10);
  glPopMatrix();

  const dReal pos3[3] = {0,0,height/2.0};
  shift_transform(pos3);
  gluDisk(quad, 0, radius, 10, 10);
  glPopMatrix();

  gluDeleteQuadric(quad);
  glPopMatrix();
#endif // WANT_GLUT
}

void draw_wire_capsule(const dReal *pos, const dReal *R, dReal radius, dReal height) {
#ifdef WANT_GLUT
  set_transform(pos, R);
  const dReal pos2[3] = {0,0,-height/2.0};
  shift_transform(pos2);
  //TODO I should be reusing this quadric
   GLUquadric *quad = gluNewQuadric();
   gluQuadricDrawStyle(quad,GLU_LINE);
   gluCylinder(quad, radius, radius, height, 24,24);

   glutWireSphere(radius, 10,10 );
  glPopMatrix();

  const dReal pos3[3] = {0,0,height/2.0};
  shift_transform(pos3);

  glutWireSphere(radius, 10,10 );
  glPopMatrix();

  gluDeleteQuadric(quad);
  glPopMatrix();
#endif // WANT_GLUT

}


void draw_capsule(const dReal *pos, const dReal *R, dReal radius, dReal height) {
#ifdef WANT_GLUT
  set_transform(pos, R);
  const dReal pos2[3] = {0,0,-height/2.0};
  shift_transform(pos2);
  //TODO I should be reusing this quadric
  GLUquadric *quad = gluNewQuadric();
  gluQuadricDrawStyle(quad,GLU_FILL);
  gluCylinder(quad, radius, radius, height, 24,24);


  glutSolidSphere(radius, 10, 10);
  glPopMatrix();

  const dReal pos3[3] = {0,0,height/2.0};
  shift_transform(pos3);
  glutSolidSphere(radius, 10, 10);
  glPopMatrix();

  gluDeleteQuadric(quad);
  glPopMatrix();
#endif // WANT_GLUT


}


extern "C" void dsDrawTriangle (
				const float *v0, const float *v1,
				const float *v2, int solid)
{
//  if (current_state != 2) dsError ("drawing function called outside simulation loop");
//  setupDrawingMode();
  glShadeModel (GL_FLAT);
//  set_transform (pos,R);
  drawTriangle (v0, v1, v2, solid);
  glPopMatrix();
}


/*****************************************************************************
 *  ODE BODY                                                                 *
 *****************************************************************************/
ODEBody::ODEBody(ODEDynamics* parent, Device* container)
: Body(container) {


  f_position = new flo[3];
  f_orientation = new flo[4];
  f_velocity = new flo[3];
  f_ang_velocity = new flo[3];

  this->parent=parent; moved=FALSE;
  for(int i=0;i<3;i++) desired_v[i]=0;
  did_bump=false;

  // create and attach body, shape, and mass
  body = dBodyCreate(parent->world);

}

ODEBody::ODEBody(ODEDynamics *parent, Device* container, flo x, flo y, flo z,
                 flo r) : Body(container) {
	flo w = r*2;
	flo l = r*2;
	flo h = r*2;

  f_position = new flo[3];
  f_orientation = new flo[4];
  f_velocity = new flo[3];
  f_ang_velocity = new flo[3];

  this->parent=parent; moved=FALSE;
  for(int i=0;i<3;i++) desired_v[i]=0;
  did_bump=false;
  // create and attach body, shape, and mass

  geom = dCreateBox(parent->space,w,l,h);
  dMass m;
  dMassSetBox(&m,parent->density,w,l,h);

  dGeomSetBody(geom,body);


  dBodySetMass(body,&m);
  // set position and orientation
  dBodySetPosition(body, x, y, z);
  dQuaternion Q; dQFromAxisAndAngle (Q,0,0,1,0);
  dBodySetQuaternion (body,Q);
  // set up back-pointer
  dBodySetData(body,(void*)this);
  dGeomSetData(geom,(void*)this);

   if(parent->is_2d){
	  dJointID planeJointID = dJointCreatePlane2D( parent->world, 0);
	  dJointAttach( planeJointID, body, 0 );
   }
}


ODEBody::ODEBody(ODEDynamics *parent, Device* container, flo x, flo y, flo z,
                 flo w, flo l, flo h, flo mass) : Body(container) {

	printf("ODE  Body \n");

  f_position = new flo[3];
  f_orientation = new flo[4];
  f_velocity = new flo[3];
  f_ang_velocity = new flo[3];

  this->parent=parent; moved=FALSE;
  for(int i=0;i<3;i++) desired_v[i]=0;
  did_bump=false;
  // create and attach body, shape, and mass
  body = dBodyCreate(parent->world);
  geom = dCreateBox(parent->space,w,l,h);
  dMass m;
  dMassSetBoxTotal(&m,mass,w,l,h);

  dGeomSetBody(geom,body);


  dBodySetMass(body,&m);
  // set position and orientation
  dBodySetPosition(body, x, y, z);
  dQuaternion Q; dQFromAxisAndAngle (Q,0,0,1,0);
  dBodySetQuaternion (body,Q);
  // set up back-pointer
  dBodySetData(body,(void*)this);
  dGeomSetData(geom,(void*)this);

   if(parent->is_2d){
	  dJointID planeJointID = dJointCreatePlane2D( parent->world, 0);
	  dJointAttach( planeJointID, body, 0 );
   }
}

ODEBody::~ODEBody() {
  dBodyDestroy(body); dGeomDestroy(geom);
  parent->bodies.remove(parloc);

  delete f_position;
  delete f_orientation;
  delete f_velocity;
  delete f_ang_velocity;
}

void ODEBody::dump_state(FILE* out, int verbosity) {
  if(verbosity==0) {
    const flo *v = position(); fprintf(out," %.2f %.2f %.2f",v[0],v[1],v[2]);
    v = velocity(); fprintf(out," %.2f %.2f %.2f",v[0],v[1],v[2]);
    v = orientation(); fprintf(out," %.2f %.2f %.2f %.2f",v[0],v[1],v[2],v[3]);
    v = ang_velocity(); fprintf(out," %.2f %.2f %.2f",v[0],v[1],v[2]);
  } else {
    const flo *v = position();
    fprintf(out,"Position=[%.2f %.2f %.2f], ",v[0],v[1],v[2]);
    v = velocity();
    fprintf(out,"Velocity=[%.2f %.2f %.2f] (Speed=%.2f)\n",v[0],v[1],v[2],
            sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]));
    v = orientation();
    fprintf(out,"Orientation=[%.2f %.2f %.2f %.2f], ",v[0],v[1],v[2],v[3]);
    v = ang_velocity();
    fprintf(out,"Angular Velocity=[%.2f %.2f %.2f]\n",v[0],v[1],v[2]);
  }
}

void ODEBody::drive() {
  const dReal *cur = dBodyGetLinearVel(body);
  dReal force[3];
  force[0] = K_MOVE * (desired_v[0] - cur[0]);
  force[1] = K_MOVE * (desired_v[1] - cur[1]);
  force[2] = (parent->parent->is_3d())? K_MOVE * (desired_v[2] - cur[2]) : 0;
  dBodyAddForce(body, force[0], force[1], force[2]);
}

/******************************
 * ODE Box
 *******************************/
ODEBox::ODEBox(ODEDynamics* parent, Device* container, flo x, flo y, flo z, flo w, flo l, flo h, flo mass)
: ODEBody(parent, container ) {

  geom = dCreateBox(parent->space,w,l,h);
  dMass m;
  dMassSetBoxTotal(&m,mass,w,l,h);

  dGeomSetBody(geom,body);


  dBodySetMass(body,&m);
  // set position and orientation
  dBodySetPosition(body, x, y, z);
  dQuaternion Q; dQFromAxisAndAngle (Q,0,0,1,0);
  dBodySetQuaternion (body,Q);
  //dBodySetRotation(body,Q);
  // set up back-pointer
  dBodySetData(body,(void*)this);
  dGeomSetData(geom,(void*)this);

   if(parent->is_2d){
	  dJointID planeJointID = dJointCreatePlane2D( parent->world, 0);
	  dJointAttach( planeJointID, body, 0 );
   }
}


void ODEBox::visualize() {
#ifdef WANT_GLUT
//		  GLfloat light_ambient[] =
//		  {1.0,1.0,1.0, 1.0};
//		  GLfloat light_diffuse[] =
//		  {1.0, 1.0, 1.0, 1.0};
//		  GLfloat light_specular[] =
//		  {1.0, 1.0, 1.0, 1.0};
//		  GLfloat light_position[] =
//		  {1.0, 1.0, 10.0, 0.0};
//
//		  glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
//		  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
//		  glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
//		  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
//
//		  glEnable(GL_LIGHT0);
//		  glDepthFunc(GL_LESS);
//		  glEnable(GL_DEPTH_TEST);
//		  glEnable(GL_LIGHTING);

	if(!parent->is_show_bot) return; // don't display unless should be shown
  // get location/size information
  const dReal pos[3] = {0,0,0};
  const dReal *R = dGeomGetRotation(geom);
  dVector3 sides; dGeomBoxGetLengths(geom,sides);

  // choose base color
  BOOL pushed=FALSE;
  Color* color;
  if (container->is_selected) {
		palette->use_color(ODEDynamics::ODE_SELECTED);
	} else {
		if (!dBodyIsEnabled(body)) {
			palette->use_color(ODEDynamics::ODE_DISABLED);
		} else {
			if (parent->is_multicolored_bots) {
				flo h = (360.0 * parloc) / parent->bodies.max_id();
				flo r, g, b;
				hsv_to_rgb(h, 1, 1, &r, &g, &b);
				palette->scale_color(ODEDynamics::ODE_RAINBOW_BASE,r/255,g/255,b/255,1);
				pushed = TRUE;
			}
		}
	}
  if(did_bump){
		palette->use_color(ODEDynamics::ODE_BOT_BUMPED);
  }
  if(!pushed) palette->use_color(ODEDynamics::ODE_BOT);
  draw_box(pos,R,sides);
  palette->use_color(ODEDynamics::ODE_EDGES); // draw edges in separate color
  draw_wire_box(pos,R,sides);
#endif // WANT_GLUT
}

void ODEBox::render_selection() {
  const dReal pos[3] = {0,0,0};
  const dReal *R = dGeomGetRotation(geom);
  dVector3 sides; dGeomBoxGetLengths(geom,sides);
  draw_box(pos,R,sides);
}


/******************************
 * ODE Sphere
 *******************************/
ODESphere::ODESphere(ODEDynamics* parent, Device* container, double* pos, double* quat, double radius, double mass)
: ODEBody(parent, container ) {

//	mass = 10;
//	radius = 10;
cout<<"radius "<<radius<<endl;

  geom = dCreateSphere(parent->space, radius);
  dMass m;
  dMassSetSphereTotal(&m, mass, radius);

  dGeomSetBody(geom,body);


  dBodySetMass(body,&m);
  // set position and orientation
  dBodySetPosition(body, pos[0], pos[1], pos[2]);
  dQuaternion Q; dQFromAxisAndAngle (Q,0,0,1,0);
  dBodySetQuaternion (body,Q);
  // set up back-pointer
  dBodySetData(body,(void*)this);
  dGeomSetData(geom,(void*)this);

   if(parent->is_2d){
	  dJointID planeJointID = dJointCreatePlane2D( parent->world, 0);
	  dJointAttach( planeJointID, body, 0 );
   }
}


void ODESphere::visualize() {
#ifdef WANT_GLUT
  if(!parent->is_show_bot) return; // don't display unless should be shown
  // get location/size information
  const dReal pos[3] = {0,0,0};
  const dReal *R = dGeomGetRotation(geom);
  dReal rad = dGeomSphereGetRadius(geom);
  // choose base color
  BOOL pushed=FALSE;
  Color* color;
  if (container->is_selected) {
		palette->use_color(ODEDynamics::ODE_SELECTED);
	} else {
		if (!dBodyIsEnabled(body)) {
			palette->use_color(ODEDynamics::ODE_DISABLED);
		} else {
			if (parent->is_multicolored_bots) {
				flo h = (360.0 * parloc) / parent->bodies.max_id();
				flo r, g, b;
				hsv_to_rgb(h, 1, 1, &r, &g, &b);
				palette->scale_color(ODEDynamics::ODE_RAINBOW_BASE,r/255,g/255,b/255,1);
				pushed = TRUE;
			}
		}
	}
  if(did_bump){
		palette->use_color(ODEDynamics::ODE_BOT_BUMPED);
  }
  if(!pushed) palette->use_color(ODEDynamics::ODE_BOT);
  draw_sphere(pos, R, rad);
  palette->use_color(ODEDynamics::ODE_EDGES); // draw edges in separate color
  draw_wire_sphere(pos,R,rad);
#endif // WANT_GLUT
}

void ODESphere::render_selection() {
  const dReal pos[3] = {0,0,0};
  const dReal *R = dGeomGetRotation(geom);
  dReal rad = dGeomSphereGetRadius(geom);
  draw_sphere(pos,R,rad);
}


/******************************
 * ODE Cylinder
 *******************************/
ODECylinder::ODECylinder(ODEDynamics* parent, Device* container, double* pos, double* quat, double radius, double length, double mass)
: ODEBody(parent, container ) {

//	mass = 10;
//	radius = 10;
cout<<"radius "<<radius<<endl;

  geom = dCreateCylinder(parent->space,radius, length);
  dMass m;
  dMassSetCylinderTotal(&m, mass, 1, radius, length);

  dGeomSetBody(geom,body);


  dBodySetMass(body,&m);
  // set position and orientation
  dBodySetPosition(body, pos[0], pos[1], pos[2]);
  dQuaternion Q;
//  dQFromAxisAndAngle (Q,0,0,1,0);
  Q[0] = quat[0];
  Q[1] = quat[1];
  Q[2] = quat[2];
  Q[3] = quat[3];

  dBodySetQuaternion (body,Q);
  // set up back-pointer
  dBodySetData(body,(void*)this);
  dGeomSetData(geom,(void*)this);

   if(parent->is_2d){
	  dJointID planeJointID = dJointCreatePlane2D( parent->world, 0);
	  dJointAttach( planeJointID, body, 0 );
   }
}


void ODECylinder::visualize() {
#ifdef WANT_GLUT
  if(!parent->is_show_bot) return; // don't display unless should be shown
  // get location/size information

  const dReal *R = dGeomGetRotation(geom);
  dReal height;
  dReal rad;
  dGeomCylinderGetParams(geom, &rad, &height);
  const dReal pos[3] = {0,0,0};
  // choose base color
  BOOL pushed=FALSE;
  Color* color;
  if (container->is_selected) {
		palette->use_color(ODEDynamics::ODE_SELECTED);
	} else {
		if (!dBodyIsEnabled(body)) {
			palette->use_color(ODEDynamics::ODE_DISABLED);
		} else {
			if (parent->is_multicolored_bots) {
				flo h = (360.0 * parloc) / parent->bodies.max_id();
				flo r, g, b;
				hsv_to_rgb(h, 1, 1, &r, &g, &b);
				palette->scale_color(ODEDynamics::ODE_RAINBOW_BASE,r/255,g/255,b/255,1);

				pushed = TRUE;
			}
		}
	}
  if(did_bump){
		palette->use_color(ODEDynamics::ODE_BOT_BUMPED);
  }

  if(!pushed) palette->use_color(ODEDynamics::ODE_BOT);
  draw_cylinder(pos, R, rad, height);
  palette->use_color(ODEDynamics::ODE_EDGES); // draw edges in separate color
  draw_wire_cylinder(pos, R, rad, height);
#endif // WANT_GLUT
}

void ODECylinder::render_selection() {
	  const dReal pos[3] = {0,0,0};
	dReal length;
  dReal radius;
  dGeomCylinderGetParams(geom, &radius, &length);
  const dReal *R = dGeomGetRotation(geom);
  draw_cylinder(pos,R,radius, length);
}


/******************************
 * ODE Capsule
 *******************************/
ODECapsule::ODECapsule(ODEDynamics* parent, Device* container, double* pos, double* quat, double radius, double length, double mass)
: ODEBody(parent, container ) {

//	mass = 10;
//	radius = 10;
cout<<"radius "<<radius<<endl;

	geom = dCreateCapsule(parent->space,radius, length);
  dMass m;
  dMassSetCapsuleTotal(&m, mass, 1, radius, length);

  dGeomSetBody(geom,body);

  //TODO just for meta sim
  dBodySetMaxAngularSpeed(body, 0);

  dBodySetMass(body,&m);
  // set position and orientation
  dBodySetPosition(body, pos[0], pos[1], pos[2]);
  dQuaternion Q; dQFromAxisAndAngle (Q,0,0,1,0);
  dBodySetQuaternion (body,Q);
  // set up back-pointer
  dBodySetData(body,(void*)this);
  dGeomSetData(geom,(void*)this);

   if(parent->is_2d){
	  dJointID planeJointID = dJointCreatePlane2D( parent->world, 0);
	  dJointAttach( planeJointID, body, 0 );
   }
}


void ODECapsule::visualize() {
#ifdef WANT_GLUT
  if(!parent->is_show_bot) return; // don't display unless should be shown
  // get location/size information

  const dReal *R = dGeomGetRotation(geom);
  dReal height;
  dReal rad;
  dGeomCapsuleGetParams(geom, &rad, &height);
  const dReal pos[3] = {0,0,0};
  // choose base color
  BOOL pushed=FALSE;
  Color* color;
  if (container->is_selected) {
		palette->use_color(ODEDynamics::ODE_SELECTED);
	} else {
		if (!dBodyIsEnabled(body)) {
			palette->use_color(ODEDynamics::ODE_DISABLED);
		} else {
			if (parent->is_multicolored_bots) {
				flo h = (360.0 * parloc) / parent->bodies.max_id();
				flo r, g, b;
				hsv_to_rgb(h, 1, 1, &r, &g, &b);
				palette->scale_color(ODEDynamics::ODE_RAINBOW_BASE,r/255,g/255,b/255,1);

				pushed = TRUE;
			}
		}
	}
  if(did_bump){
		palette->use_color(ODEDynamics::ODE_BOT_BUMPED);
  }

  if(!pushed) palette->use_color(ODEDynamics::ODE_BOT);
  draw_capsule(pos, R, rad, height);
  palette->use_color(ODEDynamics::ODE_EDGES); // draw edges in separate color

  draw_wire_capsule(pos, R, rad, height);
#endif // WANT_GLUT
}

void ODECapsule::render_selection() {
	  const dReal pos[3] = {0,0,0};
	dReal length;
  dReal radius;
  dGeomCapsuleGetParams(geom, &radius, &length);
  const dReal *R = dGeomGetRotation(geom);
  draw_capsule(pos,R,radius, length);
}

