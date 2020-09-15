/* SBOL XML emission hack
Copyright (C) 2009-2013, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

// Emit an abstract GRN in SBOL format
#include "biocompiler.h"
#include "cheapo-xml.h"
#include "sbol.h"

/*************** XML Extension ***************/
struct SBOL_XML : public XMLitem {
  SBOL_XML()
    : XMLitem("s:Collection","SHORT/col/GeneticRegulatoryNetwork")
  {}

  void emit_prefix(ostream* out) {
    *out << "<?xml version=\"1.0\"?>" << endl;
    *out << "<rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"" << endl;
    *out << "  xmlns:rdfs=\"http://www.w3.org/2000/01/rdf-schema#\"" << endl;
    *out << "  xmlns:so=\"http://purl.obolibrary.org/obo/\"" << endl; 
    *out << "  xmlns:s=\"http://sbols.org/v1#\"" << endl;
    *out << "  xmlns:grn=\"urn:bbn.com:tasbe:grn\">" << endl;
  }
};

/*************** XMLIZERS FOR GRN ELEMENTS ***************/
void sb_add_type_annotation(XMLitem *elt,ProtoType* t) {
  XMLitem *ext = new XMLitem("grn:design");
  XMLitem* type = new XMLitem("grn:DataType");
  if(t->isA("ProtoBoolean")) {
    type->attributes["grn:logicalType"]="boolean";
    ProtoBoolean *bt = dynamic_cast<ProtoBoolean*>(t);
    if(bt->constant) type->attributes["grn:logicalValue"]=b2s(bt->value);
  } else {
    compile_warn("Don't know how to XMLize type"+t->to_str()+": type information will be discarded");
  }
  ext->addElt(type); elt->addElt(ext);
}
 
set<grn::Chemical*> sb_chem_cache;
XMLitem* sb_chemical_to_xml(grn::Chemical* c) {
  XMLitem* x = new XMLitem("grn:ChemicalSpecies");
  x->attributes["grn:uid"]=c->name;
  if(!sb_chem_cache.count(c)) {
    // annotate w. logical type
    if(c->attributes.count("type"))
      sb_add_type_annotation(x,((grn::ProtoTypeAttribute*)c->attributes["type"])->type);
    // if it's a motif-constant and produced by something, annotate its family
    if(c->attributes.count(":motif-constant") && c->producers.size()) {
      XMLitem* family = new XMLitem("grn:Family"); 
      family->attributes["grn:name"]=c->name;
      x->linkElt("grn:property",family);
    }
    // note: may need to convert to scientific notation
    //x->attributes["gamma"]=f2s(c->halflife,4);
    //x->attributes["H"]=f2s(c->hill_coefficient,4);
    sb_chem_cache.insert(c);
  }
  return x;
}

string SO_prefix = "http://purl.obolibrary.org/obo/";
XMLitem* sb_dc_to_xml(grn::DNAComponent* dc) {
  static int next_id = 0;
  XMLitem* x = new XMLitem("s:DnaComponent","part_"+i2s(++next_id));
  x->addElt(new TrivialItem("s:displayId","part_"+i2s(next_id))); 
  XMLitem* type = new XMLitem("rdf:type");
  if(dc->isA("Promoter")) {
    x->addElt(new TrivialItem("s:name","Promoter "+i2s(next_id)));
    type->attributes["rdf:resource"] = SO_prefix+"SO_0000167"; // Promoter
    sb_add_type_annotation(x,((grn::Promoter*)dc)->rate); // annotate w. logical type
    //x->attributes["alpha"] = f2s(force_promoter_rate((Promoter*)dc));
  } else if(dc->isA("CodingSequence")) {
    grn::Chemical* c = ((grn::CodingSequence*)dc)->product;
    x->addElt(new TrivialItem("s:name",c->name+" CDS"));
    type->attributes["rdf:resource"] = SO_prefix+"SO_0000316"; // CDS
    XMLitem *ext = new XMLitem("grn:regulation"); x->addElt(ext);
    ext->linkElt("grn:product",sb_chemical_to_xml(c));
  } else if(dc->isA("Terminator")) {
    x->addElt(new TrivialItem("s:name","Terminator "+i2s(next_id)));
    type->attributes["rdf:resource"] = SO_prefix+"SO_0000141"; // terminator
    // no other information needed for terminator
  } else {
    ierror("Don't yet know how to XMLize "+dc->to_str());
  }
  x->addElt(type); // put last, after all other elements
  XMLitem* sa = new XMLitem("s:SequenceAnnotation","SHORT/anot/an_"+i2s(++next_id));
  sa->linkElt("s:subComponent",x);
  return sa;
}

XMLitem* sb_rr_to_xml(grn::RegulatoryReaction* rr) {
  XMLitem *rxn = new XMLitem("grn:RegulatoryReaction");
  rxn->linkElt("grn:substrate",sb_chemical_to_xml(rr->substrate));
  XMLitem *regulator = new XMLitem("grn:regulatedBy");
  regulator->attributes["grn:repression"] = b2s(rr->repressor);
  regulator->addElt(sb_chemical_to_xml(rr->regulator));
  rxn->addElt(regulator);
  return rxn;
}

void comment(Collection* design, string s) {
  char* prior = getCollectionDescription(design);
  string newstr = s;
  if(prior!=NULL) { newstr = std::string(prior)+"\n"+newstr; }
  setCollectionDescription(design,s.c_str());
}
/*************** ENTRY POINT ***************/
// TUs = transtriction units
void grn::GRNEmitter::to_sbol(ostream* out) {
  Document* document = createDocument();
  //Collection* design = createCollection(document, );
  //comment(design, "Genetic regulatory network for Proto: " + string(parent->last_script));
  //comment(design, std::string("Produced by BioCompiler version ")+BIOCOMPILER_VERSION);

  // Old version...
  sb_chem_cache.clear();
  SBOL_XML sbol;
  sbol.addComment("Genetic regulatory network for Proto: "
                 + string(parent->last_script));
  sbol.addComment(std::string("Produced by BioCompiler version ")+BIOCOMPILER_VERSION);
  sbol.addElt(new TrivialItem("s:displayId","GeneticRegulatoryNetwork"));
  // Serialize all transcriptional units
  sbol.addBreak(); sbol.addComment("Transcriptional Units");
  int idx = 0;
  for_set(grn::DNAComponent*,grn.dnacomponents,i) {
    if(i!=grn.dnacomponents.begin()) sbol.addBreak();
    FunctionalUnit* fu = (FunctionalUnit*)*i;
    XMLitem *seq = new XMLitem("s:DnaComponent","SHORT/part/FunctionalUnit_"+i2s(++idx));
    sbol.linkElt("s:component",seq);
    seq->addElt(new TrivialItem("s:displayId","FunctionalUnit_"+i2s(idx)));
    XMLitem *lastdc = NULL;
    for(int j=0;j<fu->sequence.size();j++) {
      XMLitem *dc = sb_dc_to_xml(fu->sequence[j]);
      if(lastdc) { // add precedence link
        XMLitem* prec = new XMLitem("s:precedes");
        prec->attributes["rdf:resource"] = dc->attributes["rdf:about"];
        lastdc->addEltToFront(prec);
      }
      seq->linkElt("s:annotation",dc);
      lastdc=dc;
      for_set(ExpressionRegulation*,fu->sequence[j]->regulators,er) {
        XMLitem *reg = new XMLitem("grn:regulatedBy");
        reg->attributes["grn:repression"]=b2s((*er)->repressor);
        reg->addElt(sb_chemical_to_xml((*er)->signal)); 
        XMLitem *ext = new XMLitem("grn:regulation"); ext->addElt(reg);
        // lastdc always exists:
        XMLitem *subC = *lastdc->elements.begin();
          (*subC->elements.begin())->addElt(ext);
      }
    }
  }
  // Serialize all reactions
  if(grn.reactions.size()) {
    sbol.addBreak(); sbol.addComment("Regulatory Reactions");
    XMLitem *ext = new XMLitem("grn:regulation"); sbol.addElt(ext);
    for_set(grn::RegulatoryReaction*,grn.reactions,ri)
      { ext->addElt(sb_rr_to_xml(*ri)); }
  }
  sbol.emit(out); *out << flush;
}
