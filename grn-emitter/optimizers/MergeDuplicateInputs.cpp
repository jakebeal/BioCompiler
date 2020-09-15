/* Optimizers
Copyright (C) 2009-2011, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

#include "grn_optimizers.h"

// Merges functional units with same inputs and different outputs
MergeDuplicateInputs::MergeDuplicateInputs(GRNEmitter* parent, Args* args) {
  verbosity = args->extract_switch("--merge-duplicate-inputs-verbosity") ? 
    args->pop_number() : parent->verbosity;
}
  
// For this purpose: must have some promoters & expression regulation
bool MergeDuplicateInputs::input_eqv(FunctionalUnit* a,FunctionalUnit* b) {
  //*cpout<<"Checking equivalence to: "<<b->to_str()<<endl;
  // First, check for existence and equivalence of promoters
  set<int> a_promoters;
  for(int i=0;i<a->sequence.size();i++) {
    // track set of promoters
    if(a->sequence[i]->isA("Promoter")) {
      a_promoters.insert(i);
      // test for equivalent promoter in B
      if(i>=b->sequence.size() || !b->sequence[i]->isA("Promoter")
         || !equivalent_promoters((Promoter*)a->sequence[i], (Promoter*)b->sequence[i])) return false;
    }
  }
  //*cpout<<"  All A promoters matched in B"<<endl;
  if(a_promoters.size()==0) return false;
  for(int i=0;i<b->sequence.size();i++) {
    // make sure there are no B-promoters not in A
    if(b->sequence[i]->isA("Promoter")) 
      if(!a_promoters.count(i)) return false;
  }

  //*cpout<<"  Promoters same, checking CDS"<<endl;
  DNAComponent* first_cds = NULL;
  // Second, check for equivalence in all CDS regulations
  for(int i=0;i<a->sequence.size();i++) {
    if(a->sequence[i]->isA("CodingSequence")) {
      if(first_cds==NULL) {
        first_cds = a->sequence[i];
      } else {
        if(!equivalent_regulation(first_cds,a->sequence[i])) return false;
      }
    }
  }
  if(first_cds==NULL) return false; // no point in merging if no CDS
  for(int i=0;i<b->sequence.size();i++) {
    if(b->sequence[i]->isA("CodingSequence")) {
      if(!equivalent_regulation(first_cds,b->sequence[i])) return false;
    }
  }
  //*cpout<<"  Inputs same"<<endl;
  return true;
}
  
void MergeDuplicateInputs::act(FunctionalUnit* fu) {
  vector<DNAComponent*>::iterator ip = fu->sequence.begin();
  bool found = false;
  // find the insertion point:
  for( ; ip!=fu->sequence.end(); ip++) {
    if((*ip)->isA("CodingSequence")) found=true;
    else if((*ip)->isA("Terminator")) break;
  }
  if(!found) return; // don't insert to empty regions
    
  // TODO is there a way to do this all in one loop?
  // we want to find all the chemicals so we don't add a duplicate later!
  set<Chemical*,CompilationElement_cmp> chemicals;

  for(vector<DNAComponent*>::iterator chemi = fu->sequence.begin() ; chemi!=fu->sequence.end(); chemi++) {
    if((*chemi)->isA("CodingSequence"))
      chemicals.insert(((CodingSequence*)(*chemi))->product);
    else if((*ip)->isA("Terminator")) break;
  }
  
  V4<<"Checking for equivalence for: "<<fu->to_str()<<endl;
  // walk rest of FUs, looking for matches
  set<DNAComponent*>::iterator bi = grn->dnacomponents.begin();
  for( ;bi!=grn->dnacomponents.end();bi++) {
    if((*bi)==fu) continue; // can't merge with self
    FunctionalUnit* f2 = (FunctionalUnit*)*bi;
    if(input_eqv(fu,f2)) {
      V3<<"Merging with equivalent functional unit: "<<f2->to_str()<<endl;
      for(int i=0;i<f2->sequence.size();i++) {
        if(f2->sequence[i]->isA("CodingSequence")) {
          CodingSequence* pcs=(CodingSequence*)f2->sequence[i];
          Chemical* chem = pcs->product; 
            
          if (verbosity>=3) {
            *cpout<<"before chemical: "<<chem->name<<" with producers: ";
            set<CodingSequence*>::iterator ci=chem->producers.begin();
            for( ;ci!=chem->producers.end();ci++) {
              (*ci)->print(cpout);
              *cpout<<" ";
            }
            *cpout<<endl;
          }
            
          if (chemicals.count(chem) == 0) {
            // this is not a duplicate:
            CodingSequence* pcsCopy = 
              (CodingSequence*)pcs->clone();
            ip = fu->sequence.insert(ip,pcsCopy); 
            pcsCopy->container=fu;
              
            // update the list of chemicals!
            chemicals.insert(chem); note_change(chem);
          }
          else {
            V2<<"Not adding PCS because chemical: "<<chem->name<<" is already produced."<<endl;
          }
          
          // always delete the old one:
          delete_feature(pcs);
          delete pcs;

          if (verbosity>=3) {
            *cpout<<"after chemical: "<<chem->name<<" with producers: ";
            set<CodingSequence*>::iterator ci=chem->producers.begin();
            for( ;ci!=chem->producers.end();ci++) {
              (*ci)->print(cpout);
              *cpout<<" ";
            }
            *cpout<<endl;
          }
        }
      }
    }
  }
}
