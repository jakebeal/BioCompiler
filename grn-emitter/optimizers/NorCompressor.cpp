/* Optimizers
Copyright (C) 2009-2011, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

#include "grn_optimizers.h"

// Biology-specific chemical compression of NORs
NorCompressor::NorCompressor(GRNEmitter* parent, Args* args) {
  verbosity = args->extract_switch("--nor-compressor-verbosity") ? 
    args->pop_number() : parent->verbosity;
}

Chemical* NorCompressor::is_Or(FunctionalUnit* fu) {
  Chemical* input = NULL;
  for(int i=0;i<fu->sequence.size();i++) { 
    for_set(ExpressionRegulation*,fu->sequence[i]->regulators,er) {
      if(input!=NULL) return NULL; // can only be one region
      // Make sure it's a live, non-reaction boolean activator 
      if((*er)->signal->producers.size()==0
         || (*er)->signal->regulatedBy.size()>0
         || (*er)->signal->regulatorFor.size()>0 || (*er)->repressor)
        return NULL;
      input = (*er)->signal; // continue to ensure it's the only one
    }
  }
  return input; // must be at least one RR
}
  
void NorCompressor::act(Chemical* c) {
  // Is chemical a 1-1 repressor, with an OR input?
  if(!one_to_one(c)) return;
  if(!(*c->consumers.begin())->repressor) return;
  FunctionalUnit* fu = (*c->producers.begin())->container;
  Chemical* or_input = is_Or(fu);
  if(or_input==NULL) return;
    
  V2<<"Compressing NOR on chemical "<<c->to_str()<<endl;
  // If so, then replace old producer with producers at OR inputs
  CodingSequence* oldpcs = *c->producers.begin();
  for_set(CodingSequence*,or_input->producers,i) {
    FunctionalUnit* fu = (*i)->container;
    vector<DNAComponent*>::iterator ip = fu->sequence.begin();
    for( ; ip!=fu->sequence.end(); ip++) {
      if((*ip)==(*i)) {
        CodingSequence* pcs = (CodingSequence*)oldpcs->clone();
        V3<<"Inserting "<<ce2s(pcs)<<" after "<<ce2s(*ip)<<" in "<<ce2s(fu)<<endl;
        fu->sequence.insert(ip,pcs); pcs->container=fu;
        note_change(fu); break;
      }
    }
  }
  V3<<"Deleting old producer of "<<ce2s(c)<<" in "<<ce2s(oldpcs)<<endl;
  note_change(oldpcs->container);
  delete_feature(oldpcs); c->producers.erase(oldpcs);
}

