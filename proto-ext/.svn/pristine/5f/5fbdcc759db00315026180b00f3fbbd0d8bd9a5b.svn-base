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

template<typename Element>
class BasicStack {
	
	protected:
		
		/// The maximum number of elements.
		Size capacity;
		
		/// A pointer to the beginning of the stack.
		Element * base;
		
		/// A pointer to the end of the stack.
		Element * top;
		
	public:
		inline explicit BasicStack(Size capacity = 0) : base(0), top(0) { reset(capacity); }
		
		/// Reset the stack.
		/**
		 * All elements will be deconstructed and the space will be deallocated.
		 * New space wlil be allocated if a new capacity is given.
		 *
		 * \param new_capacity The new capacity of this stack.
		 */
		inline void reset(Size new_capacity = 0) {
			pop(size());
			if (base) Memory<Element>::deallocate(base,capacity);
			top = base = new_capacity ? Memory<Element>::allocate(new_capacity) : 0;
			capacity = new_capacity;
		}
		
		/// The number of elements currently stored.
		inline Size size() const {
			return top - base;
		}
		
		/// Check whether there are currently elements stored (false) or not (true).
		inline bool empty() const {
			return top == base;
		}
		
		/// The number of elements that can be pushed before the stack is full.
		inline Size free() const {
			return capacity - size();
		}
		
		/// Check whether the stack is full.
		inline bool full() const {
			return size() == capacity;
		}
		
		/// Push a new element on the stack.
		inline void push(Element const & element) {
			new (top++) Element(element);
		}
		
		/// Pop an element from the stack.
		inline Element pop() {
			Element element = *--top;
			top->~Element();
			return element;
		}
		
		/// Remove multiple elements from the stack.
		inline void pop(Size elements) {
			top -= elements;
			for(Index i = 0; i < elements; i++) top[i].~Element();
		}
		
		/// Access an element by its offset from the top of the stack.
		inline Element       & peek(Size offset = 0)       { return top[-1-offset]; }
		/// Get an element by its offset from the top of the stack.
		inline Element const & peek(Size offset = 0) const { return top[-1-offset]; }
		
		/// Access an element by its offset from the base of the stack.
		inline Element       & operator [] (Index index)       { return base[index]; }
		/// Get an element by its offset from the base of the stack.
		inline Element const & operator [] (Index index) const { return base[index]; }
		
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
