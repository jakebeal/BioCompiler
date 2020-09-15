/* Abstract genetic regulatory network representation
Copyright (C) 2009-2011, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

#include "config.h"
#include "grn.h"
#include "grn_utilities.h"

using namespace grn;

/*****************************************************************************
 *  REP FUNCTIONS THAT CAN'T BE DEFINED IN HEADER                            *
 *****************************************************************************/

void DNAComponent::print_regulators(ostream* out) { 
  for_set(ExpressionRegulation*, regulators, i) { (*i)->print(out); }
}

DNAComponent::~DNAComponent() {
  set<ExpressionRegulation*,CompilationElement_cmp> deletesafe_regulators = regulators;
  for_set(ExpressionRegulation*, deletesafe_regulators, i) { 
    (*i)->signal->consumers.erase(*i);
  }
}

DNAComponent* CodingSequence::clone() { 
  CodingSequence* cds = new CodingSequence(product);
  for_set(ExpressionRegulation*, regulators, i) 
    { cds->regulators.insert((*i)->clone(cds)); }
  return cds;
}

DNAComponent* Promoter::clone() { 
  Promoter* p  = new Promoter(rate); 
  for_set(ExpressionRegulation*, regulators, i) 
    { p->regulators.insert((*i)->clone(p)); }
  return p;
}

void GRN::print(ostream* out) {
  ostringstream dc_net;
  for_set(DNAComponent*,dnacomponents,i) { (*i)->print(&dc_net); dc_net<<endl; }
  *out<<"Genetic Regulatory Network:\n"<<dc_net.str();
}


/*****************************************************************************
 *  COMPLEX GRN MANIPULATION FUNCTIONS                                       *
 *****************************************************************************/

void GRN::delete_chemical(Chemical* c) {
  /// ADA: products! need this to check for the flag too!
  /// ADA: if(c->consumers.size() && c->producers.size()) ierror("Attempted to delete non-redundant chemical");
  if(c->consumers.size() || c->regulatorFor.size())
    ierror("Attempted to delete chemical that is still required by network");
  // delete all producer PCSs
  set<CodingSequence*> to_delete;
  set<CodingSequence*>::iterator pi=c->producers.begin();
  for( ;pi!=c->producers.end();pi++) 
    { delete_feature(*pi); to_delete.insert(*pi); }
  for_set(CodingSequence*,to_delete,pi) { delete *pi; }
  // delete all regulator reactions
  set<RegulatoryReaction*>::iterator ri=c->regulatedBy.begin();
  for( ;ri!=c->regulatedBy.end();ri++) delete (*ri);

  /// ADA delete all consumer RRs
  /// ADA set<RegulatoryRegion*>::iterator rr=c->consumers.begin();
  /// ADA for( ;rr!=c->consumers.end();rr++) delete_feature(*rr);

  chemicals.erase(c->name);
}
void  GRN::delete_functional_unit(FunctionalUnit* fu) {
  // change links w. chemicals
  for(int i=0;i<fu->sequence.size();i++) {
    for_set(ExpressionRegulation*, fu->sequence[i]->regulators, er) {
      (*er)->signal->consumers.erase(*er);
    }
    if(fu->sequence[i]->isA("CodingSequence")) {
      CodingSequence* pcs = (CodingSequence*)fu->sequence[i];
      pcs->product->producers.erase(pcs);
    }
  }
  dnacomponents.erase(fu);
}


// invert all regulation in entire functional unit
void GRN::invert_regulation(FunctionalUnit* target,int verbosity) {
  for(int i=0;i<target->sequence.size();i++) {
    for_set(ExpressionRegulation*, target->sequence[i]->regulators, er) {
      (*er)->repressor = !(*er)->repressor;
    }
    if(target->sequence[i]->isA("Promoter")) {
      Promoter* p = (Promoter*)target->sequence[i];
      if(is_high_promoter(p)) { p->rate = new ProtoBoolean(false); }
      else if(is_low_promoter(p)) { p->rate = new ProtoBoolean(true); }
      else {
        ierror("Tried to invert a non-Boolean promoter, but don't know how.");
      }
    }
  }
}

// invert regulation of just this er, recalculating promoters accordingly
void GRN::invert_regulation(ExpressionRegulation* er,int verbosity) {
  V4<<"Inverting regulation of: "<<er->to_str()<<endl;
  FunctionalUnit* fu = er->target->container;
  // first, invert the regulatory relationship
  er->repressor = !er->repressor;

  // next, adjust the promoter, if there is one
  Promoter* p = NULL;
  bool all_repressors = true;
  for(int i=0;i<fu->sequence.size();i++) {
    for_set(ExpressionRegulation*, fu->sequence[i]->regulators, er) {
      V4<<"Checking for repression: "<<(*er)->to_str()<<" = "<<b2s((*er)->repressor)<<endl;
      if(!(*er)->repressor) all_repressors = false;
    }
    if(fu->sequence[i]->isA("Promoter")) {
      V4<<"Found promoter "<<fu->sequence[i]->to_str()<<endl;
      if(p!=NULL) ierror("Tried to invert regulation on a multi-promoter complex, but don't know how");
      p = (Promoter*)fu->sequence[i];
    }
  }

  if(p!=NULL) {
    if(is_high_promoter(p) || is_low_promoter(p)) { 
      V4<<"Setting promoter type: "<<b2s(all_repressors)<<endl;
      p->rate = new ProtoBoolean(all_repressors);
    } else {
      ierror("Tried to invert a non-Boolean promoter, but don't know how.");
    }
  }
}


// Replace-inputs only works if regulation relations are consistent
// for all CDSs
void GRN::replace_inputs(FunctionalUnit* dst, FunctionalUnit* src,DNAComponent* src_cds, int verbosity) {
  V4<<"replace_inputs finding input portions of source: "<<src->to_str()<<endl;
  vector<DNAComponent*> newseq;
  vector<ExpressionRegulation*> cds_regs;
  DNAComponent* first_cds = NULL;
  // copy input portion of src
  for(int i=0;i<src->sequence.size();i++) {
    if(src->sequence[i]->isA("Promoter")) {
      V4<<"Cloning promoter: "<<src->sequence[i]->to_str()<<endl;
      newseq.push_back(((Promoter*)src->sequence[i])->clone());
    } else if(src->sequence[i]->isA("CodingSequence")) {
      // Only copy regulations from src_cds
      if(src->sequence[i]==src_cds || src_cds==NULL) {
        if(first_cds==NULL) {
          V4<<"Copying CDS regulations: "<<src->sequence[i]->to_str()<<endl;
          for_set(ExpressionRegulation*, src->sequence[i]->regulators, er)
            { cds_regs.push_back(*er); }
          first_cds = src->sequence[i];
        } else { // Subsequence CDS: confirm equality
          V4<<"Verifying equality of CDS regulations: "<<src->sequence[i]->to_str()<<endl;
          if(!equivalent_regulation(first_cds,src->sequence[i]))
            ierror("Tried to replace inputs on functional unit using differently regulated coding sequences");
        }
      }
    }
  }
  
  V4<<"replace_inputs finding non-input portions of: "<<dst->to_str()<<endl;
  // copy non-input portion of dst
  for(int i=0;i<dst->sequence.size();i++) {
    if(dst->sequence[i]->isA("Promoter")) {
      V4<<"Deleting old promoter: "<<dst->sequence[i]->to_str()<<endl;
      delete dst->sequence[i]; // kill old, GCing its net connections
    } else {
      newseq.push_back(dst->sequence[i]); // copy non-input portion
      if(dst->sequence[i]->isA("CodingSequence")) {
        V4<<"Replacing regulation on: "<<dst->sequence[i]->to_str()<<endl;
        // kill old regulations
        set<ExpressionRegulation*,CompilationElement_cmp> deletesafe_regulators = dst->sequence[i]->regulators;
        for_set(ExpressionRegulation*, deletesafe_regulators, er)
          { delete *er; }
        // replace with new regulations
        for(int j=0;j<cds_regs.size();j++) {
          ExpressionRegulation* er = cds_regs[j];
          V4<<"Adding regulation: "<<er->to_str()<<endl;
          dst->sequence[i]->regulators.insert(er->clone(dst->sequence[i]));
        }
      }
    }
  }

  V4<<"put new sequence into destination: "<<endl;
  // pop the new sequence in place
  dst->sequence.clear();
  for(int i=0;i<newseq.size();i++) dst->add(newseq[i]);
  V4<<"new functional unit: "<<dst->to_str()<<endl;
}


/*****************************************************************************
 *  INTEGRITY CERTIFICATION                                                  *
 *****************************************************************************/

void GRNCertifyBackpointers::act(Chemical* c) {
  if(verbosity>=4) *cpout<<"Certifying chemical "<<c->to_str()<<endl;
  for_set(ExpressionRegulation*, c->consumers, ci) {
    if((*ci)->signal==NULL) ierror(c,"GRN data structure corrupted: null consumer backpointer");
    if((*ci)->target==NULL) ierror(c,"GRN data structure corrupted: null consumer target");
    if((*ci)->signal!=c)
      { bad=true; ierror(c,"GRN data structure corrupted: bad consumer backpointer: "+c->to_str());}
    if((*ci)->target->container==NULL) ierror(c,"GRN data structure corrupted: null target container");
    if(!grn->dnacomponents.count((*ci)->target->container)) {
      bad=true;
      ierror(c,"GRN data structure corrupted: ghost consumer for "+c->to_str()+" in "+(*ci)->target->container->to_str());
    }
  }
  for_set(CodingSequence*, c->producers, pi) {
    if((*pi)->product!=c)
      { bad=true; ierror(c,"GRN data structure corrupted: bad producer backpointer: "+c->to_str());}
    if(!grn->dnacomponents.count((*pi)->container)) {
      bad=true; string tmp = (*pi)->container->to_str();
      ierror(c,"GRN data structure corrupted: ghost producer: "+tmp+" for "+c->to_str());
    }
  }
  for_set(RegulatoryReaction*, c->regulatorFor, ri) {
    if((*ri)->regulator!=c) {
      bad=true;
      ierror(c,"GRN data structure corrupted: bad regulation backpointer: "+c->to_str());
    }
    if(!grn->reactions.count(*ri)) {
      bad=true; string tmp = (*ri)->to_str();
      ierror(c,"GRN data structure corrupted: ghost regulation: "+tmp+" for "+c->to_str());
    }
  }
  for_set(RegulatoryReaction*, c->regulatedBy, ri) {
    if((*ri)->substrate!=c) {
      bad=true;
      ierror(c,"GRN data structure corrupted: bad regulator backpointer: "+c->to_str());
    }
    if(!grn->reactions.count(*ri)) {
      bad=true; string tmp = (*ri)->to_str();
      ierror(c,"GRN data structure corrupted: ghost regulator: "+tmp+" for "+c->to_str());
    }
  }
  // check to make sure the chemical has a type:
  if (!(c->attributes["type"])) 
    { bad=true; ierror("GRN data structure corrupted: chemical missing its type: "+c->to_str()); }
}
  
void GRNCertifyBackpointers::act(FunctionalUnit* fu) {
  if(verbosity>=4) *cpout<<"Certifying functional unit "<<fu->to_str()<<endl;
  for(int i=0;i<fu->sequence.size();i++) {
    for_set(ExpressionRegulation*,fu->sequence[i]->regulators,er) {
      if((*er)->signal==NULL) ierror(fu,"GRN data structure corrupted: null regulator backpointer");
      if((*er)->target==NULL) ierror(fu,"GRN data structure corrupted: null regulator target");
      if((*er)->target!=fu->sequence[i]) ierror(fu,"GRN data structure corrupted: bad target backpointer: "+(*er)->to_str());
      if(!(*er)->signal->consumers.count(*er))
        { bad=true; ierror(fu,"GRN data structure corrupted: bad regulation backpointer: "+(*er)->to_str()); }
    }
    if(fu->sequence[i]->container!=fu)
      { bad=true; ierror(fu,"GRN data structure corrupted: bad functional-unit backpointer: "+fu->to_str()); }
    if(fu->sequence[i]->isA("CodingSequence")) {
      CodingSequence* pcs = (CodingSequence*)fu->sequence[i];
      if(!pcs->product->producers.count(pcs))
        { bad=true; ierror(fu,"GRN data structure corrupted: bad coding sequence backpointer: "+pcs->to_str()); }
    }
  }
}
