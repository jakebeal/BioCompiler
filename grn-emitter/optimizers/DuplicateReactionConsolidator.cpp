/* Optimizers
Copyright (C) 2009-2011, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

#include "grn_optimizers.h"

// Merge identically-used outputs (a form of common subexpression elimination)
DuplicateReactionConsolidator::DuplicateReactionConsolidator(GRNEmitter* parent, Args* args) {
  verbosity = args->extract_switch("--duplicate-reaction-consolidator-verbosity") ? 
    args->pop_number() : parent->verbosity;
}

bool same_reaction(RegulatoryReaction* r1, RegulatoryReaction* r2) {
  if(r1->regulator != r2->regulator) return false;
  if(r1->substrate != r2->substrate) return false;
  if(r1->repressor != r2->repressor) return false;
  if(r1->dissociation != r2->dissociation) return false;
  if(r1->alpha != r2->alpha) return false;
  return true;
}

void DuplicateReactionConsolidator::act(Chemical* c) {
  // look through the set of reactions, searching for identical reactions
  set<RegulatoryReaction*> to_delete;
  for_set(RegulatoryReaction*,c->regulatedBy,i) {
    for_set(RegulatoryReaction*,c->regulatedBy,j) {
      if(to_delete.count(*i)) continue; // only check each once
      if(*i==*j) continue; // ignore self
      if(same_reaction(*i,*j))
        to_delete.insert(*j);
    }
  }
  for_set(RegulatoryReaction*,to_delete,i) { 
    c->regulatedBy.erase(*i);
    (*i)->regulator->regulatorFor.erase(*i);
    grn->reactions.erase(*i);
    delete *i;
  }
  if(to_delete.size()) {
    note_change(c);
  }
}
