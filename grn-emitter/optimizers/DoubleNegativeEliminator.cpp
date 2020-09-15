/* Optimizers
Copyright (C) 2009-2011, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

#include "config.h"
#include "grn_optimizers.h"

DoubleNegativeEliminator::DoubleNegativeEliminator(GRNEmitter* parent, Args* args) {
  verbosity = args->extract_switch("--double-negative-verbosity") ? 
    args->pop_number() : parent->verbosity;
}

// one-one chemical relation returns producer CDS; else returns null.
CodingSequence* DoubleNegativeEliminator::isInverter(FunctionalUnit* fu, bool strict) {
  // Find the input
  ExpressionRegulation* input=NULL;
  for(int i=0;i<fu->sequence.size();i++) {
    for_set(ExpressionRegulation*,fu->sequence[i]->regulators,er) {
      if(input || !(*er)->repressor) return NULL; // multi-input or activator
      input=(*er);
    }
    if(fu->sequence[i]->isA("Promoter")) {
      Promoter* p = (Promoter*)fu->sequence[i];
      if (!is_high_promoter(p)) return NULL; // not a high promoter
    }
  }
  if(input==NULL) return NULL; // no inhibition
  if(!one_to_one(input->signal)) return NULL; // not 1-to-1
  // Check that there are no other outputs from source
  CodingSequence* pcs = *(input->signal->producers.begin());
  FunctionalUnit* parent = pcs->container;
  if(strict) {
    for(int i=0;i<parent->sequence.size();i++) {
      if(parent->sequence[i]->isA("CodingSequence") && 
         parent->sequence[i] != pcs)
        return NULL; // two products
    }
  }
  return pcs;
}

// A function unit is invertible if it has at least one coding
// sequence, and it all of its units are not constants, do not
// participate in reactions, are not produced elsewhere, and are
// the sole regulators of the functional units that they effect
bool invertible_products(FunctionalUnit* fu) {
  bool found = false;
  for(int i=0;i<fu->sequence.size();i++) {
    if(fu->sequence[i]->isA("CodingSequence")) {
      found = true;
      CodingSequence* cds = (CodingSequence*)fu->sequence[i];
      Chemical* c = cds->product;
      // motif constants may be special or have side effects: leave them alone
      if(c->attributes.count(":motif-constant")) return false;
      // must be produced by precisely this functional unit
      if(!(c->producers.size()==1)) return false;
      // must not participate in any reactions
      if(!(c->regulatorFor.size()==0 && c->regulatedBy.size()==0)) return false;
      // nothing it regulates can also be regulated by anything else
      for_set(ExpressionRegulation*,c->consumers,j) {
        FunctionalUnit* target_fu = (*j)->target->container;
        for(int k=0;k<target_fu->sequence.size();k++) {
          for_set(ExpressionRegulation*,target_fu->sequence[k]->regulators,tr) {
            if(*tr != *j) return false;
          }
        }
      }
    }
  }
  return found; // products only invertible if they exist
}

void DoubleNegativeEliminator::act(FunctionalUnit* fu) {
  // Both cases of application involve a single "inverter" relation:
  CodingSequence* parent_cds = isInverter(fu,true); if(!parent_cds) return;
  FunctionalUnit* parent = parent_cds->container;

  // Case 1: two inverters in a row
  CodingSequence* grandparent_cds = isInverter(parent,false); 
  if(grandparent_cds) {
    FunctionalUnit* grandparent = grandparent_cds->container;
    V2 << "Eliminating double negative: \n  " << grandparent->to_str()
       << "\n  " << parent->to_str() << "\n  " << fu->to_str() << "\n";
    // Rewire:
    grn->replace_inputs(fu,grandparent,grandparent_cds,verbosity);
    note_change(fu);
    return;
  }

  // Case 2: one inverter, whose products are not produced anywhere else
  // and which are not themselves regulated or motif constants
  if(invertible_products(fu)) {
    V2 << "Eliminating negative by inverting product regulation: \n  " 
       << parent->to_str() << "\n  " << fu->to_str() << "\n";
    // Rewire:
    grn->replace_inputs(fu,parent,NULL,verbosity);
    V3 << "Inverting product regulation on new sequence: \n   "<<fu->to_str()<<endl; 
    for(int i=0;i<fu->sequence.size();i++) {
      if(fu->sequence[i]->isA("CodingSequence")) {
        CodingSequence* cds = (CodingSequence*)fu->sequence[i];
        for_set(ExpressionRegulation*,cds->product->consumers,i) { 
          grn->invert_regulation((*i),verbosity);
          note_change((*i)->target->container);
        }
      }
    }
    note_change(fu);
    return;
  }
}
