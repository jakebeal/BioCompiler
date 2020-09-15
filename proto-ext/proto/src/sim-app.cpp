/* Simulator application
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

// This file describes how to load all of the needed modules, manage
// the evolution of time, and dispatch events.

#include "config.h"
#include <sys/stat.h>
#include <sys/types.h>
#include "proto_version.h"

#define Instruction InstructionX
#include "spatialcomputer.h"
#undef Instruction

#include "utils.h" // also pulls in math
#include "plugin_manager.h"
#include "DefaultsPlugin.h"
#if USE_NEOCOMPILER
#include "compiler.h"
#else
#include "compiler-utils.h" // ordinarily included by neocompiler
#include "paleocompiler.h"  // should also end up pulling in these two #defines
#endif
#include "visualizer.h"
#include "sim-instructions.h"

map<string,uint8_t> OPCODE_MAP = create_opcode_map();

void shutdown_app(void);

#if USE_NEOCOMPILER
NeoCompiler* compiler = NULL;
#else
PaleoCompiler* compiler = NULL;
#endif
SpatialComputer* computer = NULL;
Visualizer* vis = NULL;

bool test_mode = false;
char dump_name[1000]; // for controlling all outputs when test_mode is true

/*****************************************************************************
 *  TIMING AND UPDATE LOOP                                                   *
 *****************************************************************************/
double stop_time = INFINITY; // by default, the simulator runs forever
bool is_sim_throttling = false; // when true, use time_ratio
bool is_stepping = false; // is time advancement frozen?
bool is_step = false; // if in stepping mode, take a single step
double time_ratio = 1.0; // sim_time/real_time
double step_size = 0.01; // default is 100 fps
double sim_time = 0.0; // elapsed simulated time
double last_real = 0.0; // last real time
double last_sim_time = 0.0; // previous advancement of time
double last_inflection_sim = 0.0; // simulator time of last ratio change
double last_inflection_real = 0.0; // real time of last ratio change
bool evolution_lagging = false; // is the simulator keeping up or not?
#define FPS_DECAY 0.95
double fps=1.0; // frames-per-second measurement
bool show_time=false;
string opcode_file=""; // file to look for opcodes


// evolve all top-level items
void advance_time() {
  bool changed = false;
  if(compiler)
     changed |= compiler->evolve(sim_time);
  changed |= (vis && vis->evolve(sim_time));
  changed |= computer->evolve(sim_time);
#ifdef WANT_GLUT
  if(changed && vis!=NULL) glutPostRedisplay(); // redraw only if needed
#endif // WANT_GLUT
}

// this routine is called either by GLUT or directly. It manages time evolution
void idle () {
  if(sim_time>stop_time) shutdown_app(); // quit when time runs out
  if(is_step || !is_stepping) {
    is_step=false;
    double new_real = get_real_secs();
    if(is_sim_throttling) {
      // time math avoids incremental deltas to limit error accumulation
      double real_delta = new_real - last_inflection_real;
      double new_time = real_delta/time_ratio + last_inflection_sim;
      if(new_time-last_sim_time > step_size/2) {
        flo tfps = 1/(new_real-last_real); last_real = new_real;
        fps = (1-FPS_DECAY)*tfps + FPS_DECAY*fps;
        // step_size = min step
        sim_time = max(new_time, last_sim_time + step_size);
        evolution_lagging = (sim_time-last_sim_time > step_size*10);
        if(evolution_lagging) { // maximum step is 10x normal step
          sim_time = last_sim_time+(step_size*10);
        }
        advance_time(); last_sim_time=sim_time;
      }
    } else {
      flo tfps = 1/(new_real-last_real); last_real = new_real;
      fps = (1-FPS_DECAY)*tfps + FPS_DECAY*fps;
      sim_time+=step_size; evolution_lagging=false;
      advance_time(); last_sim_time=sim_time;
    }
    
  }
}


/*****************************************************************************
 *  EVENT HANDLING AND DISPATCH                                              *
 *****************************************************************************/
#define CLICK_FUZZ 5  // # pixels mouse can move before click turns into drag
MouseEvent mouse;
KeyEvent key;

void select_region(flo min_x, flo min_y, flo max_x, flo max_y) {
#ifdef WANT_GLUT
  Rect rgn(min_x,max_x,min_y,max_y);
  int n = computer->devices.size();
  vis->start_select_3D(&rgn,n);
  computer->render_selection();
  vis->end_select_3D(n,&computer->selection);
  computer->update_selection();
#endif // WANT_GLUT
}

bool selecting = false;
static double drag_anchor[3], drag_current[3];

bool app_handle_mouse(MouseEvent *mouse) {
#ifdef WANT_GLUT
  if(!mouse->shift) { // left click selects, right click prints too
    if(mouse->button==GLUT_LEFT_BUTTON || mouse->button==GLUT_RIGHT_BUTTON) {
      switch(mouse->state) {
      case 0: // click
        select_region(mouse->x-1,mouse->y-1,mouse->x+1,mouse->y+1);
        if(mouse->button==GLUT_RIGHT_BUTTON) {
          computer->dump_selection(stdout,1); // print what's been clicked on
        }
        return true;
      }
    }
  } else { // shift left drag selects a region
    if(mouse->button==GLUT_LEFT_BUTTON) {
      switch(mouse->state) {
      case 1: // drag start
        selecting = true;
        drag_anchor[0]=drag_current[0]=mouse->x;
        drag_anchor[1]=drag_current[1]=mouse->y;
        drag_anchor[2]=drag_current[2]=0;
        return true;
      case 2: // drag continue
        drag_current[0]=mouse->x; drag_current[1]=mouse->y;
        return true;
      case 3: // drag end
        drag_current[0]=mouse->x; drag_current[1]=mouse->y;
        select_region(min(drag_anchor[0], drag_current[0]),
                      min(drag_anchor[1], drag_current[1]),
                      max(drag_anchor[0], drag_current[0]),
                      max(drag_anchor[1], drag_current[1]));
        selecting = false;
        return true;
      }
    } else if(mouse->button==GLUT_RIGHT_BUTTON) { // shift-right-drag = move
      switch(mouse->state) {
      case 1: // drag start
        vis->click_3d(mouse->x,mouse->y,drag_anchor);
        //post("MM: %f, %f, %f\n",drag_anchor[0],drag_anchor[1],drag_anchor[2]);
        return true;
      case 2: // drag continue
      case 3: // draw end
        vis->click_3d(mouse->x,mouse->y,drag_current);
        flo dp[3]; for(int i=0;i<3;i++) dp[i]=drag_current[i]-drag_anchor[i];
        computer->drag_selection(dp);
        for(int i=0;i<3;i++) drag_anchor[i]=drag_current[i];
        return true;
      }
    }
  }
  return false;
#endif // WANT_GLUT
}

// A mouse event is either consumed by the visualizer or computer
// The visualizer uses window coordinates, the computer uses 3D coordinates
// To allow the computer to do modal behavior (e.g. keystroke triggered
// selections), we'll check it first
void dispatch_mouse_event() {
#ifdef WANT_GLUT
  bool handled = 
    app_handle_mouse(&mouse) || computer->handle_mouse(&mouse)
    || vis->handle_mouse(&mouse);
  if(handled) glutPostRedisplay();
  // If a drag isn't handled at the start, it won't generate more dispatches
  if(mouse.state==1) mouse.state=(handled?2:-1);
#endif // WANT_GLUT
}

#define TIME_RATIO_STEP 1.1892 // 2^1/4
#define MINIMUM_RATIO 0.001 // no slower than 1ms sim per real second
#define MAXIMUM_RATIO 1000 // no faster than 1000 seconds sim per real second
bool app_handle_key(KeyEvent *key) {
  if(key->normal) {
    if(key->ctrl) {
      switch(key->key) {
      case 19: // Ctrl-S = slow down
        if(time_ratio < MAXIMUM_RATIO) {
          time_ratio *= TIME_RATIO_STEP; step_size /= TIME_RATIO_STEP;
        }
        last_inflection_sim=sim_time;
        last_inflection_real=get_real_secs();
        return true;
      case 1: // Ctrl-A = speed up
        if(time_ratio > MINIMUM_RATIO) {
          time_ratio /= TIME_RATIO_STEP; step_size *= TIME_RATIO_STEP;
        }
        last_inflection_sim=sim_time;
        last_inflection_real=get_real_secs();
        return true;
      case 4: // Ctrl-D = real-time
        step_size *= time_ratio; time_ratio = 1;
        last_inflection_sim=sim_time;
        last_inflection_real=get_real_secs();
        return true;
      }
    } else {
      switch(key->key) {
      case 'q': shutdown_app();
      case 's':
        is_stepping = 1;
        is_step = 1;
        return true;
      case 'x':
        is_stepping = 0;
        is_step = 0;
        last_inflection_sim=sim_time;
        last_inflection_real=get_real_secs();
        return true;
      case 'T': 
        show_time=!show_time;
        return true;
      case 'X':
        is_sim_throttling = !is_sim_throttling;
        last_inflection_sim=sim_time;
        last_inflection_real=get_real_secs();
        return true;
      case 'l':
        int len; uint8_t* s = compiler->compile(compiler->last_script,&len);
        computer->load_script_at_selection(s,len);
        return true;
      }
    }
  }
  return false;
}

// A key event may go to any of the top-level objects
void dispatch_key_event() {
#ifdef WANT_GLUT
  bool handled = 
    app_handle_key(&key) ||
    computer->handle_key(&key) ||
    vis->handle_key(&key) || // visualizer always there for GLUT events
    compiler->handle_key(&key);
  if(handled)  glutPostRedisplay();
#endif // WANT_GLUT
}

// Resizes the window, then tells the viewer to restart itself.
#ifdef WANT_GLUT
void resize (int new_w, int new_h) { vis->resize(new_w,new_h); }
#endif // WANT_GLUT


Color *TIME_DISPLAY, *FPS_DISPLAY, *LAG_WARNING, *DRAG_SELECTION;
void register_app_colors() {
#ifdef WANT_GLUT
  TIME_DISPLAY = palette->register_color("TIME_DISPLAY",1,0,1);
  FPS_DISPLAY = palette->register_color("FPS_DISPLAY",1,0,1);
  LAG_WARNING = palette->register_color("LAG_WARNING",1,0,0);
  DRAG_SELECTION = palette->register_color("DRAG_SELECTION", 1, 1, 0, 0.5);
#endif
}

// give each top-level object a chance to display itself
// Only the computer lives in 3D space: the others all live in 2D window coords
void render () {
#ifdef WANT_GLUT
  vis->prepare_frame();
  vis->view_3D(); // enter the 3D view for the computer
  computer->visualize();
  vis->end_3D();
  
  // local drawing
  if(show_time) {
    char text[100];
    glPushMatrix(); glPushAttrib(GL_CURRENT_BIT); 
    palette->use_color(TIME_DISPLAY);
    sprintf(text, "%.2f", sim_time);
    glTranslatef( -vis->width/2+50, -vis->height/2+50, -0.1);
    draw_text_justified(TD_BOTTOM, 100, 100, text);
    glPopAttrib(); glPopMatrix();
    
    glPushMatrix(); glPushAttrib(GL_CURRENT_BIT); 
    palette->use_color(FPS_DISPLAY);
    sprintf(text, "%.2f", fps);
    glTranslatef( vis->width/2-50, -vis->height/2+50, -0.1); 
    draw_text_justified(TD_BOTTOM, 100, 100, text);
    glPopAttrib(); glPopMatrix();
  }
  if(evolution_lagging) {
    glPushMatrix(); glPushAttrib(GL_CURRENT_BIT); 
    palette->use_color(LAG_WARNING);
    glTranslatef( 0, -vis->height/2+50, -0.1);
    draw_text_justified(TD_BOTTOM, 100, 100, "LAG WARNING");
    glPopAttrib(); glPopMatrix();
  }
  if(selecting) {
    glPushMatrix();
    palette->use_color(DRAG_SELECTION);
    glTranslatef((drag_anchor[0]+drag_current[0]-vis->width)/2,
                 -(drag_anchor[1]+drag_current[1]-vis->height)/2, -0.05);
    draw_quad(fabs(drag_anchor[0]-drag_current[0]),
              fabs(drag_anchor[1]-drag_current[1]));
    glPopMatrix();
  }
  // rest of drawing
  vis->visualize(); 
  if(compiler) compiler->visualize();
  
  vis->complete_frame();
#endif // WANT_GLUT
}
// there should be something that blinks when the simulator can't keep up
// with the time demands of its throttle [evolution_lagging==true]


// Callback for button events
// Clicking and dragging will be separated as per the Apple HIG.  To whit:
// 1. A click is when the button is pressed and released in the same spot
// 2. Dragging is when the mouse is moved while the button is down
// Multiple-button events are ignored---only the start-state matters
void on_mouse_button ( int button, int state, int x, int y ) {
#ifdef WANT_GLUT
  if (state == GLUT_DOWN && mouse.button==-1) { // no concurrent button ops
    mouse.x=x; mouse.y=y; // note the starting location
    mouse.shift = glutGetModifiers() & GLUT_ACTIVE_SHIFT;
    mouse.button = button;
    mouse.state=0; // assume click until known to be a drag
  } else if (state == GLUT_UP) { // on button release, reset
    if(mouse.state==0) { 
      dispatch_mouse_event(); // clicks do not move from start location
    } else if(mouse.state==2) { 
      mouse.state=3; mouse.x=x; mouse.y=y; // final drag location
      dispatch_mouse_event(); }
    mouse.button=-1;
  }
#endif // WANT_GLUT
}

// Callback for mouse motion events (button up or down)
// Converts a click event into a drag event following significant motion
void on_mouse_motion( int x, int y ) {
  if(mouse.button==-1) { // mouse is up
    mouse.x=x; mouse.y=y;
  } else {
    if(mouse.state==0) { // is it still a click?
      // mouse location does not change until click becomes drag
      if(max(abs(x-mouse.x), abs(y-mouse.y)) > CLICK_FUZZ) {
        mouse.state=1;
        dispatch_mouse_event(); // start point is old position
      }
    } else if(mouse.state==2) { // it's a live drag
      mouse.x=x; mouse.y=y;
      dispatch_mouse_event();
    }
  }
}

// X and Y are mouse locations, and thus ignored
void keyboard_handler( unsigned char key_id, int x, int y ) {
#ifdef WANT_GLUT
  key.normal=true; key.key = key_id;
  key.ctrl = glutGetModifiers() & GLUT_ACTIVE_CTRL;
  dispatch_key_event();
#endif // WANT_GLUT
}
void special_handler( int key_id, int x, int y ) {
#ifdef WANT_GLUT
  key.normal=false; key.special = key_id;
  key.ctrl = glutGetModifiers() & GLUT_ACTIVE_CTRL;
  dispatch_key_event();
#endif // WANT_GLUT
}

/*****************************************************************************
 *  STARTING AND STOPPING APPLICATION                                        *
 *****************************************************************************/
// destroy in the opposite order from creation
void shutdown_app() {
#ifdef WANT_GLUT
  if(vis) delete vis;
#endif // WANT_GLUT
  delete computer;
  delete compiler;
  exit(0);
}

#ifndef WANT_GLUT
#define DEFAULT_HEADLESS true
#else
#define DEFAULT_HEADLESS false
#endif

// handle command-line arguments for top-level application
void process_app_args(Args *args) {
  // Should we just display the plugin inventory and exit?
  if(args->extract_switch("--plugins")) {
    DefaultsPlugin::register_defaults();
    const PluginInventory *pm = plugins.get_plugin_inventory();
    cout << "Displaying available plugins (by type):\n";
    PluginInventory::const_iterator i = pm->begin();
    for(; i!=pm->end(); i++) {
      cout << "  " << i->first << ": ";
      PluginTypeInventory::const_iterator j = i->second.begin();
      if(j!=i->second.end()) { cout << j->first; j++; }
      for(; j!=i->second.end(); j++) { cout << ", " << j->first; }
      cout << endl;
    }
    cout << "All plugins displayed; exiting.\n";
    exit(0);
  }


  // maximum time for simulation (useful for headless execution)
  if(args->extract_switch("-opcodes")) opcode_file = args->pop_next();
  // should the simulator start paused?
  is_stepping = args->extract_switch("-step");
  // maximum time for simulation (useful for headless execution)
  if(args->extract_switch("-stop-after")) stop_time = args->pop_number();
  // throttle when told explicitly
  if(args->extract_switch("-throttle")) {
    is_sim_throttling=true;
    last_inflection_real=get_real_secs(); // need to know when it starts
  }
  show_time = args->extract_switch("-T");
  // set the ratio between simulated and real time
  if(args->extract_switch("-ratio")) time_ratio = args->pop_number();
  // minimum amount of time to advance in each simulation step
  step_size =((args->extract_switch("-s"))?args->pop_number():0.01/time_ratio);
  // is the system in test mode (and should put all output to the same spot)
  test_mode = args->extract_switch("--test-mode");
  if(test_mode) {
    const char* dump_dir = 
      args->extract_switch("-dump-dir") ? args->pop_next() : "dumps";
    const char* dump_stem =
      args->extract_switch("-dump-stem") ? args->pop_next() : "dump";
    
    // ensure that the directory exists
#ifdef _WIN32  
    if(mkdir(dump_dir) != 0) {
#else
    if(mkdir(dump_dir, ACCESSPERMS) != 0) {
#endif
      //ignore
    }
    sprintf(dump_name,"%s/%s.log",dump_dir,dump_stem);
    cperr = cpout = new ofstream(dump_name); // begin by making compiler output
  }
}

vector<uint8_t> convert_inst(vector<string> tokens) {
   //map<string,uint8_t>::iterator it = OPCODE_MAP.begin();
   //while(it != OPCODE_MAP.end()) {
   //   post(" %s -> %d \n", it->first.c_str(), it++->second);
   //}
   vector<uint8_t> ret;
   for(int i=0; i<tokens.size(); i++) {
      map<string,uint8_t>::iterator it = OPCODE_MAP.find(tokens[i]);
      if( it == OPCODE_MAP.end() ) {
         ret.push_back((uint8_t)atoi(tokens[i].c_str()));
      } else {
         ret.push_back(it->second);
      }
   }
   return ret;
}

vector<string> tokenize_file(string file, int *len) {
   vector<string> tokens;
   ifstream infile(file.c_str());
   string line;
   while ( infile.is_open() && infile.good() ) {
      string tmp;
      getline(infile,tmp);
      //remove comments
      if(tmp.find("//") != string::npos)
         tmp.erase(tmp.find("//"));
      line += tmp;
   }
   line.erase(remove(line.begin(), line.end(), '\n'), line.end());
   char* linechars = const_cast<char*>(line.c_str());
   char* tok = strtok(linechars,",");
   while(tok != NULL) {
      tokens.push_back(tok);
      tok = strtok(NULL,", ");
   }
   *len = (int)tokens.size();
   return tokens;
}

/**
 * Reads opcodes from file.
 */
uint8_t* read_script(string file, int *len) {
   vector<string> tokens = tokenize_file(file, len);
   vector<uint8_t> instructions = convert_inst(tokens);
   uint8_t* ret = new uint8_t[instructions.size()];
   for(int i=0; i<instructions.size(); i++) {
      ret[i] = instructions[i];
      //cout << "ret[" << i << "] = " << (int)ret[i] << endl;
   }
   return ret;
}

int main (int argc, char *argv[]) {
  post("PROTO v%s%s (%s) (Developed by MIT Space-Time Programming Group 2005-2008)\n",
      PROTO_VERSION,
#if USE_NEOCOMPILER
      "[neo]",
#else
      "[paleo]",
#endif
      KERNEL_VERSION);
  Args *args = new Args(argc,argv); // set up the arg parser

  // initialize randomness  [JAH: fmod added for OS X bug]
  unsigned int seed = (unsigned int)
    (args->extract_switch("-seed") ? args->pop_number()
    : fmod(get_real_secs()*1000, RAND_MAX));
  post("Using random seed %d\n", seed);
  srand(seed);

  process_app_args(args);
  bool headless = args->extract_switch("-headless") || DEFAULT_HEADLESS;
  if(!headless) {
    vis = new Visualizer(args); // start visualizer
  } else {
#ifdef WANT_GLUT
    palette = Palette::default_palette;
#endif // WANT_GLUT
  }

  computer = new SpatialComputer(args,!test_mode);

  if(opcode_file != "") {
     post("reading opcodes from: %s\n", opcode_file.c_str());
     // read from file
     int len = -1;
     uint8_t* s = read_script(opcode_file,&len);
     //post("script[%d]=\n",len);
     //for(unsigned int i=0; i<len; ++i)
     //   post("%d\n", s[i]);
     if(len > 0 && s != NULL) {
        computer->load_script(s,len);
     }
     else
        uerror("Problem loading opcode file: %s", opcode_file.c_str());
     if(!headless) {
       vis->set_bounds(computer->vis_volume); // connect to computer
       register_app_colors();
     }
  } else {
     // use a compiler
#if USE_NEOCOMPILER
     compiler = new NeoCompiler(args);  // first the compiler
     compiler->emitter = new ProtoKernelEmitter(compiler,args);
#else
     compiler = new PaleoCompiler(args);  // first the compiler
#endif
     string defops;
     computer->appendDefops(defops);
     compiler->setDefops(defops);
     if(!headless) {
        vis->set_bounds(computer->vis_volume); // connect to computer
        register_app_colors();
     }
     // load the script
     int len;
     if(args->argc==1) {
        uerror("No program specified: all arguments consumed.");
     } else {
       uint8_t* s = compiler->compile(args->argv[args->argc-1],&len);
       computer->load_script(s,len);
     }
  }
  // if in test mode, swap the C++ file for a C file for the SpatialComputer
  if(test_mode) {
    delete cpout;
    computer->dump_file = fopen(dump_name,"a");
  }
  
  // Overlay palettes, if needed
  // This comes last, so that we can ensure that all the colors are
  // registered before we start to put them to use
  while(args->extract_switch("-palette",false)) { // may use many palette files
#ifdef WANT_GLUT
    palette->overlay_from_file(args->pop_next()); // patch the palette
#else
    // ignore the palette arguments
#endif
  }

  // Check if there are any leftover arguments:
  if(args->argc>2) {
    post("WARNING: %d unhandled arguments:",args->argc-2);
    for(int i=2;i<args->argc;i++) post(" '%s'",args->argv[i-1]);
    post("\n");
  }
  
  // and start!
  if(headless) {
    if(stop_time==INFINITY) 
      uerror("Headless runs must set an end time with -stop-after N");
    while(1) idle();
  } else {
#ifdef WANT_GLUT
    // set up callbacks for user interface and idle
    glutMouseFunc(on_mouse_button);
    glutMotionFunc(on_mouse_motion);
    glutPassiveMotionFunc(on_mouse_motion);
    glutDisplayFunc(render);
    glutReshapeFunc(resize);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard_handler);
    glutSpecialFunc(special_handler);
    // finally, hand off control to the glut event-loop
    glutMainLoop(); 
#endif
  }
}

/*  Things not yet imported from the 1st generation simulator:
Medium parameters:
    else if (strcmp(arg, "-M") == 0) 
       is_show_medium = 1; 
     else if (strcmp(arg, "-d") == 0)  
       default_sim->is_medium_decay = 1; 
     if (strcmp(arg, "-stalk") == 0) 
       default_sim->is_stalk = 1; 
     else if (strcmp(arg, "-slime") == 0) 
       default_sim->is_slime = 1; 
     else if (strcmp(arg, "-mr") == 0) 
       default_sim->medium_range = atof(check_cmd_key_value(i++, argc, argv)); 

 Folding parameters: 
     else if (strcmp(arg, "-regress-every") == 0) 
       default_sim->regress_every = atoi(check_cmd_key_value(i++, argc, argv)); 
     else if (strcmp(arg, "-folds") == 0) 
       default_sim->is_folds = 1; 

 Other parameters: 
     } else if (strcmp(arg, "-e") == 0) { 
       int s = 0; 
       Obj *obj; 
       obj = eval_obj(read_object(check_cmd_key_value(i++, argc, argv), &s)); 
       check_isa(obj, sim_class); 
       sim = root_sim = (SIM*)obj; 
     } else if (strcmp(arg, "-nc") == 0)  
       default_sim->n_channels = atoi(check_cmd_key_value(i++, argc, argv)); 
     else if (strcmp(arg, "-2") == 0) { 
       // MIN_X *= 2; 
       // MAX_X *= 2; 
       is_double_view = 1; 
     } else if (strcmp(arg, "-sf") == 0){ 
       load_eval_obj(check_cmd_key_value(i++, argc, argv)); 
*/

/*
Medium keys:
  case 'Y':
    sim->is_show_matter = !sim->is_show_matter;
    break;
  case 'M': 
    is_show_medium = !is_show_medium;
    break;

Link keys:
  #ifdef IS_MOTE
  case 'D':  // implemented for sim nodes bug not link yet [11/29]
    toggle_mote_config (CONFIG_DEBUG, 0);
    break;
  case 'F':
    toggle_mote_config (CONFIG_LED, CLOCK_LED);
    break;
  #endif

Other/unknown keys:
  case 'I': 
    is_show_rid = !is_show_rid;
    break;
  case 'k': 
    do_sim(root_sim, &toggle_show_txt, NULL, NULL);
    break;
  case ' ':
  case '>':
    seq_inc = 1;
    break;
  case '':
  case '<':
    seq_inc = -1;
    break;
  case '+': val++; break;
  case '-': val--; break;
  default: post("Unknown key %c\n", key);
  }
*/
