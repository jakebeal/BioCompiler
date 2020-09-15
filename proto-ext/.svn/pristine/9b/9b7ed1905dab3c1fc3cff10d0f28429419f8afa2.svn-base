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
/// Provides the IEEE754binary32 class.

#ifndef __IEEE754_HPP
#define __IEEE754_HPP

#include <types.hpp>

/// An IEEE 754 binary32 floating point number representation.
/**
 * Used to convert between the platform specific floating point representation and an IEEE 754 binary32 representation.
 * 
 * This implementation assumes that on the used platform \c float is already an IEEE 754 binary32 floating point number.
 * Platforms can \ref fileoverloading "overload this file" if any conversion is needed.
 */
class IEEE754binary32 {
		
	private:
		float value;
		
	public:
		
		/// Convert from a Number.
		IEEE754binary32(Number number) : value(number) {}
		
		/// Convert to a Number.
		operator Number() { return value; }
		
		/// Convert from the IEEE 754 binary32 4 byte representation.
		IEEE754binary32(Int8 data[4]) : value(*reinterpret_cast<float *>(data)) {}
		
		/// Convert to the IEEE 754 binary32 4 byte representation.
		operator Int8 * () { return reinterpret_cast<Int8 *>(&value); }
		
};

#endif

