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
/// Provides the SharedVector class.

#ifndef __SHAREDVECTOR_HPP
#define __SHAREDVECTOR_HPP

#include <types.hpp>
#include <memory.hpp>

/// A vector with shared contents.
/**
 * A SharedVector can only grow, not shrink. New space is automatically allocated when needed.
 * 
 * The contents will be shared across copies of an instance, unless created by copy().
 * 
 * \tparam Element The type of elements in the vector.
 */
template<typename Element>
class SharedVector {
	
	protected:
		
		class VectorData {
			
			protected:
				mutable Counter reference_count;
				Size vectorsize;
				Size vectorcapacity;
				Element * elements;
				
				inline void reset(Size size = 0) {
					if (elements){
						for(Index i = 0; i < vectorsize; i++) elements[i].~Element();
						Memory<Element>::deallocate(elements, vectorcapacity);
					}
					vectorsize = size;
					elements = vectorsize ? Memory<Element>::allocate(size) : 0;
				}
				
				inline void grow(Size extra_capacity = 1) {
					Element * new_elements = Memory<Element>::allocate(vectorcapacity + extra_capacity);
					for(Index i = 0; i < vectorsize; i++){
						new (&new_elements[i]) Element(elements[i]);
						elements[i].~Element();
					}
					if (elements) Memory<Element>::deallocate(elements,vectorcapacity);
					vectorcapacity += extra_capacity;
					elements = new_elements;
				}
				
				inline ~VectorData() {
					reset();
				}
				
			public:
				inline VectorData() : reference_count(1), vectorsize(0), vectorcapacity(0), elements(0) {}
				inline explicit VectorData(Size capacity) : reference_count(1), vectorsize(0), vectorcapacity(capacity), elements(Memory<Element>::allocate(vectorcapacity)) {}
				
				inline VectorData(VectorData const & vector, Size free_space = 0) : reference_count(1), vectorsize(0), vectorcapacity(vector.size() + free_space), elements(Memory<Element>::allocate(vectorcapacity)) {
					for(;vectorsize < vector.size(); vectorsize++) new (&elements[vectorsize]) Element(vector.elements[vectorsize]);
				}
				
				inline void push(Element const & element) {
					if (vectorsize == vectorcapacity) grow();
					new (&elements[vectorsize++]) Element(element);
				}
				
				inline Size    size      () const { return vectorsize     ; }
				inline Size    capacity  () const { return vectorcapacity ; }
				inline Counter references() const { return reference_count; }
				
				inline operator Element       * ()       { return elements; }
				inline operator Element const * () const { return elements; }
				
				inline VectorData       * grab()       { reference_count++; return this; }
				inline VectorData const * grab() const { reference_count++; return this; }
				
				inline void release() const {
					if (!--reference_count){
						this->~VectorData();
						Memory<VectorData>::deallocate(const_cast<VectorData *>(this));
					}
				}
				
				inline VectorData & operator = (VectorData const & vector){
					reset(vector.size());
					for(;vectorsize < vector.size(); vectorsize++) new (&elements[vectorsize]) Element(vector.elements[vectorsize]);
				}
				
		} * data;
		
		inline SharedVector(VectorData * data) : data(data) {}
		
	public:
		/// Construct an empty vector.
		inline SharedVector() : data(new (Memory<VectorData>::allocate()) VectorData()) {}
		
		/// Allocate a new vector with the specified initial capacity.
		inline explicit SharedVector(Size capacity) : data(new (Memory<VectorData>::allocate()) VectorData(capacity)) {}
		
		/// Construct another instance of this vector.
		/**
		 * \note The contents will be shared. Use copy() to get a copy.
		 */
		inline SharedVector(SharedVector const & vector) : data(vector.data->grab()) {}
		
		/// Get access the elements.
		inline operator Element       * ()       { return *data; }
		
		/// Get constant access the elements.
		inline operator Element const * () const { return *data; }
		
		/// Add an element to the back of the vector.
		inline void push(Element const & element) {
			data->push(element);
		}
		
		/// Get the contents from another vector.
		/**
		 * \note The contents will be shared, not copied. To get a copy, you can use:
		 * \code
		 * vector_a = vector_b.copy();
		 * \endcode
		 */
		inline SharedVector & operator = (SharedVector const & vector) {
			data->release();
			data = vector.data->grab();
			return *this;
		}
		
		/// Create a copy of this vector.
		/**
		 * All elements will be copied using their own copy constructor.
		 */
		inline SharedVector copy() const {
			return SharedVector(new (Memory<VectorData>::allocate()) VectorData(*data));
		}
		
		/// The number of elements in this vector.
		inline Size size() const {
			return data->size();
		}
		
		/// Check whether the vector is empty (true) or not (false).
		inline bool empty() const {
			return size() == 0;
		}
		
		/// The vectors current capacity.
		inline Size capacity() const {
			return data->capacity();
		}
		
		/// The number of instances with the same shared contents including this one.
		inline Counter instances() const {
			return data->references();
		}
		
		/// Deconstruct the vector.
		/**
		 * If this was the last instance of this vector, the contents will be deconstructed and deallocated as well.
		 */
		inline ~SharedVector() {
			data->release();
		}
		
};

#endif
