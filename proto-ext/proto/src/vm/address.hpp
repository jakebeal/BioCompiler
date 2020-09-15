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
/// Provides the Address class

#ifndef __ADDRESS_HPP
#define __ADDRESS_HPP

#include "types.hpp"

/// An address of a constant series of bytes in memory.
/**
 * One of the types that can be stored in Data.
 * 
 * \note This is just a wrapper for an Int8 constant pointer, with an explicit constructor.
 *       This is useful, because \c 0 cannot be interpreted as an Address (unlike <tt>Int8 *</tt>),
 *       you have to explicitly say <tt>Address(0)</tt>.
 *       This makes something like \code Data d = 0; \endcode unambiguous.
 *       (It's interpreted as a Number, not an Address.)
 */
class Address {
	protected:
		Int8 const * address;
	public:
		inline explicit Address(Int8 const * const & address = 0) : address(address) {}
		inline operator Int8 const *       & ()       { return address; }
		inline operator Int8 const * const & () const { return address; }
};

#endif
