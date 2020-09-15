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
/// Provides the Data class

#ifndef __DATA_HPP
#define __DATA_HPP

#include "memory.hpp"
#include "types.hpp"
#include "tuple.hpp"
#include "address.hpp"
#include "stack.hpp"
#include "field.hpp"
#include "config.h"

/// The main data type of the VM.
/**
 * It can hold
 * \li a Number;
 * \li a Tuple;
 * \li an Address;
 * \li a FieldData; or
 * \li nothing at all (ie. 'undefined' or 'not set').
 */
class Data {
	
	public:
		/// The type of a Data object.
		enum Type {
			Type_undefined,
			Type_number,
			Type_tuple,
			Type_address,
                        Type_field
		};
		
	protected:
		
		// The value_type specifies how the value should be interpreted.
		Type value_type;
		
		// The value can contain a Number, a Tuple, an Address, or a FieldData.
		// Because some of these types have non-trivial constructors, the union does not (and can not) contain members of the non-trivial types.
		// So, instead, it only contains members of the same size as the types it should be able to contain.
		// A reinterpret_cast should be used to interface the value as a specific type.
		// Please note that we have to take care of calling constructors and deconstructors ourselves.
		union Value {
                  Alignment alignment;
                  Number number;
                  char tuple_data[sizeof(Tuple)];
                  char address_data[sizeof(Address)];
                  char   field_data[sizeof(FieldData)];
		} value;
		
	public:
		
  ///< Get a Data object representing 'undefined' (ie. 'not set').
  inline Data(                       ) : value_type(Type_undefined) { } 
  ///< Get a Data object containing a Number.
  inline Data(Number  const & number ) : value_type(Type_number   ) { value.number = number; } 
  inline Data(Tuple   const & tuple  ) : value_type(Type_tuple    ) { new (&value) Tuple  (tuple  ); } ///< Get a Data object containing a Tuple.
		inline Data(Address const & address) : value_type(Type_address  ) { new (&value) Address(address); } ///< Get a Data object containing an Address.
		inline Data(FieldData const & field) : value_type(Type_field  ) { new (&value) FieldData(field); } ///< Get a Data object containing a FieldData.
		
		/// Copy a Data object.
		inline Data & operator = (Data const & data) {
			switch(data.type()){
				case Type_undefined: reset();                 break;
				case Type_number   : reset(data.asNumber ()); break;
				case Type_tuple    : reset(data.asTuple  ()); break;
				case Type_address  : reset(data.asAddress()); break;
				case Type_field    : reset(data.asField()); break;
			}
			return *this;
		}
		
		/// Get a copy of a Data object.
		inline Data(Data const & data) : value_type(Type_undefined) {
			*this = data;
		}
		
		/// Check whether the value is set (true) or not (false).
		inline bool isSet() const {
			return value_type != Type_undefined;
		}
		
		/// Get the Type of value stored.
		inline Type type() const {
			return value_type;
		}
		
  ///< Interpret this Data object as a Number.
  inline Number        & asNumber ()       { return value.number; }
  inline Number  const & asNumber () const { return value.number; }
		
		inline Tuple         & asTuple  ()       { return *reinterpret_cast<      Tuple   *>(&value); } ///< Interpret this Data object as a Tuple.
		inline Tuple   const & asTuple  () const { return *reinterpret_cast<const Tuple   *>(&value); } ///< Interpret this Data object as a Tuple.
		
		inline Address       & asAddress()       { return *reinterpret_cast<      Address *>(&value); } ///< Interpret this Data object as an Address.
		inline Address const & asAddress() const { return *reinterpret_cast<const Address *>(&value); } ///< Interpret this Data object as an Address.

		inline FieldData     & asField  ()       { return *reinterpret_cast<      FieldData *>(&value); } ///< Interpret this Data object as a FieldData.
		inline FieldData const & asField() const { return *reinterpret_cast<const FieldData *>(&value); } ///< Interpret this Data object as a FieldData.
		
		/// Reset the value to 'undefined' (ie. 'not set').
		inline void reset() {
#ifdef GC_COMP
		    switch(value_type){
			  case Type_undefined:                 break;
			  case Type_number   : resetNumber (); break;
			  case Type_tuple    : resetTuple  (); break;
			  case Type_address  : resetAddress(); break;
			  case Type_field    : resetField  (); break;
		    }
#endif
		}
		
  inline void reset(Number  const & number ) { reset(); value_type = Type_number ; value.number = number; } ///< Set the value to a Number.
		inline void reset(Tuple   const & tuple  ) { reset(); value_type = Type_tuple  ; new (&value) Tuple  (tuple  ); } ///< Set the value to a Tuple.
		inline void reset(Address const & address) { reset(); value_type = Type_address; new (&value) Address(address); } ///< Set the value to an Address.
		inline void reset(FieldData   const & field  ) { reset(); value_type = Type_field  ; new (&value) FieldData  (field  ); } ///< Set the value to a Field.
		
		/// Make a 'real' copy of the data.
		/**
		 * This will copy the contents of a Tuple, instead of sharing them.
		 * 
		 * \see Shared
		 */
		inline Data copy() const {
			if (value_type == Type_tuple) return Data(asTuple().copy());
			if (value_type == Type_field) return Data(asField().copy());
			return *this;
		}
		
		/// Deconstruct the data.
		inline ~Data() {
			reset();
		}
		
	protected:
		inline void resetNumber () { value_type = Type_undefined; }
		inline void resetTuple  () { asTuple  ().~Tuple  (); value_type = Type_undefined; }
		inline void resetAddress() { asAddress().~Address(); value_type = Type_undefined; }
		inline void resetField  () { asField  ().~FieldData  (); value_type = Type_undefined; }
		
		friend class Stack<Data>;
		friend class DataStack;
		
};

template<>
class Stack<Data> : public BasicStack<Data> {
	
public:
  inline explicit Stack(Size capacity = 0) : BasicStack<Data>(capacity) {}
  
		inline Number  popNumber () { return pop().asNumber();  }
		inline Tuple   popTuple  () { return pop().asTuple();   }
		inline Address popAddress() { return pop().asAddress(); }
		inline FieldData popField() { return pop().asField();   }
};

class DataStack {
protected:
  Data* contents;
  Size capacity, subcapacity; // subcapacity optimizes size checks
  Size top;

public:
  inline explicit DataStack(Size capacity = 0) {
    this->capacity = 0; reset(capacity);
  }
  inline ~DataStack() { delete[] contents; }

  inline void reset(Size new_capacity = 0) {
    if(capacity) delete[] contents;
    capacity = new_capacity; subcapacity = capacity-1;
    if(new_capacity) { contents = new Data[capacity]; }
    top = -1; // nothing in stack
  }

  /// The number of elements currently stored.
  inline Size size() const {
    return top + 1;
  }
		
  /// Check whether there are currently elements stored (false) or not (true).
  inline bool empty() const {
    return top < 0;
  }
		
  /// The number of elements that can be pushed before the stack is full.
  inline Size free() const {
    return subcapacity - top;
  }
  
  /// Check whether the stack is full.
  inline bool full() const {
    return top == subcapacity;
  }
		
  /// Push a new element on the stack.
  inline void push(Data const & element) {
    if(full()) cerr << "Stack overflow: full with " << size() << " elements\n";
    contents[++top] = element;
  }

  // Assumption that all stack elements are reset/scratch
  inline void push(Number const number) {
    if(full()) cerr << "Stack overflow: full with " << size() << " elements\n";
    top++;
    contents[top].value_type = Data::Type_number;
    contents[top].value.number = number;
  }
  
  inline void set_top(Data const & element) {
    contents[top] = element;
  }
  inline void replaceNumber(Number const number) {
    // ensure we are replacing a number
    assert(contents[top].value_type == Data::Type_number);
    // just swap the contents
    contents[top].value.number = number;
  }
//   inline void replaceTuple(Tuple const & tuple) {
//     // ensure we are replacing a number
//     assert(contents[top].value_type == Data::Type_tuple);
//     // just swap the contents
//     contents[top].value.tuple = tuple;
//   }
  
  /// Pop an element from the stack.
  inline Data& pop() {
    return contents[top--];
  }
  
  /// Remove multiple elements from the stack.
  inline void pop(Size elements) {
    top -= elements; // memory leak spot
  }
  
  /// Access an element by its offset from the top of the stack.
  inline Data       & peek(Size offset = 0)       { return contents[top-offset]; }
  /// Get an element by its offset from the top of the stack.
  inline Data const & peek(Size offset = 0) const { return contents[top-offset]; }
		
  /// Access an element by its offset from the base of the stack.
  inline Data       & operator [] (Index index)       { return contents[index]; }
  /// Get an element by its offset from the base of the stack.
  inline Data const & operator [] (Index index) const { return contents[index]; }
  
  
  inline Number  popNumber () { return pop().asNumber();  }
  inline Tuple   popTuple  () { return pop().asTuple();   }
  inline Address popAddress() { return pop().asAddress(); }
  inline FieldData popField() { return pop().asField();   }
};

#endif
