/* Proto compiler
Copyright (C) 2009, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

// During experimental development, the NeoCompiler will be worked on
// "off to the side" of the original, not replacing it.

#include "compiler.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>

#include <fstream>

#include "config.h"

#include "nicenames.h"
#include "plugin_manager.h"

using namespace std;

/*****************************************************************************
 *  COMPILATION SEQUENCE                                                     *
 *****************************************************************************/
// Order of compilation:
// 1. Read a file or string into an S-expression
// 2. Interpret the S-expression into dataflow graph elements
//   [Repeat steps 1 & 2 until all files & sexprs have been read]
// 3. Optimize the dataflow graph
// 4. Emit the dataflow graph as platform-specific code
// 5. Optimize the platform-specific code

// Error behavior: in any given stage, do all the independent processing
// that's possible, flag if there's an error, and then quit

// len is filled in w. output length... eventually
uint8_t* NeoCompiler::compile(const char *str, int* len) {
  last_script=str;
  V1 << "Parsing expression...\n";
  compile_phase = "parsing"; // PHASE: text-> sexpr
  SExpr* sexpr = read_sexpr("command-line",str);
  compiler_error|=!sexpr; terminate_on_error();
  V1 << "Interpreting parsed expression into DFG...\n";
  compile_phase = "interpretation"; // PHASE: sexpr -> IR
  interpreter->interpret(sexpr); // terminates on error internally
  if(interpreter->dfg->output==NULL)
    { compile_error("Program has no content."); terminate_on_error(); }
  if(is_dump_interpreted) interpreter->dfg->print(cpout);
  if(is_dump_dotfiles) {
    ofstream dotstream((dotstem+".interpreted.dot").c_str());
    if(dotstream.is_open()) interpreter->dfg->printdot(&dotstream,is_dotfields);
  }
  if(is_early_terminate==3) 
    { *cperr << "Stopping before analysis" << endl; exit(0); }
  
  V1 << "Analyzing and optimizing DFG...\n";
  compile_phase = "analysis"; // PHASE: IR manipulation
  analyzer->transform(interpreter->dfg); // terminates on error internally
  if(is_dump_analyzed) interpreter->dfg->print(cpout);
  if(is_dump_dotfiles) {
    ofstream dotstream((dotstem+".analyzed.dot").c_str());
    if(dotstream.is_open()) interpreter->dfg->printdot(&dotstream,is_dotfields);
  }
  if(is_early_terminate==2) 
    { *cperr << "Stopping before localization" << endl; exit(0); }
  
  compile_phase = "legality check"; // PHASE: legality check
  IRPropagator *p = new CheckTypeConcreteness();
  p->propagate(interpreter->dfg);
  
  V1 << "Global-to-local transformation of DFG...\n";
  compile_phase = "localization"; // PHASE: Global-to-local transformation
  localizer->transform(interpreter->dfg); // terminates on error internally
  if(is_dump_raw_localized) interpreter->dfg->print(cpout);
  if(is_dump_dotfiles) {
    ofstream dotstream((dotstem+".rawlocalized.dot").c_str());
    if(dotstream.is_open()) interpreter->dfg->printdot(&dotstream,is_dotfields);
  }
  V1 << "Analyzing and optimizing localized DFG\n";
  compile_phase = "local analysis"; // PHASE: IR manipulation
  analyzer->transform(interpreter->dfg); // terminates on error internally
  if(is_dump_localized) interpreter->dfg->print(cpout);
  if(is_dump_dotfiles) {
    ofstream dotstream((dotstem+".localized.dot").c_str());
    if(dotstream.is_open()) interpreter->dfg->printdot(&dotstream,is_dotfields);
  }
  if(is_early_terminate==1) 
    { *cperr << "Stopping before emission" << endl; exit(0); }
  
  V1 << "Emitting DFG to executable form...\n";
  compile_phase = "emission"; // PHASE: code emission
  return emitter->emit_from(interpreter->dfg, len);
}

/*****************************************************************************
 *  COMPILER OBJECT API                                                      *
 *****************************************************************************/
NeoCompiler::NeoCompiler(Args* args) : Compiler(args) {
  is_dump_all = args->extract_switch("-CDall");
  is_dump_interpreted = args->extract_switch("-CDinterpreted") | is_dump_all;
  is_dump_analyzed = args->extract_switch("-CDanalyzed") | is_dump_all;
  is_dump_raw_localized = args->extract_switch("-CDraw-localized")|is_dump_all;
  is_dump_localized = args->extract_switch("-CDlocalized") | is_dump_all;
  is_dump_code = args->extract_switch("--instructions") | is_dump_all;
  is_dump_dotfiles = args->extract_switch("--dump-dotfiles");
  is_dotfields = args->extract_switch("--dot-field-nodes");
  dotstem = args->extract_switch("--dotstem")?args->pop_next():"ir";
  is_early_terminate = (args->extract_switch("--no-emission") ? 1 : 0);
  if(args->extract_switch("--no-localization")) is_early_terminate = 2;
  if(args->extract_switch("--no-analysis")) is_early_terminate = 3;
  last_script="";
  paranoid = args->extract_switch("--paranoid");
  infile = (args->extract_switch("--infile"))?args->pop_next():"";
  verbosity = (args->extract_switch("--verbosity")?args->pop_int():0);
  
  // Set up paths
  // srcdir is an undocumented option used for uninstalled execution
  string srcdir = (args->extract_switch("--srcdir"))?args->pop_next():"";
  if (args->extract_switch("--basepath"))
    proto_path.add_to_path(args->pop_next());
  else 
    proto_path.add_default_path(srcdir);
  while(args->extract_switch("-path",false)) // can extract multiple times
    proto_path.add_to_path(args->pop_next());
  
  interpreter = new ProtoInterpreter(this,args);
  analyzer = new ProtoAnalyzer(this,args);
  localizer = new GlobalToLocal(this,args);

  emitter = NULL;
}

NeoCompiler::~NeoCompiler() {
  delete interpreter;
}

static int
os_mkdir(const char *pathname)
{
#ifdef _WIN32
  return mkdir(pathname);
#else
  return mkdir(pathname, ACCESSPERMS);
#endif
}

// When being run standalone, -D controls dumping (it's normally
// consumed by the simulator).  Likewise, if -dump-stem is present,
// then dumping goes to a file instead of stdout.
void
NeoCompiler::init_standalone(Args *args)
{
  compiler_test_mode = args->extract_switch("--test-mode");
  is_dump_code |= args->extract_switch("-D");
  bool dump_to_stdout = true;
  const char *dump_dir = "dumps", *dump_stem = "dump";
  if (args->extract_switch("-dump-dir"))
    { dump_dir = args->pop_next(); dump_to_stdout = false; }
  if (args->extract_switch("-dump-stem"))
    { dump_stem = args->pop_next(); dump_to_stdout = false; }
  if (dump_to_stdout) {
    cpout = &cout;
  } else {
    char buf[1000];
    // Ensure that the dump directory exists.  EEXIST is too coarse,
    // but it will do for now.
    if (0 != os_mkdir(dump_dir) && errno != EEXIST)
      uerror("Unable to create dump directory %s", dump_dir);
    snprintf(buf, sizeof buf, "%s/%s.log", dump_dir, dump_stem);
    cpout = new ofstream(buf);
  }
  if (compiler_test_mode)
    cperr = cpout;

  // Default is to emit for ProtoKernel.
  if (args->extract_switch("-EM")) {
    // This is a static cast rather than a dynamic cast because
    // get_compiler_plugin yields a void *, not a pointer to class.
    emitter =
      static_cast<CodeEmitter *>
        (plugins.get_compiler_plugin
         (EMITTER_PLUGIN, args->pop_next(), args, this));
    if (emitter == NULL)
      uerror("Emitter not available");
  } else {
    ProtoKernelEmitter *kemitter = new ProtoKernelEmitter(this, args);
    kemitter->init_standalone(args);
    emitter = kemitter;
  }

  // In internal-tests mode: run each internal test, then exit.
  if (args->extract_switch("--internal-tests")) {
    type_system_tests();
    exit(0);
  }
}

void
NeoCompiler::set_platform(const string &path)
{
  cerr << "WARNING: NeoCompiler platform handling not yet implemented\n";
}

void
NeoCompiler::setDefops(const string &defops)
{
  if(emitter->isA("ProtoKernelEmitter")) {
    ((ProtoKernelEmitter*)emitter)->setDefops(defops);
  } else {
    cerr << "WARNING: don't know how to load operators for non-VM emitters";
  }
}

// Neocompiler is case sensitive, paleocompiler is not
bool SE_Symbol::case_insensitive = false;
