/* Optimizers
Copyright (C) 2009-2011, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

#include "grn_optimizers.h"

GRNCopyPropagator::GRNCopyPropagator(GRNEmitter* parent, Args* args) {
  verbosity = args->extract_switch("--grn-copy-propagator-verbosity") ? 
    args->pop_number() : parent->verbosity;
}

void GRNCopyPropagator::act(Chemical* c) {
  // Is chemical is 1-1 inducer?
  if(!one_to_one(c)) return;
  if((*c->consumers.begin())->repressor) return;
  // Is consumer regulated only by the one chemical?
  if(!sole_regulator(*c->consumers.begin())) return;
  // If so, then replace it (and only it) in consumer's inputs
  V2<<"Copy propagating on chemical "<<c->to_str()<<endl;
  ExpressionRegulation* er = *c->consumers.begin();
  // er will be invalid after the replace_inputs call!
  FunctionalUnit* fu = er->target->container;
  CodingSequence* pcs = *c->producers.begin();
  V3<<"replacing src: "<<pcs->to_str()<<" destination: "<<er->to_str()<<endl;
  
  grn->replace_inputs(fu,pcs->container,pcs,verbosity);
  // This kills the old regulations and thus er is no longer valid. 

  note_change(fu);
}
