/* Standalone compiler app
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "config.h"
#include "proto_version.h"
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#if USE_NEOCOMPILER
#include "compiler.h"
#else
#include "paleocompiler.h"
#endif
#include "plugin_manager.h"
#define Instruction InstructionX
#include "DefaultsPlugin.h"
#undef Instruction

void run_test_suite(); // testing kludge

int main (int argc, char *argv[]) {
  Args *args = new Args(argc,argv); // set up the arg parser
  if(!args->find_switch("--json")) { // don't print block in web mode
    post("PROTO v%s%s (Kernel %s) (Developed by MIT Space-Time Programming Group 2005-2008)\n",
	 PROTO_VERSION,
#if USE_NEOCOMPILER
	 "[neo]",
#else
	 "[paleo]",
#endif
	 KERNEL_VERSION);
  }
  plugins.ensure_initialized(args);
  
#if USE_NEOCOMPILER
  NeoCompiler* neocompiler = new NeoCompiler(args);
  neocompiler->init_standalone(args);
#else
  PaleoCompiler* compiler = new PaleoCompiler(args);
  compiler->init_standalone(args);
#endif

  if(args->extract_switch("--test")) { run_test_suite(); exit(0); }

  // load the script
  int len;
  if(args->argc==1) {
#if USE_NEOCOMPILER
  if(neocompiler->infile.length()>0) {
	ifstream fileStream(neocompiler->infile.c_str());
	if(fileStream.is_open()) {
	  string line;
      getline(fileStream, line);
	  uint8_t* s = neocompiler->compile(line.c_str(),&len);
	} else {
      uerror("Could not open input file");
	}
  } else {
	uerror("Not provided anything to compiler");
  }
#else
    uint8_t* s = compiler->compile("(app)",&len);
#endif
  } else if(args->argc==2) {
#if USE_NEOCOMPILER
    uint8_t* s = neocompiler->compile(args->argv[args->argc-1],&len);
#else
    uint8_t* s = compiler->compile(args->argv[args->argc-1],&len);
#endif
  } else {
    post("Error: %d unhandled arguments:",args->argc-2);
    for(int i=2;i<args->argc;i++) post(" '%s'",args->argv[i-1]);
    post("\n");
  }

  exit(0);
}


/// test suite!
extern void test_compiler_utils();

void run_test_suite() {
  test_compiler_utils();
}

// Touch the palette element to be sure that it gets linked in on
// systems with a linker too smart for its own good
void* palette = NULL;
