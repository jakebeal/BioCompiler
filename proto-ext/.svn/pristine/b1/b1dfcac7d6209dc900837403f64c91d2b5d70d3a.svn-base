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
/// Provides the Memory class.

#include <new>

#ifndef __MEMORY_HPP
#define __MEMORY_HPP

#include "types.hpp"

/// An interface to the memory (de)allocation functions.
/**
 * \tparam Element The type of the element(s) to (de)allocate.
 */
template<typename Element>
class Memory {
	public:
		
		/// Allocate memory.
		/**
		 * Allocate memory to hold a number of Elements.
		 * 
		 * \param capacity The number of Elements, or one when omitted.
		 * 
		 * \return A pointer to the allocated memory.
		 * 
		 * \warning This does not construct the element(s).
		 * Use the placement new operator to do it yourself:
		 * \code
		 * Thing * t = Memory<Thing>::allocate();
		 * new (t) Thing();
		 * \endcode
		 */
		static inline Element * allocate(Size capacity = 1) {
			return static_cast<Element *>(operator new [] (capacity * sizeof(Element)));
		}
		
		/// Deallocate memory.
		/**
		 * Deallocate memory previously allocated by Memory::allocate.
		 * 
		 * \param memory A pointer to the to be deallocated memory.
		 * \param capacity The number of elements that were allocated by Memory::allocate, or one when omitted.
		 *
		 * \warning This does not deconstruct the element(s).
		 * You have to call the deconstructor(s) yourself:
		 * \code
		 * t->~Thing();
		 * Memory<Thing>::deallocate(t);
		 * \endcode
		 */
		static inline void deallocate(Element * memory, Size capacity = 1) {
			operator delete [] (memory);
		}
		
};

#endif
