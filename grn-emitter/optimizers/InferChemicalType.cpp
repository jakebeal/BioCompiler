/* Optimizers
Copyright (C) 2009-2011, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

#include "grn_optimizers.h"

// Chemical type inference
// Currently, this only changes Booleans to constant true/false
// Also, this only works on chemicals not regulated by other chemicals

GRNInferChemicalType::GRNInferChemicalType(GRNEmitter* parent, Args* args) {
  verbosity = args->extract_switch("--grn-chemical-type-inference-verbosity") ? 
    args->pop_number() : parent->verbosity;
}
void GRNInferChemicalType::act(Chemical* c) {
  if(!c->regulatedBy.empty()) return; // only handle simple TFs
  if(c->attributes.count(":motif-constant")) return; // ignore I/O

  // only if type is non-constant ProtoBoolean
  ProtoType* pt = get_chemical_type(c);
  if(pt==NULL || !pt->isA("ProtoBoolean")) return;
  ProtoBoolean* b=dynamic_cast<ProtoBoolean*>(pt);
  if(b->constant) return;
    
  // check if any producer is constitutive-high or all are constitutive-low
  bool not_const_low = false, const_high = false;
  for_set(CodingSequence*,c->producers,i) {
    FunctionalUnit* fu = (*i)->container;
    bool high_promoter = false, low_promoter = false, regulated = false;
    for(int i=0;i<fu->sequence.size();i++) {
      if(fu->sequence[i]->regulators.size()>0) 
        regulated = true;
      if(fu->sequence[i]->isA("Promoter")) {
        high_promoter |= is_high_promoter(((Promoter*)fu->sequence[i]));
        low_promoter |= is_low_promoter(((Promoter*)fu->sequence[i]));
      }
    }
    if(!low_promoter || regulated) not_const_low = true;
    if(high_promoter && !regulated) const_high = true;
  }

  ProtoType* newtype = NULL;
  // if any producer is constitutive-high, then type is True
  if(const_high) { newtype = new ProtoBoolean(true); }
  if(!not_const_low) { newtype = new ProtoBoolean(false); }
    
  if(newtype!=NULL) {
    if(verbosity>=2) *cpout<<"Inferred type "<<newtype->to_str()<<" for chemical "<<c->to_str()<<endl;
    c->attributes["type"] = new ProtoTypeAttribute(newtype);
    note_change(c);
  }
}
