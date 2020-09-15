
#ifndef CPP_UNIT_TYPECHECKINGTESTCASE_H
#define CPP_UNIT_TYPECHECKINGTESTCASE_H

#include <cppunit/extensions/HelperMacros.h>

/**
 * A test case for Proto TypeChecking.
 */
class TypeCheckingTestCase : public CPPUNIT_NS::TestFixture
{
  CPPUNIT_TEST_SUITE( TypeCheckingTestCase );
  CPPUNIT_TEST( example );
  CPPUNIT_TEST( deliteralizeScalar );
  CPPUNIT_TEST( deliteralizeTuple );
  //get_ref
  CPPUNIT_TEST( getNthArg );
  CPPUNIT_TEST( getRefSymbol );
  CPPUNIT_TEST( getRefUnlit );
  CPPUNIT_TEST( getRefFieldof );
  CPPUNIT_TEST( getRefFt );
  CPPUNIT_TEST( getRefInputs );
  CPPUNIT_TEST( getRefLast );
  CPPUNIT_TEST( getRefLcs );
  CPPUNIT_TEST( getRefNth );
  CPPUNIT_TEST( getRefNthAllSame );
  CPPUNIT_TEST( getRefOutput );
  CPPUNIT_TEST( getRefTupof );
  CPPUNIT_TEST( getRefOptionalInputs );
  CPPUNIT_TEST( getRefRest );
  //assert_ref
  CPPUNIT_TEST( assertNthArg );
  CPPUNIT_TEST( assertNthEmptyRest );
  CPPUNIT_TEST( assertRefSymbol );
  CPPUNIT_TEST( assertRefUnlit );
  CPPUNIT_TEST( assertRefFieldof );
  CPPUNIT_TEST( assertRefFt );
  CPPUNIT_TEST( assertRefInputs );
  CPPUNIT_TEST( assertRefInputsRest );
  CPPUNIT_TEST( assertRefOutput );
  CPPUNIT_TEST( assertRefTupof );
  CPPUNIT_TEST( assertRefLast );
  CPPUNIT_TEST( assertRefLastBounded );
  CPPUNIT_TEST( assertRefLcs );
  CPPUNIT_TEST( assertRefNth );
  CPPUNIT_TEST( assertRefNthFillTup );
  CPPUNIT_TEST( assertRefNthReplaceFilledTup );
  CPPUNIT_TEST( assertRefNthUnlit );
  CPPUNIT_TEST( supertype );
  CPPUNIT_TEST( fillTuple );
  CPPUNIT_TEST_SUITE_END();

protected:

public:
  void setUp();

protected:
  void example();
  void deliteralizeScalar();
  void deliteralizeTuple();
  //get_ref
  void getNthArg();
  void getRefSymbol();
  void getRefUnlit();
  void getRefFieldof();
  void getRefFt();
  void getRefInputs();
  void getRefOutput();
  void getRefTupof();
  void getRefLast();
  void getRefLcs();
  void getRefNth();
  void getRefNthAllSame();
  void getRefOptionalInputs();
  void getRefRest();
  //assert_ref
  void assertNthArg();
  void assertNthEmptyRest();
  void assertRefSymbol();
  void assertRefUnlit();
  void assertRefFieldof();
  void assertRefFt();
  void assertRefInputs();
  void assertRefInputsRest();
  void assertRefOutput();
  void assertRefTupof();
  void assertRefLast();
  void assertRefLastBounded();
  void assertRefLcs();
  void assertRefNth();
  void assertRefNthFillTup();
  void assertRefNthReplaceFilledTup();
  void assertRefNthUnlit();
  void supertype();
  void fillTuple();
};


#endif
