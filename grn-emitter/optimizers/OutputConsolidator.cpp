/* Optimizers
Copyright (C) 2009-2011, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

#include "grn_optimizers.h"

// Merge identically-used outputs (a form of common subexpression elimination)
OutputConsolidator::OutputConsolidator(GRNEmitter* parent, Args* args) {
  verbosity = args->extract_switch("--output-consolidator-verbosity") ? 
    args->pop_number() : parent->verbosity;
}

void OutputConsolidator::act(FunctionalUnit* fu) {
  // For each output, check if it's single-source.
  // If multiple single-sources are found, then merge them.
  CodingSequence* ss = NULL;
  for(int i=0;i<fu->sequence.size();i++) {
    if(fu->sequence[i]->isA("CodingSequence")) {
      CodingSequence* pcs = (CodingSequence*)fu->sequence[i];
      if(pcs->product->attributes.count(":motif-constant")) 
        continue; // don't merge motif constants
      if(pcs->product->regulatorFor.size()>0
         || pcs->product->regulatedBy.size()>0)
        continue; // don't mess with reactions yet
      if(pcs->product->producers.size()==1) { // single-source
        if(ss==NULL) { ss=pcs;
        } else { // TODO: make this check for repressor/inducer match
          V2<<"Merging "<<pcs->product->to_str()<<" into "<<ss->product->to_str()<<endl;
          set<ExpressionRegulation*,CompilationElement_cmp> 
            deletesafe_consumers = pcs->product->consumers;
          for_set(ExpressionRegulation*,deletesafe_consumers,ri) {
            V3<<"Moving consumer "<<(*ri)->to_str()<<" from signal "<<(*ri)->signal->to_str()<<" to "<<ss->product->to_str()<<endl;
            FunctionalUnit* fu2 = (*ri)->target->container;
            (*ri)->signal = ss->product;

            V4<<"Checking consumer FU for duplicates: "<<fu2->to_str()<<endl;
            bool duplicate = false;
            for(int j=0;j<fu2->sequence.size();j++) {
              for_set(ExpressionRegulation*,fu2->sequence[j]->regulators,ri2) {
                if((*ri!=*ri2) && (*ri)->same_regulation(*ri2)) {
                  if (verbosity>=4) {(*ri)->print(cpout); *cpout<<" is a duplicate rr"<<endl;}
                  duplicate = true; // ignore the duplicate region
                }
              }
            }
            V4<<"Duplicate found? "<<b2s(duplicate)<<endl;
            if (!duplicate) {
              V4<<"Adding "<<(*ri)->target->to_str()<<" as consumer of "<<ss->product->to_str()<<endl;
              ExpressionRegulation* new_reg = new ExpressionRegulation(ss->product,(*ri)->target,(*ri)->repressor,(*ri)->strength,(*ri)->dissociation);
              ss->product->consumers.insert(new_reg);
            }
            
            delete *ri; // now discard the old consumer
            //note_change((*ri)->container); TODO: figure out why this crashes
            //note_change(fu2); TODO this doesn't work either!
          }
          V4<<"Discarding all consumers of "<<pcs->product->to_str()<<endl;
          pcs->product->consumers.clear();
          V3<<"All consumers of "<<pcs->product->to_str()<<" removed; deleting chemical."<<endl;
          grn->delete_chemical(pcs->product);
          note_change(fu);
        }
      }
    }
  }
}
