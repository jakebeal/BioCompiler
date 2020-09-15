#include "config.h"
#include "proto_version.h"
#include "spatialcomputer.h"
#include "utils.h" // also pulls in math
#include "plugin_manager.h"
#include "DefaultsPlugin.h"
#include "compiler-utils.h" // ordinarily included by neocompiler
#include "visualizer.h"
#include <fstream>
#include <iostream>
#include <algorithm>

#define DEFAULT_HEADLESS true

SpatialComputer* computer = NULL;

bool test_mode = false;
char dump_name[1000]; // for controlling all outputs when test_mode is true

map<string,uint8_t> create_opcode_map() {
   map<string,uint8_t> m;
  #define INSTRUCTION(name) m[#name] = name;
  #define INSTRUCTION_N(name,n) m[#a "_" #n] = name<a>;
  #include "delftproto.instructions"
  #undef INSTRUCTION_N
  #undef INSTRUCTION
   return m;
}

map<string,uint8_t> opcodes = create_opcode_map();

vector<uint8_t> convert_inst(vector<string> tokens) {
   map<string,uint8_t>::iterator it = opcodes.begin();
   vector<uint8_t> ret;
   for(int i=0; i<tokens.size(); i++) {
      map<string,uint8_t>::iterator it = opcodes.find(tokens[i]);
      if( it == opcodes.end() ) {
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


void FATAL(string err) {
   cout << "FATAL: " << err << endl;
   exit(1);
}

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
double stop_time = INFINITY; // by default, the simulator runs forever

void print_exit_notes() {
}

void shutdown_app() {
  delete computer;
  print_exit_notes();
  exit(0);
}

void advance_time() {
  bool changed = computer->evolve(sim_time);
}

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

int main (int argc, char *argv[]) {
  printf("PROTO v%s%s (Kernel %s) (Developed by MIT Space-Time Programming Group 2005-2008)\n",
      PROTO_VERSION,
      "[opsim]",
      KERNEL_VERSION);
  Args *args = new Args(argc,argv); // set up the arg parser
  // define color palette
#ifdef WANT_GLUT
  palette = Palette::default_palette;
#endif //WANT_GLUT
  computer = new SpatialComputer(args,!test_mode); // then the computer
  int len = -1;
  if(args->argc <= 0)
     FATAL("Insufficient arguments");
  uint8_t* s = read_script(args->argv[args->argc-1],&len);
  if(len > 0 && s != NULL)
     computer->load_script(s,len);
  cout << "DONE load_script." << endl;
  // and start!
  bool headless = args->extract_switch("-headless") || DEFAULT_HEADLESS;
  if(args->extract_switch("-stop-after")) stop_time = args->pop_number();
  computer->is_dump = true;
  computer->is_debug = true;
  computer->dump_period = stop_time-1;
  if(headless) {
    if(stop_time==INFINITY) 
      uerror("Headless runs must set an end time with -stop-after N");
    while(1) idle();
  }
  cout << "Exiting OPSIM." << endl;
}
