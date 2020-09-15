/* Header for GRN type management utilities 
Copyright (C) 2009-2011, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

#ifndef __GRN_UTILITIES__
#define __GRN_UTILITIES__

#include "grn.h"

namespace grn {

/***** Name utilies *****/
string sanitized_name(string name);

/***** Type utilies *****/
extern bool is_constitutive_true(ProtoType* pt);
extern bool is_constitutive_false(ProtoType* pt);
extern bool is_high_promoter(Promoter* p);
extern bool is_low_promoter(Promoter* p);

extern bool equivalent_promoters(Promoter* p1, Promoter* p2);
extern bool equivalent_regulation(DNAComponent* a, DNAComponent* b);
extern string scalar_to_str(ProtoScalar* s);
extern float force_promoter_rate(Promoter* p);

extern void convert_sexp_to_type(SExpr* type, Chemical* chem, OperatorInstance* oi);

extern ProtoType* get_chemical_type(Chemical* chem);

/***** GRN comparison and editing *****/
extern bool one_to_one(Chemical* c);
extern bool sole_regulator(ExpressionRegulation* er);
extern void delete_feature(DNAComponent* dc);

extern int estimate_diameter(GRN* net);

/***** Propagator for walking GRNs *****/
class GRNPropagator : public CompilationElement {
 public:
  // behavior variables
  int verbosity;
  int loop_abort; // # equivalent passes through worklist before assuming loop
  // propagation work variables
  set<Chemical*,CompilationElement_cmp> worklist_c;
  set<DNAComponent*,CompilationElement_cmp> worklist_f;
  bool any_changes;
  GRN* grn;
  
  GRNPropagator(int abort=10) { loop_abort=abort; verbosity = 0;}
  bool propagate(GRN* net); // fill worklist, then act until empty
  virtual void preprop() {} virtual void postprop() {} // hooks
  // action routines to be filled in by inheritors
  virtual void act(Chemical* c) {}
  virtual void act(FunctionalUnit* fu) {}
  // note_change: adds neighbors to the worklist
  void note_change(Chemical* c);
  void note_change(FunctionalUnit* fu);
 private:
  void queue_nbrs(Chemical* c, int marks=0);
  void queue_nbrs(FunctionalUnit* c, int marks=0);
};

/***** Certify Backpointers *****/
/*****************************************************************************
 *  INTEGRITY CERTIFICATION                                                  *
 *****************************************************************************/

class GRNCertifyBackpointers : public GRNPropagator {
 public:
  bool bad;
  GRNCertifyBackpointers(int verbosity) { this->verbosity = verbosity; }
  virtual void print(ostream* out=0) { *out << "CertifyBackpointers"; }
  void preprop() { bad=false; }
  void postprop() {
    if(bad) {
      ierror("GRN data structure corrupted: backpointer certification failed");
    }
  }

  void act(Chemical* c);
  void act(FunctionalUnit* fu);
};

}

#endif // __GRN_UTILITIES__
