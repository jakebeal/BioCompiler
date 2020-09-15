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
/// Provides the Array class.

#ifndef __ARRAY_HPP
#define __ARRAY_HPP

#include <types.hpp>
#include <memory.hpp>

/// A simple array.
/**
 * \tparam Element The type of elements stored in the array.
 */
template<typename Element> class Array {
	
	protected:
		
		/// The number of elements in the array.
		Size array_size;
		
		/// A pointer to the first element.
		Element * array;
		
	public:
		/// Allocate an array of the specified size, or an empty one when the size is omitted.
		/**
		 * All elements will be constructed with their default constructor.
		 */
		explicit Array(Size size = 0) : array(0) { reset(size); }
		
		/// Make a copy of an array.
		Array(Array<Element> const & a) : array(0) {
			*this = a;
		}
		
		/// Reset the array.
		/**
		 * All elements will be deconstructed and the array will be deallocated.
		 * If a new size is given, new elements will be allocated and constructed with their default constructor.
		 *
		 * \param new_size The new size of the array.
		 */
		inline void reset(Size new_size = 0) {
			if (array){
				for(Size i = 0; i < array_size; i++) array[i].~Element();
				Memory<Element>::deallocate(array,array_size);
			}
			array = new_size ? Memory<Element>::allocate(new_size) : 0;
			array_size = new_size;
			for(Size i = 0; i < array_size; i++) new (&array[i]) Element();
		}
		
		/// Copy the contents of an array.
		/**
		 * All current elements (if any) will be deconstructed and the array will be deallocated,
		 * after which a copy of the given array will be made.
		 */
		Array<Element> & operator = (Array<Element> const & a) {
			if (array){
				for(Size i = 0; i < array_size; i++) array[i].~Element();
				Memory<Element>::deallocate(array,array_size);
			}
			array = a.size() ? Memory<Element>::allocate(a.size()) : 0;
			array_size = a.size();
			for(Size i = 0; i < array_size; i++) new (&array[i]) Element(a[i]);
		}
		
		/// Clear the array.
		/**
		 * All elements will be reset to their default value.
		 */
		inline void clear() {
			for(Size i = 0; i < array_size; i++) array[i] = Element();
		}
		
		inline operator Element       * ()       { return array; } ///< Access the elements of the Array.
		inline operator Element const * () const { return array; } ///< Constant access to the elements of the Array. 
		
		/// The number of elements in the array.
		inline Size size() const { return array_size; }
		
		/// Deconstruct the elements and deallocate the array.
		inline ~Array() { reset(); }
		
};

#endif
