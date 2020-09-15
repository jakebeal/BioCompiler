/* Optimizers
Copyright (C) 2009-2011, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

#ifndef __GRN_OPTIMIZERS__
#define __GRN_OPTIMIZERS__

#include "grn.h"
#include "grn_utilities.h"
#include "biocompiler.h"

using namespace grn;

class DoubleNegativeEliminator : public GRNPropagator {
public:
  DoubleNegativeEliminator(GRNEmitter* parent, Args* args);
  virtual void print(ostream* out=0) { *out << "DoubleNegativeEliminator"; }
  CodingSequence* isInverter(FunctionalUnit* fu, bool strict);
  void act(FunctionalUnit* fu);
};

class GRNConstantEliminator : public GRNPropagator {
public:
  GRNConstantEliminator(GRNEmitter* parent, Args* args);
  virtual void print(ostream* out=0) { *out << "GRNConstantEliminator"; }
  void act(FunctionalUnit* fu);
};

class GRNCopyPropagator : public GRNPropagator {
public:
  GRNCopyPropagator(GRNEmitter* parent, Args* args);
  virtual void print(ostream* out=0) { *out << "GRNCopyPropagator"; }
  void act(Chemical* c);
};

class GRNDeadCodeEliminator : public GRNPropagator {
public:
  GRNDeadCodeEliminator(GRNEmitter* parent, Args* args);
  virtual void print(ostream* out=0) { *out << "GRNDeadCodeEliminator"; }
  void act(Chemical* c);
  void act(FunctionalUnit* fu);
};

class GRNInferChemicalType : public GRNPropagator {
public:
  GRNInferChemicalType(GRNEmitter* parent, Args* args);
  virtual void print(ostream* out=0) { *out << "GRNInferChemicalType"; }
  void act(Chemical* c);
};

class MergeDuplicateInputs : public GRNPropagator {
public:
  MergeDuplicateInputs(GRNEmitter* parent, Args* args);
  virtual void print(ostream* out=0) { *out << "MergeDuplicateInputs"; }
  bool input_eqv(FunctionalUnit* a,FunctionalUnit* b);
  void act(FunctionalUnit* fu);
};

class MultiProductEliminator : public GRNPropagator {
public:
  MultiProductEliminator(GRNEmitter* parent, Args* args);
  virtual void print(ostream* out=0) { *out << "MultiProductEliminator"; }
  void act(FunctionalUnit* fu);
};

class NorCompressor : public GRNPropagator {
public:
  NorCompressor(GRNEmitter* parent, Args* args);
  virtual void print(ostream* out=0) { *out << "NorCompressor"; }
  Chemical* is_Or(FunctionalUnit* fu);
  void act(Chemical* c);
};

class DuplicateReactionConsolidator : public GRNPropagator {
public:
  DuplicateReactionConsolidator(GRNEmitter* parent, Args* args);
  virtual void print(ostream* out=0) { *out << "DuplicateReactionConsolidator";}
  void act(Chemical* c);
};

class OutputConsolidator : public GRNPropagator {
public:
  OutputConsolidator(GRNEmitter* parent, Args* args);
  virtual void print(ostream* out=0) { *out << "OutputConsolidator"; }
  void act(FunctionalUnit* fu);
};

#endif // __GRN_OPTIMIZERS__
