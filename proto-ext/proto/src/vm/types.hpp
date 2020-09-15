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
/// Provides the basic numeric types.

#include <cstddef>
#include <limits>

#ifndef __TYPES_HPP
#define __TYPES_HPP

/// \class Size
/// Used to represent sizes/counts of things in memory.
typedef size_t Size;

/// \class Index
/// Used to represent positive offsets of memory arrays.
typedef size_t Index;

/// \class Counter
/// Used to count things.
typedef unsigned int Counter;

/// \class Int8
/// An 8 bit unsigned integer (byte).
typedef unsigned char Int8;

/// \class Int16
/// A 16 bit unsigned integer (two bytes).
typedef unsigned short Int16;

/// \class Int
/// A unsigned integer.
typedef unsigned int Int;

/// \class Alignment
/// The memory is aligned by this type.
typedef unsigned char Alignment;

/** \class Number
 * \brief A floating point number.
 * 
 * One of the types that can be stored in Data.
 */
typedef float Number;

/// \var Number_infinity
/// A Number representing infinity.
namespace {
	const Number Number_infinity = std::numeric_limits<Number>::infinity();
}

#endif
