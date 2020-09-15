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
/// Provides the literal Number instructions.

#include <ieee754.hpp>
#include <machine.hpp>
#include <instructions.hpp>

namespace Instructions {
	
	/// \name Literals
	/// \{
	
	/// Literal Number.
	/**
	 * \param Int The value.
	 * \return The value as a Number.
	 */
	void LIT(Machine & machine){
		machine.stack.push(machine.nextInt());
	}
	
#if MIT_COMPATIBILITY != NO_MIT
	/// Literal Number.
	/**
	 * \param Int8 The value.
	 * \return The value as a Number.
	 * 
	 * \deprecated_mitproto{LIT}
	 */
	void LIT8(Machine & machine){
		machine.stack.push(machine.nextInt8());
	}
	
	/// Literal Number.
	/**
	 * \param Int16 The value.
	 * \return The value as a Number.
	 * 
	 * \deprecated_mitproto{LIT}
	 */
	void LIT16(Machine & machine){
		machine.stack.push(machine.nextInt16());
	}
#endif
	
	/// Literal Number.
	/**
	 * \tparam value The value.
	 * \return The value as a Number.
	 */
	template<int value>
	void LIT_N(Machine & machine){
		machine.stack.push(value);
	}
	
	/// Literal Number.
	/**
	 * \param IEEE754binary32 The value.
	 * \return The value as a Number.
	 */
	void LIT_FLO(Machine & machine){
		Int8 float_data[4];
		float_data[0] = machine.nextInt8();
		float_data[1] = machine.nextInt8();
		float_data[2] = machine.nextInt8();
		float_data[3] = machine.nextInt8();
		machine.stack.push(Number(IEEE754binary32(float_data)));
	}
	
	/// Positive infinity.
	/**
	 * \return \m{+\infty}
	 */
	void INF(Machine & machine){
		machine.stack.push(Number_infinity);
	}
	
#if MIT_COMPATIBILITY != MIT_ONLY
	/// Negative infinity.
	/**
	 * \return \m{-\infty}
	 */
	void NEG_INF(Machine & machine){
		machine.stack.push(-Number_infinity);
	}
#endif
	
}
