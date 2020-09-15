/* Optimizers
Copyright (C) 2009-2011, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

#include "grn_optimizers.h"

GRNDeadCodeEliminator::GRNDeadCodeEliminator(GRNEmitter* parent, Args* args) {
  verbosity = args->extract_switch("--grn-dead-code-eliminator-verbosity") ? 
    args->pop_number() : parent->verbosity;
}

void GRNDeadCodeEliminator::act(Chemical* c) {
  // Delete chemicals with no consumers or regulation activity
  if(c->consumers.size()>0 || c->regulatorFor.size()>0) return;
  if(c->attributes.count(":motif-constant")) return; // has side effects
  // delete it
  if(verbosity>=2) *cpout<<"Eliminating unused chemical "<<c->to_str()<<endl;
  note_change(c);
  grn->delete_chemical(c);
  // ADA if (c->consumers.size()>0) {
  // ADA   if (c->producers.size()==0) {
  // ADA     // has consumers, but no producers
  // ADA     // if it does not have some flag, then we delete it:
  // ADA     if (true) { // foo need to check for :side-effect here!
  // ADA       if(verbosity>=2) *cpout<<"Eliminating chemical that is not produced "<<c->to_str()<<endl;
  // ADA       note_change(c);
  // ADA       grn->delete_chemical(c);
  // ADA     }
  // ADA   }
  // ADA   else
  // ADA     return; // has consumers
  // ADA }
  // ADA else {
  // ADA 
  // ADA   if(c->attributes.count(":motif-constant")) return; // has side effects
  // ADA   // delete it
  // ADA   if(verbosity>=2) *cpout<<"Eliminating unused chemical "<<c->to_str()<<endl;
  // ADA   note_change(c);
  // ADA   grn->delete_chemical(c);
  // ADA }
}

void GRNDeadCodeEliminator::act(FunctionalUnit* fu) {
  // Delete regulations whose signals cannot be produced
  for(int i=0;i<fu->sequence.size();i++) {
    // collect elements to delete
    set<ExpressionRegulation*> deletes;
    for_set(ExpressionRegulation*,fu->sequence[i]->regulators,er) {
      if((*er)->signal->producers.size()==0 && // not producible
         !(*er)->signal->attributes.count(":motif-constant")) { // and not I/O
        if(verbosity>=2) *cpout<<"Eliminating unused regulatory region "<<(*er)->to_str()<<" in "<<fu->to_str()<<endl;
        deletes.insert(*er);
      }
    }
    // delete the collection of marked elements
    for_set(ExpressionRegulation*,deletes,er) {
      (*er)->signal->consumers.erase(*er);
      (*er)->target->regulators.erase(*er);
      delete *er;
    }
  }
  
  bool product = false, input = false, low_promoter=false, promoter = false;
  // Delete FUs with no products
  for(int i=0;i<fu->sequence.size();i++) {
    if(fu->sequence[i]->isA("CodingSequence")) product=true;
    if(fu->sequence[i]->isA("Promoter")) promoter=true;
  }
  // Delete constitutively low FUs with inputs that can't be produced
  for(int i=0;i<fu->sequence.size();i++) {
    for_set(ExpressionRegulation*,fu->sequence[i]->regulators,er) {
      if((*er)->signal->producers.size() || // producible
         (*er)->signal->attributes.count(":motif-constant")) // or I/O
        input=true; // possible input
    }
    if(fu->sequence[i]->isA("Promoter")) {
      low_promoter = is_low_promoter(((Promoter*)fu->sequence[i]));
    }
  }
  // if there is no product or there is a product and 
  //     it can't be expressed and it isn't IO
  if(promoter && product && (!low_promoter || input)) return;
  // delete it
  if(verbosity>=2) *cpout<<"Eliminating unused sequence "<<fu->to_str()<<endl;
  note_change(fu);
  grn->delete_functional_unit(fu);
}
