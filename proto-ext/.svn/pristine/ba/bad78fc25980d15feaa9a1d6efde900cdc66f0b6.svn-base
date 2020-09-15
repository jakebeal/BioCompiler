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
/// Provides the Stack class

#ifndef __STACK_HPP
#define __STACK_HPP

#include <memory.hpp>
#include <vector>

template<typename Element>
class BasicStack {
	
	protected:
		
      std::vector<Element> stackVector;
		
	public:
		inline explicit BasicStack(Size capacity = 0) { reset(capacity); }
		
		/// Reset the stack.
		/**
		 * All elements will be deconstructed and the space will be deallocated.
		 * New space wlil be allocated if a new capacity is given.
		 *
		 * \param new_capacity The new capacity of this stack.
		 */
		inline void reset(Size new_capacity = 0) {
         stackVector.clear();
         stackVector.reserve(new_capacity);
		}
		
      inline void grow(Size additional_capacity = 1) {
         stackVector.reserve(stackVector.capacity()+additional_capacity);
		}
		
		/// The number of elements currently stored.
		inline Size size() const {
			return stackVector.size();
		}
		
		/// Check whether there are currently elements stored (false) or not (true).
		inline bool empty() const {
			return stackVector.empty();
		}
		
		/// The number of elements that can be pushed before the stack is full.
		inline Size free() const {
			return stackVector.capacity() - size();
		}
		
		/// Check whether the stack is full.
		inline bool full() const {
			return size() == stackVector.capacity();
		}
		
		/// Push a new element on the stack.
		inline void push(Element const & element) {
         stackVector.push_back(element);
		}
		
		/// Pop an element from the stack.
		inline Element pop() {
         Element ret = stackVector[size()-1];
         stackVector.pop_back();
         return ret;
		}
		
		/// Remove multiple elements from the stack.
		inline void pop(Size elements) {
         //stackVector.erase(stackVector.end(), stackVector.end()+elements);
         for(Int i=0; i<(Int)elements; ++i) pop();
		}
		
		/// Access an element by its offset from the top of the stack.
		inline Element       & peek(Size offset = 0)       { return stackVector[size()-1-offset]; }
		/// Get an element by its offset from the top of the stack.
		inline Element const & peek(Size offset = 0) const { return stackVector[size()-1-offset]; }
		
		/// Access an element by its offset from the base of the stack.
		inline Element       & operator [] (Index index)       { return stackVector[index]; }
		/// Get an element by its offset from the base of the stack.
		inline Element const & operator [] (Index index) const { return stackVector[index]; }
		
		/// Deconstruct the elements and deallocate the stack.
		~BasicStack() {
			reset();
		}
		
	private:
		inline BasicStack(BasicStack const &);
		inline BasicStack & operator = (BasicStack const &);
		
};

/// A stack with a fixed capacity.
/**
 * \tparam Element The type of elements stored on the Stack.
 */
template<typename Element>
class Stack : public BasicStack<Element> {
	
	public:
		
		/// Allocate a stack with the given capacity.
		/**
		 * \param capacity The capacity, an empty stack will be allocated when 0 or omitted.
		 */
		inline explicit Stack(Size capacity = 0) : BasicStack<Element>(capacity) {}
		
};

#endif
