/* Header for BioCompiler
Copyright (C) 2009-2011, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

#ifndef __BIOCOMPILER__
#define __BIOCOMPILER__

#include "config.h"
#include "grn.h"
#include "grn_utilities.h"

#define BIOCOMPILER_VERSION SVN_REVISION // Release version

namespace grn {

class GRNEmitter : public CodeEmitter, public IRPropagator {
  reflection_sub2(GRNEmitter, CodeEmitter, IRPropagator);

  string outstem; // stem for outputs, w. & w/o directory portion
  NeoCompiler* parent;
 public:
  string circuit_name;
  vector<GRNPropagator*> rules;
  vector<GRNPropagator*> postprocessing;
  int max_loops, verbosity;
  bool paranoid, emit_matlab,emit_intermediate, emit_sbol,emit_unoptimized,emit_dot,emit_to_stdout;
  bool plot_all_chemicals, plot_all_motif_constants; // all motif-constants or just I/O?
  // These need to be extracted into an abstract GRN:
  GRN grn;

 public:
  GRNEmitter(NeoCompiler* parent, Args* args);
  uint8_t* emit_from(DFG* g, int* len);

 private:
  void optimize();
  void post_process();

  void add_template(OperatorInstance *oi,vector<Chemical*> *in,vector<Chemical*> *outs);
  Chemical* parse_chemical(SExpr* def, map<string,Chemical*> *locals,OperatorInstance* oi);
  void interpret_template(OperatorInstance *oi,SE_List* s,vector<Chemical*> *ins, vector<Chemical*> *outs);

  void initialize_grn_optimizers(Args* args);
    // Output functions
  void to_matlab(string net_description);
  void to_sbol(ostream* out);
  void to_dot(ostream* out);
  void print(ostream *out) { *out << "GRN Emitter"; }
  ostream* get_stream_for(string fileclass,string extension);
};

}

#endif // __BIOCOMPILER__
