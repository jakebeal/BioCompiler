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
/// Provides the NeighbourHood class.

#ifndef __NEIGHBOURLIST_HPP
#define __NEIGHBOURLIST_HPP

#include <memory.hpp>
#include <types.hpp>
#include <neighbour.hpp>

/// A list of Neighbours.
/**
 * This implements a doubly linked list.
 * Though, it could be replaced by any other implementation (raw array, tree set, etc.), as long as it has the same public interface.
 * 
 * \todo Add documentation and some examples.
 */
class NeighbourHood {
	
	protected:
		
		struct NeighbourHoodElement {
			Neighbour neighbour;
			NeighbourHoodElement * next;
			NeighbourHoodElement * previous;
			inline NeighbourHoodElement(MachineId const & id, Size imports) : neighbour(id,imports), next(0), previous(0) {}
		};
		
		NeighbourHoodElement * first;
		NeighbourHoodElement * last;
		
		Size list_size;
		
		Size imports;
		
	public:
		
		class iterator {
			protected:
				NeighbourHoodElement * element;
				inline iterator(NeighbourHoodElement * element) : element(element) {}
				friend class const_iterator;
				friend class NeighbourHood;
			public:
				inline iterator() {}
				inline iterator & operator ++ (     ) {                     element = element->next    ; return *this; }
				inline iterator   operator ++ (int x) { iterator i = *this; element = element->next    ; return  i   ; }
				inline iterator & operator -- (     ) {                     element = element->previous; return *this; }
				inline iterator   operator -- (int x) { iterator i = *this; element = element->previous; return  i   ; }
				inline Neighbour & operator *  () const { return   element->neighbour ; }
				inline Neighbour * operator -> () const { return &(element->neighbour); }
				inline bool operator == (iterator const & i) const { return element == i.element; }
				inline bool operator != (iterator const & i) const { return element != i.element; }
				inline operator Neighbour * () const { return &(element->neighbour); }
		};
		
		class const_iterator {
			protected:
				NeighbourHoodElement const * element;
				inline const_iterator(NeighbourHoodElement const * element) : element(element) {}
				inline const_iterator(iterator const & i) : element(i.element) {}
				friend class NeighbourHood;
			public:
				inline const_iterator() {}
				inline const_iterator & operator ++ (     ) {                           element = element->next    ; return *this; }
				inline const_iterator   operator ++ (int x) { const_iterator i = *this; element = element->next    ; return  i   ; }
				inline const_iterator & operator -- (     ) {                           element = element->previous; return *this; }
				inline const_iterator   operator -- (int x) { const_iterator i = *this; element = element->previous; return  i   ; }
				inline Neighbour const & operator *  () const { return   element->neighbour ; }
				inline Neighbour const * operator -> () const { return &(element->neighbour); }
				inline bool operator == (const_iterator const & i) const { return element == i.element; }
				inline bool operator != (const_iterator const & i) const { return element != i.element; }
				inline operator Neighbour const * () const { return &(element->neighbour); }
		};
		
		explicit inline NeighbourHood(Size imports = 0) : first(0), last(0), list_size(0), imports(imports) {}
		
		inline void reset(Size imports){
			iterator i = first;
			while(i != end()) i = remove(i);
			this->imports = imports;
		}
		
		inline       iterator begin()       { return first; }
		inline const_iterator begin() const { return first; }
		inline       iterator end  ()       { return 0    ; }
		inline const_iterator end  () const { return 0    ; }
		
		inline Neighbour const & operator [] (MachineId const & id) const {
			return *find(id);
		}
		
		inline Neighbour & operator [] (MachineId const & id) {
			iterator i = find(id);
			return *(i == end() ? add(id) : i);
		}
		
		inline iterator find(MachineId const & id) {
			for(iterator i = begin(); i != end(); i++) if (i->id == id) return i;
			return end();
		}
		
		inline const_iterator find(MachineId const & id) const {
			for(const_iterator i = begin(); i != end(); i++) if (i->id == id) return i;
			return end();
		}
		
		inline iterator add(MachineId const & id) {
			NeighbourHoodElement * n = Memory<NeighbourHoodElement>::allocate();
			new (n) NeighbourHoodElement(id,imports);
			if (last) last->next = n;
			n->previous = last;
			last = n;
			if (!first) first = n;
			list_size++;
			return n;
		}
		
		inline iterator remove(iterator neighbour) {
			NeighbourHoodElement * n = neighbour.element;
			if (n->previous) n->previous->next = n->next;
			if (n->next    ) n->next->previous = n->previous;
			if (n == first) first = n->next;
			if (n == last ) last  = n->previous;
			iterator next = n->next;
			n->~NeighbourHoodElement();
			Memory<NeighbourHoodElement>::deallocate(n);
			list_size--;
			return next;
		}
		
		inline Size size () const { return  list_size; }
		inline bool empty() const { return !list_size; }
		
};

#endif
