/*   ____       _  __ _   ____            _
 *  |  _ \  ___| |/ _| |_|  _ \ _ __ ___ | |_ ___
 *  | | | |/ _ \ | |_| __| |_) | '__/ _ \| __/ _ \
 *  | |_| |  __/ |  _| |_|  __/| | ( (_) | |( (_) )
 *  |____/ \___|_|_|  \__|_|   |_|  \___/ \__\___/
 *
 * This file is part of DelftProto.
 * See COPYING for license details.
 */

/// \file
/// Provides the math instructions.

#include <math.hpp>
#include <random.hpp>
#include <machine.hpp>
#include <instructions.hpp>

namespace {
	
   bool isNaN(float f) {
      volatile float a = f;
      return a != a;
   }

	Tuple ensureTuple(Data const & d) {
		if (d.type() == Data::Type_number) {
			Tuple t(1);
			t.push(d);
			return t;
		}
		return d.asTuple();
	}
	
	float compare(Data const & a, Data const & b) {
		if (a.type() == Data::Type_number && b.type() == Data::Type_number) {
			Number aa = a.asNumber();
			Number bb = b.asNumber();
         if (isNaN(aa) || isNaN(bb)) // NaN
            return NAN;
			return aa == bb ? 0 : aa < bb ? -1 : 1;
		} else {
			Tuple aa = ensureTuple(a);
			Tuple bb = ensureTuple(b);
			Size size = aa.size() > bb.size() ? aa.size() : bb.size();
			for(Index i = 0; i < size; i++){
				Number a_element = i < aa.size() ? aa[i].asNumber() : 0;
				Number b_element = i < bb.size() ? bb[i].asNumber() : 0;
				if      (isNaN(a_element) 
                       || isNaN(b_element)) return NAN;
            else if (a_element < b_element) return -1;
				else if (a_element > b_element) return  1;
			}
			return 0;
		}
	}
	
	float compare(Machine & machine) {
		Data b = machine.stack.pop();
		Data a = machine.stack.pop();
		return compare(a, b);
	}

}

namespace Instructions {
	
	/// \name Comparison instructions
	/// \{
	
	/// Check if two numbers or vectors are equal.
	/**
	 * \param Data \m{a}
	 * \param Data \m{b}
	 * \return \m{\left\lbrace\begin{array}{ll}1&a=b\\0&a\neq b\end{array}\right.}
	 * 
	 * \note If used on tuples, when one of the tuples is shorter, the remaining of the elements will be interpreted as 0.
	 */
	void EQ(Machine & machine){
		machine.stack.push(compare(machine) == 0 ? 1 : 0);
	}
	
#if MIT_COMPATIBILITY != MIT_ONLY
	/// Check if two numbers or vectors are equal.
	/**
	 * \param Data \m{a}
	 * \param Data \m{b}
	 * \return \m{\left\lbrace\begin{array}{ll}1&a\neq b\\0&a=b\end{array}\right.}
	 * 
	 * \note If used on tuples, when one of the tuples is shorter, the remaining of the elements will be interpreted as 0.
	 */
	void NEQ(Machine & machine){
		machine.stack.push(compare(machine) != 0 ? 1 : 0);
	}
#endif
	
	/// Check if a number or vector is (lexicographically) less than another one.
	/**
	 * \param Data \m{a}
	 * \param Data \m{b}
	 * \return \m{\left\lbrace\begin{array}{ll}1&a<b\\0&a\geq b\end{array}\right.}
	 * 
	 * \note If used on tuples, when one of the tuples is shorter, the remaining of the elements will be interpreted as 0.
	 */
	void LT(Machine & machine){
      float c = compare(machine);
      if (isNaN(c))
         machine.stack.push(0);
      else
         machine.stack.push(c == -1 ? 1 : 0);
	}
	
	/// Check if a number or vector is (lexicographically) less than or equal to another one.
	/**
	 * \param Data \m{a}
	 * \param Data \m{b}
	 * \return \m{\left\lbrace\begin{array}{ll}1&a\leq b\\0&a>b\end{array}\right.}
	 * 
	 * \note If used on tuples, when one of the tuples is shorter, the remaining of the elements will be interpreted as 0.
	 */
	void LTE(Machine & machine){
      float c = compare(machine);
      if (isNaN(c))
         machine.stack.push(0);
      else
         machine.stack.push(c != 1 ? 1 : 0);
	}
	
	/// Check if a number or vector is (lexicographically) greater than another one.
	/**
	 * \param Data \m{a}
	 * \param Data \m{b}
	 * \return \m{\left\lbrace\begin{array}{ll}1&a>b\\0&a\leq b\end{array}\right.}
	 * 
	 * \note If used on tuples, when one of the tuples is shorter, the remaining of the elements will be interpreted as 0.
	 */
	void GT(Machine & machine){
      float c = compare(machine);
      if (isNaN(c))
         machine.stack.push(0);
      else
         machine.stack.push(c == 1 ? 1 : 0);
	}
	
	/// Check if a number or vector is (lexicographically) greater than or equal to another one.
	/**
	 * \param Data \m{a}
	 * \param Data \m{b}
	 * \return \m{\left\lbrace\begin{array}{ll}1&a\geq b\\0&a<b\end{array}\right.}
	 * 
	 * \note If used on tuples, when one of the tuples is shorter, the remaining of the elements will be interpreted as 0.
	 */
	void GTE(Machine & machine){
      float c = compare(machine);
      if (isNaN(c))
         machine.stack.push(0);
      else
         machine.stack.push(c != -1 ? 1 : 0);
	}
	
	/// Get the inverse boolean value of a number.
	/**
	 * \param Number \m{a}
	 * \return \m{\left\lbrace\begin{array}{ll}1&a=0\\0&a\neq0\end{array}\right.}
	 */
	void NOT(Machine & machine){
		Number a = machine.stack.popNumber();
		machine.stack.push(a ? 0 : 1);
	}
	
	/// \}
	
	/// \name Standard math operator instructions
	/// \{
	
	/// Add a number or vector (element-wise) to another one
	/**
	 * \param Data \m{a}
	 * \param Data \m{b}
	 * \return \m{a + b}
	 * 
	 * \note If used on tuples, when one of the tuples is shorter, the remaining of the elements will be interpreted as 0.
	 */
	void ADD(Machine & machine){
		Data b = machine.stack.pop();
		Data a = machine.stack.pop();
		if (a.type() == Data::Type_number && b.type() == Data::Type_number) {
			Number aa = a.asNumber();
			Number bb = b.asNumber();
			machine.stack.push(aa + bb);
		} else {
			Tuple aa = ensureTuple(a);
			Tuple bb = ensureTuple(b);
			Size size = aa.size() > bb.size() ? aa.size() : bb.size();
			Tuple result(size);
			for(Index i = 0; i < size; i++){
				Number a_element = i < aa.size() ? aa[i].asNumber() : 0;
				Number b_element = i < bb.size() ? bb[i].asNumber() : 0;
				result.push(a_element + b_element);
			}
			machine.stack.push(result);
		}
	}
	
	/// Subtract a number or vector (element-wise) from another one.
	/**
	 * \param Number \m{a}
	 * \param Number \m{b}
	 * \return \m{a - b}
	 * 
	 * \note If used on tuples, when one of the tuples is shorter, the remaining of the elements will be interpreted as 0.
	 */
	void SUB(Machine & machine){
		Data b = machine.stack.pop();
		Data a = machine.stack.pop();
		if (a.type() == Data::Type_number && b.type() == Data::Type_number) {
			Number aa = a.asNumber();
			Number bb = b.asNumber();
			machine.stack.push(aa - bb);
		} else {
			Tuple aa = ensureTuple(a);
			Tuple bb = ensureTuple(b);
			Size size = aa.size() > bb.size() ? aa.size() : bb.size();
			Tuple result(size);
			for(Index i = 0; i < size; i++){
				Number a_element = i < aa.size() ? aa[i].asNumber() : 0;
				Number b_element = i < bb.size() ? bb[i].asNumber() : 0;
				result.push(a_element - b_element);
			}
			machine.stack.push(result);
		}
	}
	
	/// Multiply a number or vector (element-wise) with a number.
	/**
	 * \param Data \m{a}
	 * \param Data \m{b}
	 * \return \m{a \cdot b}
	 */
	void MUL(Machine & machine){
		Data b = machine.stack.pop();
		Data a = machine.stack.pop();
		if (a.type() == Data::Type_number && b.type() == Data::Type_number) {
			Number aa = a.asNumber();
			Number bb = b.asNumber();
			machine.stack.push(aa * bb);
		} else {
			Number        factor = a.type() == Data::Type_number ? a.asNumber() : b.asNumber();
			Tuple const & vector = a.type() == Data::Type_number ? b.asTuple () : a.asTuple ();
			Tuple result(vector.size());
			for(Index i = 0; i < vector.size(); i++) result.push(vector[i].asNumber() * factor);
			machine.stack.push(result);
		}
	}
	
	/// Divide a number or vector (element-wise) by another.
	/**
	 * \param Number \m{a}
	 * \param Number \m{b}
	 * \return \m{\frac a b}
	 */
	void DIV(Machine & machine){
		Data b = machine.stack.pop();
		Data a = machine.stack.pop();
		if (a.type() == Data::Type_number) {
			machine.stack.push(a.asNumber() / b.asNumber());
		} else {
			Number        divisor = b.asNumber();
			Tuple const & vector  = a.asTuple ();
			Tuple result(vector.size());
			for(Index i = 0; i < vector.size(); i++) result.push(vector[i].asNumber() / divisor);
			machine.stack.push(result);
		}
	}
	
	/// Multiply two vectors (element-wise).
	/**
	 * \param Data \m{\vec a}
	 * \param Data \m{\vec b}
	 * \return \m{\vec a \cdot \vec b}
	 */
	void DOT(Machine & machine){
		Tuple a = ensureTuple(machine.stack.pop());
		Tuple b = ensureTuple(machine.stack.pop());
		Size size = a.size() > b.size() ? a.size() : b.size();
		Tuple result(size);
		for(Index i = 0; i < size; i++){
			Number a_element = i < a.size() ? a[i].asNumber() : 0;
			Number b_element = i < b.size() ? b[i].asNumber() : 0;
			result.push(a_element * b_element);
		}
		machine.stack.push(result);
	}
	
	/// \}
	
	/// \name Math function instructions
	/// \{
	
	/// Get the absolute value of a number, or the magnetude of a vector.
	/**
	 * \param Data \m{a}
	 * \return \m{|a|}
	 */
	void ABS(Machine & machine){
		Data a = machine.stack.pop();
		if (a.type() == Data::Type_number) {
			Number aa = a.asNumber();
			machine.stack.push(aa < 0 ? -aa : aa);
		} else {
			Number s = 0;
			Tuple const & vector = a.asTuple();
			for(Index i = 0; i < vector.size(); i++) {
				Number e = vector[i].asNumber();
				s += e*e;
			}
			machine.stack.push(sqrt(s));
		}
	}
	
	/// Get the (lexicographical) maximum of two numbers or vectors.
	/**
	 * \param Data \m{a}
	 * \param Data \m{b}
	 * \return \m{\max\lbrace a\,b\rbrace}
	 * 
	 * \note If used on tuples, when one of the tuples is shorter, the remaining of the elements will be interpreted as 0.
	 */
	void MAX(Machine & machine){
		Data b = machine.stack.pop();
		Data a = machine.stack.pop();
		machine.stack.push(compare(a,b) > 0 ? a : b);
	}
	
	/// Get the (lexicographical) minimum of two numbers or vectors.
	/**
	 * \param Data \m{a}
	 * \param Data \m{b}
	 * \return \m{\min\lbrace a\,b\rbrace}
	 * 
	 * \note If used on tuples, when one of the tuples is shorter, the remaining of the elements will be interpreted as 0.
	 */
	void MIN(Machine & machine){
		Data b = machine.stack.pop();
		Data a = machine.stack.pop();
		machine.stack.push(compare(a,b) < 0 ? a : b);
	}
	
	/// Get a number to the power of another.
	/**
	 * \param Number \m{a}
	 * \param Number \m{b}
	 * \return \m{a^b}
	 */
	void POW(Machine & machine){
		Number b = machine.stack.popNumber();
		Number a = machine.stack.popNumber();
		machine.stack.push(pow(a,b));
	}
	
	/// Get the remainder of a number divided by another.
	/**
	 * \param Number \m{a}
	 * \param Number \m{b}
	 * \return \m{x \equiv a \pmod{|b|}\,\quad \left\lbrace\begin{array}{ll} -|b| < x \leq 0 & a < 0 \\ 0 \leq x < |b| & a \geq 0 \end{array}\right.}
	 */
	void REM(Machine & machine){
		Number b = machine.stack.popNumber();
		Number a = machine.stack.popNumber();
		machine.stack.push(fmod(a,b));
	}
	
	/// Get the (positive) remainder of a number divided by another.
	/**
	 * \param Number \m{a}
	 * \param Number \m{b}
	 * \return \m{x \equiv a \pmod{|b|}\,\quad 0 \leq x < |b|}
	 */
	void MOD(Machine & machine){
		Number b = machine.stack.popNumber();
		Number a = machine.stack.popNumber();
		Number x = fmod(a,b);
		if (x < 0) x += b; // fmod gives the remainder, not the modulus.
		machine.stack.push(x);
	}
	
	/// Get the floor of a number.
	/**
	 * \param Number \m{a}
	 * \return \m{\left\lfloor a \right\rfloor}
	 */
	void FLOOR(Machine & machine){
		Number a = machine.stack.popNumber();
		machine.stack.push(floor(a));
	}
	
	/// Get the ceiling of a number.
	/**
	 * \param Number \m{a}
	 * \return \m{\left\lceil a \right\rceil}
	 */
	void CEIL(Machine & machine){
		Number a = machine.stack.popNumber();
		machine.stack.push(ceil(a));
	}
	
	/// Round a number.
	/**
	 * \param Number \m{a}
	 * \return \m{\left\lfloor a + \frac12 \right\rfloor}
	 */
	void ROUND(Machine & machine){
		Number a = machine.stack.popNumber();
		machine.stack.push(rint(a));
	}
	
	/// Calculate the natural logarithm of a number.
	/**
	 * \param Number \m{a}
	 * \return \m{\log_e a}
	 */
	void LOG(Machine & machine){
		Number a = machine.stack.popNumber();
		machine.stack.push(log(a));
	}
	
	/// Calculate the square root of a number.
	/**
	 * \param Number \m{a}
	 * \return \m{\sqrt a}
	 */
	void SQRT(Machine & machine){
		Number a = machine.stack.popNumber();
		machine.stack.push(sqrt(a));
	}
	
	/// Calculate the sine of an angle.
	/**
	 * \param Number \m{a}
	 * \return \m{\sin a}
	 */
	void SIN(Machine & machine){
		Number a = machine.stack.popNumber();
		machine.stack.push(sin(a));
	}
	
	/// Calculate the cosine of an angle.
	/**
	 * \param Number \m{a}
	 * \return \m{\cos a}
	 */
	void COS(Machine & machine){
		Number a = machine.stack.popNumber();
		machine.stack.push(cos(a));
	}
	
	/// Calculate the tangent of an angle.
	/**
	 * \param Number \m{a}
	 * \return \m{\tan a}
	 */
	void TAN(Machine & machine){
		Number a = machine.stack.popNumber();
		machine.stack.push(tan(a));
	}
	
	/// Calculate the hyperbolic sine of an angle.
	/**
	 * \param Number \m{a}
	 * \return \m{\sinh a}
	 */
	void SINH(Machine & machine){
		Number a = machine.stack.popNumber();
		machine.stack.push(sinh(a));
	}
	
	/// Calculate the hyperbolic cosine of an angle.
	/**
	 * \param Number \m{a}
	 * \return \m{\cosh a}
	 */
	void COSH(Machine & machine){
		Number a = machine.stack.popNumber();
		machine.stack.push(cosh(a));
	}
	
	/// Calculate the hyperbolic tangent of an angle.
	/**
	 * \param Number \m{a}
	 * \return \m{\tanh a}
	 */
	void TANH(Machine & machine){
		Number a = machine.stack.popNumber();
		machine.stack.push(tanh(a));
	}
	
	/// Calculate the angle of a sine.
	/**
	 * \param Number \m{a}
	 * \return \m{\arcsin a}
	 */
	void ASIN(Machine & machine){
		Number a = machine.stack.popNumber();
		machine.stack.push(asin(a));
	}
	
	/// Calculate the angle of a cosine.
	/**
	 * \param Number \m{a}
	 * \return \m{\arccos a}
	 */
	void ACOS(Machine & machine){
		Number a = machine.stack.popNumber();
		machine.stack.push(acos(a));
	}
	
	/// Calculate the angle of a tangent.
	/**
	 * \param Number \m{y}
	 * \param Number \m{x}
	 * \return \m{\arctan \frac y x}
	 */
	void ATAN2(Machine & machine){
		Number x = machine.stack.popNumber();
		Number y = machine.stack.popNumber();
		machine.stack.push(atan2(y,x));
	}
	
	/// Generate a (pseudo) random number or vector.
	/**
	 * \param Data \m{min}
	 * \param Data \m{max}
	 * \return \m{x \sim U(min\,max)}
	 * 
	 * \note If this instruction is used on vectors, the instruction will operate for every corresponding element in the vectors and give a vector as result.
	 * \note If used on vectors, when one of the vectors is shorter, the remaining of the elements will be interpreted as 0.
	 */
	void RND(Machine & machine){
		Data max = machine.stack.pop();
		Data min = machine.stack.pop();
		if (min.type() == Data::Type_number && max.type() == Data::Type_number) {
			Number a = min.asNumber();
			Number b = max.asNumber();
			machine.stack.push(Random::number(a,b));
		} else {
			Tuple a = ensureTuple(min);
			Tuple b = ensureTuple(max);
			Size size = a.size() > b.size() ? a.size() : b.size();
			Tuple result(size);
			for(Index i = 0; i < size; i++){
				Number a_element = i < a.size() ? a[i].asNumber() : 0;
				Number b_element = i < b.size() ? b[i].asNumber() : 0;
				result.push(Random::number(a_element, b_element));
			}
			machine.stack.push(result);
		}
	}
	
	/// \}
	
}
