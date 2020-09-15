#include <cppunit/config/SourcePrefix.h>
#include "TypeCheckingTestCase.h"
//These macros expose the (explicitly) non-public functions 
#define protected public
#define private public
#include "analyzer.h"
#include "ir.h"
#undef protected
#undef private
#include "sexpr.h"

CPPUNIT_TEST_SUITE_REGISTRATION( TypeCheckingTestCase );

void TypeCheckingTestCase::setUp()
{
   //do nothing
}

void TypeCheckingTestCase::example() {
  CPPUNIT_ASSERT( 1 == 1 );
}

void TypeCheckingTestCase::deliteralizeScalar() {
  //deliteralize(<Scalar 2>) = <Scalar>
  ProtoScalar* literal = new ProtoScalar(2);
  ProtoType* delit = Deliteralization::deliteralize(literal);
  CPPUNIT_ASSERT(delit->isA("ProtoScalar"));
  ProtoScalar* delitScalar = S_TYPE(delit);
  CPPUNIT_ASSERT(!delitScalar->isLiteral());

  //deliteralize(deliteralize(<Scalar 2>)) = deliteralize(<Scalar>) = <Scalar>
  ProtoType* redelit = Deliteralization::deliteralize(delit);
  CPPUNIT_ASSERT(redelit->isA("ProtoScalar"));
  ProtoScalar* redelitScalar = S_TYPE(redelit);
  CPPUNIT_ASSERT(!redelitScalar->isLiteral());
}

void TypeCheckingTestCase::deliteralizeTuple() {
  //deliteralize(<Tuple <Any>...>) = <Tuple <Any>...>
  ProtoTuple* tup = new ProtoTuple();
  int size = tup->types.size();
  ProtoType* delit = Deliteralization::deliteralize(tup);
  CPPUNIT_ASSERT(delit->isA("ProtoTuple"));
  CPPUNIT_ASSERT(!delit->isLiteral());
  ProtoTuple* unlit = T_TYPE(delit);
  CPPUNIT_ASSERT(size == unlit->types.size() == 1);
  CPPUNIT_ASSERT(unlit->types[0]->isA("ProtoType"));
  CPPUNIT_ASSERT(!unlit->bounded);
  
  //deliteralize(<1-Tuple <Any>>) = <1-Tuple <Any>>
  tup = new ProtoTuple(true);
  tup->add(new ProtoType());
  size = tup->types.size();
  delit = Deliteralization::deliteralize(tup);
  CPPUNIT_ASSERT(delit->isA("ProtoTuple"));
  CPPUNIT_ASSERT(!delit->isLiteral());
  unlit = T_TYPE(delit);
  CPPUNIT_ASSERT(size == unlit->types.size() == 1);
  CPPUNIT_ASSERT(unlit->types[0]->isA("ProtoType"));
  CPPUNIT_ASSERT(unlit->bounded);
  
  //deliteralize(<1-Tuple <Scalar 2>>) = <1-Tuple <Scalar>>
  tup = new ProtoTuple(true);
  tup->add(new ProtoScalar(2));
  size = tup->types.size();
  delit = Deliteralization::deliteralize(tup);
  CPPUNIT_ASSERT(delit->isA("ProtoTuple"));
  CPPUNIT_ASSERT(!delit->isLiteral());
  unlit = T_TYPE(delit);
  CPPUNIT_ASSERT(size == unlit->types.size() == 1);
  CPPUNIT_ASSERT(unlit->types[0]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(!dynamic_cast<ProtoScalar*>(unlit->types[0])->isLiteral());
  CPPUNIT_ASSERT(unlit->bounded);
}

OperatorInstance* makeNewOI(vector<ProtoType*> req_input,
                            vector<ProtoType*> opt_input,
                            ProtoTuple* rest_input,
                            ProtoType* output) {
  Signature* sig = new Signature( new CE(),     //CE
                                  output ); //ProtoType
  for(int i=0; i<req_input.size(); i++) {
     sig->required_inputs.push_back(req_input[i]);
  }
  for(int i=0; i<opt_input.size(); i++) {
     sig->optional_inputs.push_back(opt_input[i]);
  }
  sig->rest_input = rest_input;
  Operator* op = new Operator( new CE(),  //CE
                               sig ); //Signature
  AmorphousMedium* space = new AmorphousMedium( new CE(),   //CE
                                                new DFG() ); //DFG
  OperatorInstance* oi = new OperatorInstance( new CE(),    //CE
                                               op,      //Operator
                                               space ); //AmorphousMedium
  for(int i=0; i<req_input.size(); i++) 
    oi->add_input( new Field(new CE(),space,req_input[i],oi) );
  for(int i=0; i<opt_input.size(); i++) 
    oi->add_input( new Field(new CE(),space,opt_input[i],oi) );
  if(rest_input) {
    for(int i=0; i<rest_input->types.size(); i++) {
      oi->add_input( new Field(new CE(),space,rest_input->types[i],oi) );
    }
  }
  return oi;
}

void TypeCheckingTestCase::getNthArg() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoTuple() );
  req_input.push_back( new ProtoScalar() );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = NULL;
  //output
  ProtoType* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  CPPUNIT_ASSERT(tca->get_nth_arg(oi, 0)->isA("ProtoTuple"));
  CPPUNIT_ASSERT(tca->get_nth_arg(oi, 1)->isA("ProtoScalar"));
  //CPPUNIT_ASSERT(tca->get_nth_arg(oi, 2) == NULL);
}

void TypeCheckingTestCase::getRefSymbol() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoScalar() );
  req_input.push_back( new ProtoScalar(2) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = NULL;
  //output
  ProtoType* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  CPPUNIT_ASSERT(tca->get_ref(oi,new SE_Symbol("value"))->isA("ProtoTuple"));
  CPPUNIT_ASSERT(tca->get_ref(oi,new SE_Symbol("arg0"))->isA("ProtoScalar"));
  ProtoType* ref = tca->get_ref(oi,new SE_Symbol("arg1"));
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(ref->isLiteral());
  CPPUNIT_ASSERT_EQUAL(2, (int)S_VAL(ref));
}

void TypeCheckingTestCase::getRefUnlit() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoScalar() );
  req_input.push_back( new ProtoScalar(2) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = NULL;
  //output
  ProtoType* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("unlit"));
  sexpr->add(new SE_Symbol("arg1"));
  ProtoType* ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(!ref->isLiteral());
}

void TypeCheckingTestCase::getRefFieldof() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoScalar() );
  req_input.push_back( new ProtoScalar(2) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = NULL;
  //output
  ProtoType* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("fieldof"));
  sexpr->add(new SE_Symbol("arg1"));
  ProtoType* ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoField"));
  CPPUNIT_ASSERT(F_TYPE(ref)->hoodtype->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(F_TYPE(ref)->hoodtype) == 2);
}

void TypeCheckingTestCase::getRefFt() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoField(new ProtoScalar(2)) );
  req_input.push_back( new ProtoScalar(3) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = NULL;
  //output
  ProtoType* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("ft"));
  sexpr->add(new SE_Symbol("arg0"));
  ProtoType* ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 2);
}

void TypeCheckingTestCase::getRefInputs() {
  //First, make an oi as the op to a ProtoLambda
  ProtoLambda* lambda = NULL;
   {
      //required_inputs
      vector<ProtoType*> req_input = vector<ProtoType*>();
      req_input.push_back( new ProtoScalar() );
      req_input.push_back( new ProtoScalar(2) );
      //optional_inputs
      vector<ProtoType*> opt_input = vector<ProtoType*>();
      //rest
      ProtoTuple* rest = NULL;
      //output
      ProtoType* output = new ProtoTuple();
      OperatorInstance* oi = makeNewOI( req_input, 
                                        opt_input, 
                                        rest, 
                                        output );
      lambda = new ProtoLambda(oi->op);
   }
  //Now, add the ProtoLambda to a new OI
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( lambda );
  req_input.push_back( new ProtoScalar(3) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = NULL;
  //output
  ProtoType* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("inputs"));
  sexpr->add(new SE_Symbol("arg0"));
  ProtoType* ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoTuple"));
  ProtoTuple* tup = T_TYPE(ref);
  CPPUNIT_ASSERT(tup->bounded);
  CPPUNIT_ASSERT(tup->types[0]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(tup->types[1]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(tup->types[1]) == 2);
}

void TypeCheckingTestCase::getRefOutput() {
  //First, make an oi as the op to a ProtoLambda
  ProtoLambda* lambda = NULL;
   {
      //required_inputs
      vector<ProtoType*> req_input = vector<ProtoType*>();
      req_input.push_back( new ProtoScalar() );
      req_input.push_back( new ProtoScalar(2) );
      //optional_inputs
      vector<ProtoType*> opt_input = vector<ProtoType*>();
      //rest
      ProtoTuple* rest = NULL;
      //output
      ProtoType* output = new ProtoScalar(4);
      OperatorInstance* oi = makeNewOI( req_input, 
                                        opt_input, 
                                        rest, 
                                        output );
      lambda = new ProtoLambda(oi->op);
   }
  //Now, add the ProtoLambda to a new OI
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( lambda );
  req_input.push_back( new ProtoScalar(3) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = NULL;
  //output
  ProtoType* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("output"));
  sexpr->add(new SE_Symbol("arg0"));
  ProtoType* ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 4);
}

void TypeCheckingTestCase::getRefLast() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoField(new ProtoScalar(2)) );
  req_input.push_back( new ProtoScalar(3) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar(7));
  rest->add(new ProtoScalar(8));
  //output
  ProtoTuple* output = new ProtoTuple(true);
  output->add( new ProtoScalar(4) );
  output->add( new ProtoScalar(5) );
  output->add( new ProtoScalar(6) );
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("last"));
  sexpr->add(new SE_Symbol("value"));
  ProtoType* ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 6);
  sexpr = new SE_List();
  sexpr->add(new SE_Symbol("last"));
  sexpr->add(new SE_Symbol("arg2"));
  ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 8);
}

void TypeCheckingTestCase::getRefNth() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoField(new ProtoScalar(2)) );
  req_input.push_back( new ProtoScalar(1) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar(7));
  rest->add(new ProtoScalar(8));
  rest->add(new ProtoScalar(9));
  //output
  ProtoTuple* output = new ProtoTuple(true);
  output->add( new ProtoScalar(4) );
  output->add( new ProtoScalar(5) );
  output->add( new ProtoScalar(6) );
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("nth"));
  sexpr->add(new SE_Symbol("value"));
  sexpr->add(new SE_Symbol("arg1"));
  ProtoType* ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 5);
  sexpr = new SE_List();
  sexpr->add(new SE_Symbol("nth"));
  sexpr->add(new SE_Symbol("arg2"));
  sexpr->add(new SE_Symbol("arg1"));
  ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 8);
}

void TypeCheckingTestCase::getRefNthAllSame() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoField(new ProtoScalar(2)) );
  req_input.push_back( new ProtoScalar() );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar(7));
  rest->add(new ProtoScalar(7));
  rest->add(new ProtoScalar(8));
  //output
  ProtoTuple* output = new ProtoTuple(true);
  output->add( new ProtoScalar(4) );
  output->add( new ProtoScalar(4) );
  output->add( new ProtoScalar(4) );
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("nth"));
  sexpr->add(new SE_Symbol("value"));
  sexpr->add(new SE_Symbol("arg1"));
  ProtoType* ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(ref->isLiteral());
  CPPUNIT_ASSERT(S_VAL(ref) == 4);
  sexpr = new SE_List();
  sexpr->add(new SE_Symbol("nth"));
  sexpr->add(new SE_Symbol("arg2"));
  sexpr->add(new SE_Symbol("arg1"));
  ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(!ref->isLiteral());
}

void TypeCheckingTestCase::getRefLcs() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoScalar(3) );
  req_input.push_back( new ProtoScalar(3) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar(9));
  rest->add(new ProtoScalar(9));
  rest->add(new ProtoScalar(9));
  //output
  ProtoTuple* output = new ProtoTuple(true);
  output->add( new ProtoScalar(4) );
  output->add( new ProtoScalar(5) );
  output->add( new ProtoScalar(6) );
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("lcs"));
  sexpr->add(new SE_Symbol("arg0"));
  sexpr->add(new SE_Symbol("arg1"));
  ProtoType* ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(ref->isLiteral());
  CPPUNIT_ASSERT_EQUAL(3, (int)S_VAL(ref));
  sexpr = new SE_List();
  sexpr->add(new SE_Symbol("lcs"));
  sexpr->add(new SE_Symbol("arg0"));
  sexpr->add(new SE_Symbol("arg1"));
  sexpr->add(new SE_Symbol("arg2"));
  ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(!ref->isLiteral());
  sexpr = new SE_List();
  sexpr->add(new SE_Symbol("lcs"));
  sexpr->add(new SE_Symbol("arg2"));
  ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(ref->isLiteral());
  CPPUNIT_ASSERT_EQUAL(9, (int)S_VAL(ref));
}

void TypeCheckingTestCase::getRefTupof() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoScalar(2) );
  req_input.push_back( new ProtoScalar(3) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar(7));
  rest->add(new ProtoScalar(8));
  rest->add(new ProtoScalar(9));
  //output
  ProtoTuple* output = new ProtoTuple(true);
  output->add( new ProtoScalar(4) );
  output->add( new ProtoScalar(5) );
  output->add( new ProtoScalar(6) );
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("tupof"));
  sexpr->add(new SE_Symbol("arg0"));
  sexpr->add(new SE_Symbol("arg1"));
  ProtoType* ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoTuple"));
  ProtoTuple* tup = T_TYPE(ref);
  CPPUNIT_ASSERT(tup->types[0]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(tup->types[1]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(tup->types[0]) == 2);
  CPPUNIT_ASSERT(S_VAL(tup->types[1]) == 3);

  // rest alone
  sexpr = new SE_List();
  sexpr->add(new SE_Symbol("tupof"));
  sexpr->add(new SE_Symbol("arg2"));
  ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoTuple"));
  tup = T_TYPE(ref);
  CPPUNIT_ASSERT(tup->types[0]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(tup->types[1]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(tup->types[2]->isA("ProtoScalar"));
  CPPUNIT_ASSERT_EQUAL(7, (int)S_VAL(tup->types[0]));
  CPPUNIT_ASSERT_EQUAL(8, (int)S_VAL(tup->types[1]));
  CPPUNIT_ASSERT_EQUAL(9, (int)S_VAL(tup->types[2]));
  
  // rest with others
  sexpr = new SE_List();
  sexpr->add(new SE_Symbol("tupof"));
  sexpr->add(new SE_Symbol("arg0"));
  sexpr->add(new SE_Symbol("arg1"));
  sexpr->add(new SE_Symbol("arg2"));
  ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoTuple"));
  tup = T_TYPE(ref);
  CPPUNIT_ASSERT(tup->types[0]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(tup->types[1]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(tup->types[0]) == 2);
  CPPUNIT_ASSERT(S_VAL(tup->types[1]) == 3);
  CPPUNIT_ASSERT(tup->types[2]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(tup->types[3]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(tup->types[4]->isA("ProtoScalar"));
  CPPUNIT_ASSERT_EQUAL(7, (int)S_VAL(tup->types[2]));
  CPPUNIT_ASSERT_EQUAL(8, (int)S_VAL(tup->types[3]));
  CPPUNIT_ASSERT_EQUAL(9, (int)S_VAL(tup->types[4]));
}

void TypeCheckingTestCase::getRefOptionalInputs() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoScalar(2) );
  req_input.push_back( new ProtoScalar(3) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  opt_input.push_back( new ProtoScalar(4) );
  opt_input.push_back( new ProtoScalar(5) );
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar(7));
  rest->add(new ProtoScalar(8));
  rest->add(new ProtoScalar(9));
  //output
  ProtoTuple* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  ProtoType* ref = tca->get_ref(oi,new SE_Symbol("arg0"));
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 2);
  ref = tca->get_ref(oi,new SE_Symbol("arg1"));
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 3);
  ref = tca->get_ref(oi,new SE_Symbol("arg2"));
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 4);
  ref = tca->get_ref(oi,new SE_Symbol("arg3"));
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 5);
  ref = tca->get_ref(oi,new SE_Symbol("arg4"));
  CPPUNIT_ASSERT(ref->isA("ProtoTuple"));
  CPPUNIT_ASSERT_EQUAL(7, (int)S_VAL(T_TYPE(ref)->types[0]));
  CPPUNIT_ASSERT_EQUAL(8, (int)S_VAL(T_TYPE(ref)->types[1]));
  CPPUNIT_ASSERT_EQUAL(9, (int)S_VAL(T_TYPE(ref)->types[2]));
}

void TypeCheckingTestCase::getRefRest() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoScalar(2) );
  req_input.push_back( new ProtoScalar(3) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  opt_input.push_back( new ProtoScalar(4) );
  opt_input.push_back( new ProtoScalar(5) );
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar(6));
  rest->add(new ProtoScalar(7));
  //output
  ProtoTuple* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  ProtoType* ref = tca->get_ref(oi,new SE_Symbol("arg0"));
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 2);
  ref = tca->get_ref(oi,new SE_Symbol("arg1"));
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 3);
  ref = tca->get_ref(oi,new SE_Symbol("arg2"));
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 4);
  ref = tca->get_ref(oi,new SE_Symbol("arg3"));
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 5);
  ref = tca->get_ref(oi,new SE_Symbol("arg4"));
  CPPUNIT_ASSERT(ref->isA("ProtoTuple"));
  ProtoTuple* rest_elem = T_TYPE(ref);
  CPPUNIT_ASSERT(rest_elem->bounded);
  CPPUNIT_ASSERT_EQUAL(2, (int)rest_elem->types.size());
  CPPUNIT_ASSERT(rest_elem->types[0]->isA("ProtoScalar"));
  CPPUNIT_ASSERT_EQUAL(6, (int)S_VAL(rest_elem->types[0]));
  CPPUNIT_ASSERT_EQUAL(7, (int)S_VAL(rest_elem->types[1]));
}

void TypeCheckingTestCase::assertNthEmptyRest() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  //output
  ProtoTuple* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  tca->assert_nth_arg(oi,0,new ProtoTuple(true));
  CPPUNIT_ASSERT(oi->nth_input(0)->isA("ProtoTuple"));
  CPPUNIT_ASSERT_EQUAL(0, (int)T_TYPE(oi->nth_input(0))->types.size());
}

void TypeCheckingTestCase::assertNthArg() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoScalar() );
  req_input.push_back( new ProtoScalar() );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  opt_input.push_back( new ProtoScalar() );
  opt_input.push_back( new ProtoScalar() );
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar());
  rest->add(new ProtoScalar());
  //output
  ProtoTuple* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  tca->assert_nth_arg(oi,0,new ProtoScalar(2));
  CPPUNIT_ASSERT(oi->nth_input(0)->isA("ProtoScalar"));
  CPPUNIT_ASSERT(oi->nth_input(0)->isLiteral());
  CPPUNIT_ASSERT_EQUAL(2, (int)S_VAL(oi->nth_input(0)));
  tca->assert_nth_arg(oi,1,new ProtoScalar(3));
  CPPUNIT_ASSERT(oi->nth_input(1)->isA("ProtoScalar"));
  CPPUNIT_ASSERT(oi->nth_input(1)->isLiteral());
  CPPUNIT_ASSERT_EQUAL(3, (int)S_VAL(oi->nth_input(1)));
  tca->assert_nth_arg(oi,2,new ProtoScalar(4));
  CPPUNIT_ASSERT(oi->nth_input(2)->isA("ProtoScalar"));
  CPPUNIT_ASSERT(oi->nth_input(2)->isLiteral());
  CPPUNIT_ASSERT_EQUAL(4, (int)S_VAL(oi->nth_input(2)));
  tca->assert_nth_arg(oi,3,new ProtoScalar(5));
  CPPUNIT_ASSERT(oi->nth_input(3)->isA("ProtoScalar"));
  CPPUNIT_ASSERT(oi->nth_input(3)->isLiteral());
  CPPUNIT_ASSERT_EQUAL(5, (int)S_VAL(oi->nth_input(3)));
  ProtoTuple* restElem = new ProtoTuple(true);
  restElem->add(new ProtoScalar(6));
  restElem->add(new ProtoScalar(7));
  tca->assert_nth_arg(oi,4,restElem);
  CPPUNIT_ASSERT(oi->nth_input(4)->isA("ProtoScalar"));
  CPPUNIT_ASSERT(oi->nth_input(4)->isLiteral());
  CPPUNIT_ASSERT_EQUAL(6, (int)S_VAL(oi->nth_input(4)));
  CPPUNIT_ASSERT(oi->nth_input(5)->isA("ProtoScalar"));
  CPPUNIT_ASSERT(oi->nth_input(5)->isLiteral());
  CPPUNIT_ASSERT_EQUAL(7, (int)S_VAL(oi->nth_input(5)));
}


void TypeCheckingTestCase::assertRefSymbol() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoScalar() );
  req_input.push_back( new ProtoScalar() );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  opt_input.push_back( new ProtoScalar() );
  opt_input.push_back( new ProtoScalar() );
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar());
  rest->add(new ProtoScalar());
  //output
  ProtoTuple* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  tca->assert_ref(oi, new SE_Symbol("arg0"), new ProtoScalar(2));
  CPPUNIT_ASSERT(oi->nth_input(0)->isA("ProtoScalar"));
  CPPUNIT_ASSERT(oi->nth_input(0)->isLiteral());
  CPPUNIT_ASSERT_EQUAL(2, (int)S_VAL(oi->nth_input(0)));
  tca->assert_ref(oi, new SE_Symbol("value"), new ProtoVector());
  CPPUNIT_ASSERT(oi->output->range->isA("ProtoVector"));
}

void TypeCheckingTestCase::assertRefUnlit() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoType() );
  req_input.push_back( new ProtoType() );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  opt_input.push_back( new ProtoScalar() );
  opt_input.push_back( new ProtoScalar() );
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar());
  rest->add(new ProtoScalar());
  //output
  ProtoTuple* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("unlit"));
  sexpr->add(new SE_Symbol("arg0"));
  tca->assert_ref(oi, sexpr, new ProtoScalar(5));
  CPPUNIT_ASSERT(oi->nth_input(0)->isA("ProtoScalar"));
  CPPUNIT_ASSERT(!oi->nth_input(0)->isLiteral());
}

void TypeCheckingTestCase::assertRefFieldof() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoType() );
  req_input.push_back( new ProtoType() );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  opt_input.push_back( new ProtoScalar() );
  opt_input.push_back( new ProtoScalar() );
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar());
  rest->add(new ProtoScalar());
  //output
  ProtoTuple* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("fieldof"));
  sexpr->add(new SE_Symbol("arg0"));
  tca->assert_ref(oi, sexpr, new ProtoField(new ProtoScalar(5)));
  CPPUNIT_ASSERT(oi->nth_input(0)->isA("ProtoScalar"));
  CPPUNIT_ASSERT(oi->nth_input(0)->isLiteral());
  CPPUNIT_ASSERT_EQUAL(5, (int)S_VAL(oi->nth_input(0)));
}

void TypeCheckingTestCase::assertRefFt() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoType() );
  req_input.push_back( new ProtoType() );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  opt_input.push_back( new ProtoScalar() );
  opt_input.push_back( new ProtoScalar() );
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar());
  rest->add(new ProtoScalar());
  //output
  ProtoTuple* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("ft"));
  sexpr->add(new SE_Symbol("arg0"));
  tca->assert_ref(oi, sexpr, new ProtoField(new ProtoScalar(5)));
  CPPUNIT_ASSERT(oi->nth_input(0)->isA("ProtoField"));
  CPPUNIT_ASSERT(F_VAL(oi->nth_input(0))->isA("ProtoScalar"));
  CPPUNIT_ASSERT(F_VAL(oi->nth_input(0))->isLiteral());
  CPPUNIT_ASSERT_EQUAL(5, (int)S_VAL(F_VAL(oi->nth_input(0))));
}

void TypeCheckingTestCase::assertRefInputs() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  Signature* sig = new Signature( new CE(), new ProtoType() );
  sig->required_inputs.push_back(new ProtoType());
  sig->rest_input = new ProtoTuple(true);
  T_TYPE(sig->rest_input)->types.push_back(new ProtoType());
  T_TYPE(sig->rest_input)->types.push_back(new ProtoType());
  ProtoLambda* lambda = new ProtoLambda( new Operator( new CE(), sig ) );
  req_input.push_back( lambda );
  req_input.push_back( new ProtoType() );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  opt_input.push_back( new ProtoScalar() );
  opt_input.push_back( new ProtoScalar() );
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar());
  rest->add(new ProtoScalar());
  //output
  ProtoTuple* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("inputs"));
  sexpr->add(new SE_Symbol("arg0"));
  //valueTup = (tup 1 (tup 2 3))
  ProtoTuple* valueTup = new ProtoTuple(true);
  valueTup->add(new ProtoScalar(1));
  ProtoTuple* restTup = new ProtoTuple(true);
  restTup->add(new ProtoScalar(2));
  restTup->add(new ProtoScalar(3));
  valueTup->add(restTup);
  tca->assert_ref(oi, sexpr, valueTup);

  // arg0 = <Lambda [sig:osig]
  CPPUNIT_ASSERT(oi->nth_input(0)->isA("ProtoLambda"));
  Signature* osig = L_VAL(oi->nth_input(0))->signature;
  // osig = inputs [0]:<Scalar>, [1]:<Tuple <Scalar 2> <Scalar 3>>
  CPPUNIT_ASSERT(osig->nth_type(0)->isA("ProtoScalar"));
  CPPUNIT_ASSERT_EQUAL(1, (int)S_VAL(L_VAL(oi->nth_input(0))->signature->nth_type(0)));
  /*TODO: not yet implemented
  CPPUNIT_ASSERT(osig->nth_type(1)->isA("ProtoTuple"));
  cout << endl << ce2s(osig->nth_type(1)) << endl;
  CPPUNIT_ASSERT(T_TYPE(osig->nth_type(1))->types[0]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(T_TYPE(osig->nth_type(1))->types[1]->isA("ProtoScalar"));
  CPPUNIT_ASSERT_EQUAL(2, (int)S_VAL(T_TYPE(osig->nth_type(1))->types[0]));
  CPPUNIT_ASSERT_EQUAL(3, (int)S_VAL(T_TYPE(osig->nth_type(1))->types[1]));
  */
}

void TypeCheckingTestCase::assertRefInputsRest() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  Signature* sig = new Signature( new CE(), new ProtoType() );
  sig->rest_input = new ProtoTuple(); //unbound
  T_TYPE(sig->rest_input)->clear();
  T_TYPE(sig->rest_input)->types.push_back(new ProtoNumber());
  ProtoLambda* lambda = new ProtoLambda( new Operator( new CE(), sig ) );
  req_input.push_back( lambda );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = NULL;
  //output
  ProtoNumber* output = new ProtoNumber();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("inputs"));
  sexpr->add(new SE_Symbol("arg0"));
  //valueTup = (tup 1 2 3)
  ProtoTuple* valueTup = new ProtoTuple(true);
  valueTup->add(new ProtoScalar(1));
  valueTup->add(new ProtoScalar(2));
  valueTup->add(new ProtoScalar(3));
  tca->assert_ref(oi, sexpr, valueTup);
  CPPUNIT_ASSERT(oi->nth_input(0)->isA("ProtoLambda"));
  Signature* osig = L_VAL(oi->nth_input(0))->signature;
}

void TypeCheckingTestCase::assertRefOutput() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  Signature* sig = new Signature( new CE(), new ProtoScalar() );
  sig->required_inputs.push_back(new ProtoTuple());
  sig->required_inputs.push_back(new ProtoTuple());
  ProtoLambda* lambda = new ProtoLambda( new Operator( new CE(), sig ) );
  req_input.push_back( lambda );
  req_input.push_back( new ProtoType() );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  opt_input.push_back( new ProtoScalar() );
  opt_input.push_back( new ProtoScalar() );
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar());
  rest->add(new ProtoScalar());
  //output
  ProtoTuple* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("output"));
  sexpr->add(new SE_Symbol("arg0"));
  tca->assert_ref(oi, sexpr, new ProtoNumber());
  CPPUNIT_ASSERT(oi->nth_input(0)->isA("ProtoLambda"));
  CPPUNIT_ASSERT(L_VAL(oi->nth_input(0))->signature->output->isA("ProtoScalar"));
}

void TypeCheckingTestCase::assertRefTupof() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoType() );
  req_input.push_back( new ProtoType() );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  opt_input.push_back( new ProtoScalar() );
  opt_input.push_back( new ProtoScalar() );
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar());
  rest->add(new ProtoScalar());
  //output
  ProtoTuple* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("tupof"));
  sexpr->add(new SE_Symbol("arg0"));
  tca->assert_ref(oi, sexpr, new ProtoTuple());
  CPPUNIT_ASSERT(oi->nth_input(0)->isA("ProtoTuple"));
}

void TypeCheckingTestCase::assertRefLast() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoType() );
  req_input.push_back( new ProtoType() );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  opt_input.push_back( new ProtoScalar() );
  opt_input.push_back( new ProtoScalar() );
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar());
  rest->add(new ProtoScalar());
  //output
  ProtoTuple* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("last"));
  sexpr->add(new SE_Symbol("arg0"));
  ProtoTuple* tup = new ProtoTuple(true);
  tup->add(new ProtoScalar(2));
  tup->add(new ProtoScalar(3));
  tup->add(new ProtoScalar(4));
  tca->assert_ref(oi, sexpr, tup);

  sexpr = new SE_List();
  sexpr->add(new SE_Symbol("last"));
  sexpr->add(new SE_Symbol("arg1"));
  tca->assert_ref(oi, sexpr, new ProtoScalar(5));
  
  CPPUNIT_ASSERT(oi->nth_input(0)->isA("ProtoTuple"));
  CPPUNIT_ASSERT(oi->nth_input(1)->isA("ProtoTuple"));
  ProtoTuple* rettup = T_TYPE(oi->nth_input(1));
  ProtoType* retlast = rettup->types[rettup->types.size()-1];
  CPPUNIT_ASSERT(retlast->isA("ProtoScalar"));
  CPPUNIT_ASSERT(retlast->isLiteral());
  CPPUNIT_ASSERT_EQUAL(5, (int)S_VAL(retlast));
}

void TypeCheckingTestCase::assertRefLastBounded() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  ProtoTuple* arg0 = new ProtoTuple(true);
  arg0->add(new ProtoType());
  arg0->add(new ProtoType());
  arg0->add(new ProtoType());
  arg0->add(new ProtoTuple());
  req_input.push_back( arg0 );
  ProtoTuple* arg1 = new ProtoTuple(true);
  arg1->add(new ProtoType());
  arg1->add(new ProtoType());
  arg1->add(new ProtoType());
  arg1->add(new ProtoScalar());
  req_input.push_back( arg1 );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  opt_input.push_back( new ProtoScalar() );
  opt_input.push_back( new ProtoScalar() );
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar());
  rest->add(new ProtoScalar());
  //output
  ProtoTuple* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("last"));
  sexpr->add(new SE_Symbol("arg0"));
  ProtoTuple* tup = new ProtoTuple(true);
  tup->add(new ProtoScalar(2));
  tup->add(new ProtoScalar(3));
  tup->add(new ProtoScalar(4));
  tca->assert_ref(oi, sexpr, tup);

  sexpr = new SE_List();
  sexpr->add(new SE_Symbol("last"));
  sexpr->add(new SE_Symbol("arg1"));
  tca->assert_ref(oi, sexpr, new ProtoScalar(5));
  
  CPPUNIT_ASSERT(oi->nth_input(0)->isA("ProtoTuple"));
  CPPUNIT_ASSERT(oi->nth_input(1)->isA("ProtoTuple"));
  ProtoTuple* rettup = T_TYPE(oi->nth_input(1));
  ProtoType* retlast = rettup->types[rettup->types.size()-1];
  CPPUNIT_ASSERT(retlast->isA("ProtoScalar"));
  CPPUNIT_ASSERT(retlast->isLiteral());
  CPPUNIT_ASSERT_EQUAL(5, (int)S_VAL(retlast));
}

void TypeCheckingTestCase::assertRefLcs() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoType() );
  req_input.push_back( new ProtoType() );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  opt_input.push_back( new ProtoScalar() );
  opt_input.push_back( new ProtoScalar() );
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar());
  rest->add(new ProtoScalar());
  //output
  ProtoTuple* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("lcs"));
  sexpr->add(new SE_Symbol("arg0"));
  sexpr->add(new SE_Symbol("arg1"));
  sexpr->add(new SE_Symbol("arg2"));
  tca->assert_ref(oi, sexpr, new ProtoNumber());
  CPPUNIT_ASSERT(oi->nth_input(0)->isA("ProtoNumber"));
  CPPUNIT_ASSERT(oi->nth_input(1)->isA("ProtoNumber"));
  CPPUNIT_ASSERT(oi->nth_input(2)->isA("ProtoNumber"));
}

void TypeCheckingTestCase::assertRefNth() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoType() );
  req_input.push_back( new ProtoScalar(0) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  opt_input.push_back( new ProtoScalar() );
  opt_input.push_back( new ProtoScalar() );
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar());
  rest->add(new ProtoScalar());
  //output
  ProtoType* output = new ProtoScalar(4);
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("nth"));
  sexpr->add(new SE_Symbol("arg0"));
  sexpr->add(new SE_Symbol("arg1"));
  tca->assert_ref(oi, sexpr, new ProtoScalar(3));
  CPPUNIT_ASSERT(oi->nth_input(0)->isA("ProtoTuple"));
  CPPUNIT_ASSERT(oi->nth_input(1)->isA("ProtoScalar"));
  // arg0 = <Tuple <Scalar 3> <Any>...>
  CPPUNIT_ASSERT(T_TYPE(oi->nth_input(0))->types[0]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(T_TYPE(oi->nth_input(0))->types[0]->isLiteral());
  CPPUNIT_ASSERT_EQUAL(3, (int)S_VAL(T_TYPE(oi->nth_input(0))->types[0]));
  CPPUNIT_ASSERT(T_TYPE(oi->nth_input(0))->types[1]->isA("ProtoType"));
  CPPUNIT_ASSERT(!T_TYPE(oi->nth_input(0))->types[1]->isLiteral());
  CPPUNIT_ASSERT(!T_TYPE(oi->nth_input(0))->bounded);
  CPPUNIT_ASSERT_EQUAL(2, (int)T_TYPE(oi->nth_input(0))->types.size());
}

void TypeCheckingTestCase::assertRefNthFillTup() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoType() );
  req_input.push_back( new ProtoScalar(2) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  opt_input.push_back( new ProtoScalar() );
  opt_input.push_back( new ProtoScalar() );
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar());
  rest->add(new ProtoScalar());
  //output
  ProtoType* output = new ProtoScalar(4);
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("nth"));
  sexpr->add(new SE_Symbol("arg0"));
  sexpr->add(new SE_Symbol("arg1"));
  tca->assert_ref(oi, sexpr, new ProtoScalar(5));
  // arg0 = <Tuple <Any> <Any> <Scalar 5> <Any>...>
  CPPUNIT_ASSERT(oi->nth_input(0)->isA("ProtoTuple"));
  CPPUNIT_ASSERT_EQUAL(4, (int)(T_TYPE(oi->nth_input(0))->types.size()));
  CPPUNIT_ASSERT(T_TYPE(oi->nth_input(0))->types[0]->isA("ProtoType"));
  CPPUNIT_ASSERT(!T_TYPE(oi->nth_input(0))->types[0]->isLiteral());
  CPPUNIT_ASSERT(T_TYPE(oi->nth_input(0))->types[1]->isA("ProtoType"));
  CPPUNIT_ASSERT(!T_TYPE(oi->nth_input(0))->types[1]->isLiteral());
  CPPUNIT_ASSERT(T_TYPE(oi->nth_input(0))->types[2]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(T_TYPE(oi->nth_input(0))->types[2]->isLiteral());
  CPPUNIT_ASSERT_EQUAL(5, (int)S_VAL(T_TYPE(oi->nth_input(0))->types[2]));
  CPPUNIT_ASSERT(T_TYPE(oi->nth_input(0))->types[3]->isA("ProtoType"));
  CPPUNIT_ASSERT(!T_TYPE(oi->nth_input(0))->types[3]->isLiteral());
  // arg1 = <Scalar 3>
  CPPUNIT_ASSERT(oi->nth_input(1)->isA("ProtoScalar"));
}

void TypeCheckingTestCase::assertRefNthReplaceFilledTup() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  ProtoTuple* inTup = new ProtoTuple(true);
  inTup->add(new ProtoScalar(8));
  inTup->add(new ProtoScalar());
  inTup->add(new ProtoScalar(10));
  inTup->add(new ProtoScalar(11));
  req_input.push_back( inTup );
  req_input.push_back( new ProtoScalar(1) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  opt_input.push_back( new ProtoScalar() );
  opt_input.push_back( new ProtoScalar() );
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar());
  rest->add(new ProtoScalar());
  //output
  ProtoType* output = new ProtoScalar(4);
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("nth"));
  sexpr->add(new SE_Symbol("arg0"));
  sexpr->add(new SE_Symbol("arg1"));
  tca->assert_ref(oi, sexpr, new ProtoScalar(5));
  // arg0 = <Tuple <Scalar 8> <Scalar 5> <Scalar 10>>
  CPPUNIT_ASSERT(oi->nth_input(0)->isA("ProtoTuple"));
  CPPUNIT_ASSERT_EQUAL(4, (int)(T_TYPE(oi->nth_input(0))->types.size()));
  CPPUNIT_ASSERT(T_TYPE(oi->nth_input(0))->types[0]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(T_TYPE(oi->nth_input(0))->types[0]->isLiteral());
  CPPUNIT_ASSERT_EQUAL(8, (int)S_VAL(T_TYPE(oi->nth_input(0))->types[0]));
  CPPUNIT_ASSERT(T_TYPE(oi->nth_input(0))->types[1]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(T_TYPE(oi->nth_input(0))->types[1]->isLiteral());
  CPPUNIT_ASSERT_EQUAL(5, (int)S_VAL(T_TYPE(oi->nth_input(0))->types[1]));
  CPPUNIT_ASSERT(T_TYPE(oi->nth_input(0))->types[2]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(T_TYPE(oi->nth_input(0))->types[2]->isLiteral());
  CPPUNIT_ASSERT_EQUAL(10, (int)S_VAL(T_TYPE(oi->nth_input(0))->types[2]));
  CPPUNIT_ASSERT(T_TYPE(oi->nth_input(0))->types[3]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(T_TYPE(oi->nth_input(0))->types[3]->isLiteral());
  CPPUNIT_ASSERT_EQUAL(11, (int)S_VAL(T_TYPE(oi->nth_input(0))->types[3]));
  // arg1 = <Scalar 1>
  CPPUNIT_ASSERT(oi->nth_input(1)->isA("ProtoScalar"));
}

/*Same test as assertRefNth
void TypeCheckingTestCase::assertRefNthReplaceTup() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoTuple() );
  req_input.push_back( new ProtoScalar(0) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  opt_input.push_back( new ProtoScalar() );
  opt_input.push_back( new ProtoScalar() );
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar());
  rest->add(new ProtoScalar());
  //output
  ProtoType* output = new ProtoScalar(4);
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("nth"));
  sexpr->add(new SE_Symbol("arg0"));
  sexpr->add(new SE_Symbol("arg1"));
  tca->assert_ref(oi, sexpr, new ProtoScalar());
  cout << endl << ce2s(oi->nth_input(0)) << endl;
  CPPUNIT_ASSERT(oi->nth_input(0)->isA("ProtoTuple"));
  CPPUNIT_ASSERT(oi->nth_input(1)->isA("ProtoScalar"));
  //arg0 = <Tuple <Scalar>...>
  CPPUNIT_ASSERT_EQUAL(1, (int)(T_TYPE(oi->nth_input(0))->types.size()));
  CPPUNIT_ASSERT(T_TYPE(oi->nth_input(0))->types[0]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(!T_TYPE(oi->nth_input(0))->bounded);
}
*/

void TypeCheckingTestCase::assertRefNthUnlit() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoType() );
  req_input.push_back( new ProtoScalar(0) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  opt_input.push_back( new ProtoScalar() );
  opt_input.push_back( new ProtoScalar() );
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar());
  rest->add(new ProtoScalar());
  //output
  ProtoType* output = new ProtoScalar(4);
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("nth"));
  sexpr->add(new SE_Symbol("arg0"));
  sexpr->add(new SE_Symbol("arg2"));
  tca->assert_ref(oi, sexpr, new ProtoScalar(3));
  CPPUNIT_ASSERT(oi->nth_input(0)->isA("ProtoTuple"));
  CPPUNIT_ASSERT(oi->nth_input(2)->isA("ProtoScalar"));
}

void TypeCheckingTestCase::supertype() {
   ProtoVector* type = new ProtoVector();
   ProtoVector* value = new ProtoVector(true);
   value->add(new ProtoScalar());
   CPPUNIT_ASSERT(type->supertype_of(value));
   CPPUNIT_ASSERT(!value->supertype_of(type));
}

void TypeCheckingTestCase::fillTuple() {
   TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
   ProtoTuple* unbounded = new ProtoTuple();
   // fill unbounded to 0 (doesn't change)
   CPPUNIT_ASSERT( !tca->fillTuple(unbounded, 0) );
   // still not bounded
   CPPUNIT_ASSERT( !unbounded->bounded );
   // size = 1
   CPPUNIT_ASSERT_EQUAL( 1, (int)unbounded->types.size() );
   // type is any
   CPPUNIT_ASSERT( unbounded->types[0]->isA("ProtoType") );
   
   // fill unbounded to 1 (still doesn't change)
   CPPUNIT_ASSERT( !tca->fillTuple(unbounded, 1) );
   // still not bounded
   CPPUNIT_ASSERT( !unbounded->bounded );
   // size = 1
   CPPUNIT_ASSERT_EQUAL( 1, (int)unbounded->types.size() );
   // type is any
   CPPUNIT_ASSERT( unbounded->types[0]->isA("ProtoType") );
   
   // fill unbounded to 5
   CPPUNIT_ASSERT( tca->fillTuple(unbounded, 5) );
   // still not bounded
   CPPUNIT_ASSERT( !unbounded->bounded );
   // size = 5
   CPPUNIT_ASSERT_EQUAL( 5, (int)unbounded->types.size() );
   // types are <Any>
   CPPUNIT_ASSERT( unbounded->types[0]->isA("ProtoType") );
   CPPUNIT_ASSERT( unbounded->types[1]->isA("ProtoType") );
   CPPUNIT_ASSERT( unbounded->types[2]->isA("ProtoType") );
   CPPUNIT_ASSERT( unbounded->types[3]->isA("ProtoType") );
   CPPUNIT_ASSERT( unbounded->types[4]->isA("ProtoType") );

   ProtoTuple* bounded = new ProtoTuple(true);
   // empty bounded tuple, fill to 0 (doesn't change)
   CPPUNIT_ASSERT( !tca->fillTuple(bounded, 0) );
   // still bounded
   CPPUNIT_ASSERT( bounded->bounded );
   // size = 0
   CPPUNIT_ASSERT_EQUAL( 0, (int)bounded->types.size() );
}
