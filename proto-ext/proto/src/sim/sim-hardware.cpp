/* Code to manage the interface between the simulator and the kernel.
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "config.h"
#include "spatialcomputer.h"
#include <iostream>
#include <sstream>

using namespace std;
/*****************************************************************************
 *  SIMULATED HARDWARE                                                       *
 *****************************************************************************/
// globals managed by set_vm_context
SimulatedHardware* hardware=NULL;
Device* device=NULL;
Machine* machine=NULL;

Device* current_device() { return device; }
SimulatedHardware* current_hardware() { return hardware; }
Machine* current_machine() { return machine; }

SimulatedHardware::SimulatedHardware() {
//    std::cout << "SimulatedHardware base ptr: " << &base << std::endl;
  for(int i=0;i<NUM_HARDWARE_FNS;i++) patch_table[i]=&base;

  requiredPatches.push_back(SET_DT_FN); // SET_DT_OP
  requiredPatches.push_back(MOV_FN); // MOV_OP
  requiredPatches.push_back(SET_PROBE_FN); // PROBE_OP
  requiredPatches.push_back(READ_RADIO_RANGE_FN); // HOOD_RADIUS_OP
  // AREA_OP ; func called in proto.c is machine_area. Which FUNC
//  requiredPatches.push_back(); 
  requiredPatches.push_back(FLEX_FN); // FLEX_OP
  // INFINITESIMAL_OP ; func called in proto.c is machine_area. Which FUNC
//  requiredPatches.push_back();
//  requiredPatches.push_back(); // DT_OP ??
  requiredOpcodes.push_back("READ_RANGER_OP");
  // NBR_RANGE_OP. TODO Check with Jake
//  requiredPatches.push_back(); // NBR_BEARING_OP
//  requiredPatches.push_back(); // NBR_VEC_OP
//  requiredPatches.push_back(); // NBR_LAG_OP
//  requiredPatches.push_back(); // MID_OP
  requiredPatches.push_back(READ_SPEED_FN); // SPEED_OP
  requiredPatches.push_back(READ_BEARING_FN); // BEARING_OP
  
//  dumpPatchTable();

    opHandlerCount = 0;
}

int SimulatedHardware::registerOpcode(OpHandlerBase* opHandler) {
  opHandlers[opHandlerCount] = opHandler;
  opHandler->opcode = opHandlerCount++ + PLATFORM_OPCODE_OFFSET;
  return opHandler->opcode;
}

void SimulatedHardware::dispatchOpcode(uint8_t op) {
  int ix = op - PLATFORM_OPCODE_OFFSET;
  if (ix < 0 || ix >= opHandlerCount) {
    // TODO Out-of-range should throw exception or something
    return;
  }
  (*opHandlers[ix])(machine);
}

void SimulatedHardware::appendDefops(string& defops) {
  stringstream ss;
  for (int ix = 0; ix < opHandlerCount; ix++) {
    OpHandlerBase* opHandler = opHandlers[ix];
    ss << "(defop " << (ix + PLATFORM_OPCODE_OFFSET) << " " <<  opHandler->defop << ")\n";
  }
  defops = ss.str();
}

void SimulatedHardware::patch(HardwarePatch* p, HardwareFunction fn) {
    //printf("\n Patching Func: %i \n", fn);
    
  patch_table[fn]=p;

//  dumpPatchTable();
}
void SimulatedHardware::dumpPatchTable()
{
    for(int i = 0; i<NUM_HARDWARE_FNS;i++)
  {
      std::cout << "Patch table i = " << i << " : " << patch_table[i] << std::endl;
  }
}

// This function sets the globals that connect it to a simulated device.
void SimulatedHardware::set_vm_context(Device* d) {
  hardware = this;
  device = d;
  machine = d->vm;
  /*is_debugging_val = is_kernel_debug && d->debug();
  is_tracing_val = is_kernel_trace && d->debug();
  is_script_debug_val = is_kernel_debug_script && d->debug();*/
}

/*****************************************************************************
 *  DEBUGGING INFORMATION                                                    *
 *****************************************************************************/
// The kernel uses these to decide what information to post
// debug_id specifies which node is being debugged.
int debug_id = -1; // -1 means all: we'll set it there and modulate is_X
int is_debugging_val = 0;
int is_tracing_val = 0;
int is_script_debug_val = 0;


/*****************************************************************************
 *  KERNEL HARDWARE CALLOUTS                                                 *
 *****************************************************************************/

void reinitHardware(void) { return; }

void mov (Tuple val)
{ hardware->patch_table[MOV_FN]->mov(val); }
void flex (Number val) 
{ hardware->patch_table[FLEX_FN]->flex(val); }
void set_probe (Data d, uint8_t p) 
{ hardware->patch_table[SET_PROBE_FN]->set_probe(d,p); }

void set_dt (Number dt)
{ hardware->patch_table[SET_DT_FN]->set_dt(dt);}

Number read_radio_range () 
{ return hardware->patch_table[READ_RADIO_RANGE_FN]->read_radio_range(); }
Number read_bearing () 
{ return hardware->patch_table[READ_BEARING_FN]->read_bearing(); }
Number read_speed () 
{ return hardware->patch_table[READ_SPEED_FN]->read_speed(); }

int radio_send_export (uint8_t version, Array<Data> const & data) {
  return hardware->patch_table[RADIO_SEND_EXPORT_FN]->
    radio_send_export(version,data); 
}
int radio_send_script_pkt (uint8_t version, uint16_t n, uint8_t pkt_num, 
                           uint8_t *script) {
return hardware->patch_table[RADIO_SEND_SCRIPT_PKT_FN]->
  radio_send_script_pkt(version,n,pkt_num,script);
}
int radio_send_digest (uint8_t version, uint16_t script_len, uint8_t *digest) {
  return hardware->patch_table[RADIO_SEND_DIGEST_FN]->
    radio_send_digest(version,script_len,digest);
}

extern void my_platform_operation(uint8_t op); 
void platform_operation(uint8_t op) { my_platform_operation(op); }

/*****************************************************************************
 *  MEMORY MANAGEMENT                                                        *
 *****************************************************************************/
/*
// the simulator grants only a fixed-size block of memory to each machine
int MAX_MEM_SIZE = 4*4096;
uint8_t* MEM_CONS(MACHINE *m) {
  m->memlen = MAX_MEM_SIZE;
  m->membuf = (uint8_t*)calloc(m->memlen,1);
  if(m->membuf==NULL) uerror("Malloc failed for membuf!");
  m->saved_memptr = m->memptr = 0;
  return m->membuf;
}
void MEM_GROW (MACHINE *m) { uerror("OUT OF MEM"); }

MACHINE* allocate_machine() {
  MACHINE* vm = (MACHINE*)calloc(1,sizeof(MACHINE));
  MEM_CONS(vm);
  return vm;
}
void deallocate_machine(MACHINE** vm) {
  FREE(&(*vm)->membuf); 
  FREE(vm);
}
*/
/*****************************************************************************
 *  PRETTY-PRINTING                                                          *
 *****************************************************************************/
/*
void post_data_to2 (char *str, DATA *d, int verbosity) {
  char buf[100];
  if (d->is_dead) { // Note: we should never see "DEAD" markers
    sprintf(buf, "(DEAD "); strcat(str, buf); 
  }
  switch (d->tag) {
  case NUM_TAG: {
    sprintf(buf, "%.2f", NUM_GET(d)); strcat(str, buf); break; }
  case FUN_TAG: { // Note: a function is just an ID
    sprintf(buf, "F%d", FUN_GET(d)); strcat(str, buf); break; }
  case VEC_TAG: {
    int i;
    VEC_VAL *v = VEC_GET(d);
    if(verbosity>0) strcat(str, "[");
    for (i = 0; i < v->n; i++) {
      if (i != 0) strcat(str, " ");
      post_data_to2(str, &v->elts[i], verbosity);
    }
    if(verbosity>0) strcat(str, "]");
    break; }
  }
  if (d->is_dead) {
    sprintf(buf, ")"); strcat(str, buf); 
  }
}

void post_stripped_data_to (char *str, DATA *d) {
  strcpy(str, "");
  post_data_to2(str, d, 0);
}

void post_data_to (char *str, DATA *d) {
  strcpy(str, "");
  post_data_to2(str, d, 1);
}

void post_data (DATA *d) {
  char buf[256];
  post_data_to(buf, d);
  post(buf);
}*/

void post_data_to2(char * str, Data data, int verbosity) {
	switch(data.type()){
		case Data::Type_undefined:
			strcpy(str, "(UNDEFINED)");
			break;
		case Data::Type_number:
			sprintf(str, "%.2f", data.asNumber());
			break;
		case Data::Type_tuple: {
			char buf[100];
			strcpy(str, verbosity ? "[" : "");
			for(size_t i = 0; i < data.asTuple().size(); i++){
				if (i) strcat(str, " ");
				post_data_to2(buf, data.asTuple()[i], verbosity);
				strcat(str, buf);
			}
			if (verbosity) strcat(str, "]");
			break;
		}
		case Data::Type_address:
			strcpy(str, "(Address)");
			break;
	}
}

void post_data_to(char * str, Data data) {
	post_data_to2(str, data, 1);
}

void post_stripped_data_to(char * str, Data data) {
	post_data_to2(str, data, 0);
}

void post_data(Data data) {
	char buf[256];
	post_data_to(buf, data);
	post(buf);
}

