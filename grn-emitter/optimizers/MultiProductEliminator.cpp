/* Optimizers
Copyright (C) 2009-2011, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

#include "grn_optimizers.h"

// Multiple-Product eliminator
// When turned on, remove split multiple-product sequences into
// multiple functional units
// Should only be run after the rest of the transformations are complete

MultiProductEliminator::MultiProductEliminator(GRNEmitter* parent, Args* args) {
  verbosity = args->extract_switch("--multi-product-eliminator-verbosity") ? 
    args->pop_number() : parent->verbosity;
}

void MultiProductEliminator::act(FunctionalUnit* fu) {
  int pcs_count = 0; int prefix_size = 0; int suffix_size = 0;
  // Check for structure: preface-PCS*-suffix
  for(int i=0;i<fu->sequence.size();i++) {
    if(fu->sequence[i]->isA("CodingSequence")) {
      if(suffix_size>0)
        ierror(fu,"Don't yet know how to split products for: "+ce2s(fu));
      pcs_count++;
    } else if(pcs_count==0) { prefix_size++;
    } else { suffix_size++;
    }
  }
    
  if(pcs_count<=1) return; // Only split 2+ product FunctionalUnits

  V2 << "Splitting products of " << ce2s(fu) << endl;
  for(int i=0;i<pcs_count;i++) {
    FunctionalUnit* newfu = new FunctionalUnit();
    for(int j=0;j<prefix_size;j++) // add prefix
      newfu->add(fu->sequence[j]->clone());
    newfu->add(fu->sequence[prefix_size+i]->clone()); // add product
    for(int j=0;j<suffix_size;j++) // add suffix
      newfu->add(fu->sequence[prefix_size+pcs_count+j]->clone());
    V3 << "Adding Functional Unit " << ce2s(newfu) << endl;
    grn->dnacomponents.insert(newfu);
    note_change(newfu);
  }
  V3 << "Deleting Functional Unit " << ce2s(fu) << endl;
  //note_change(fu);
  grn->delete_functional_unit(fu);
}

