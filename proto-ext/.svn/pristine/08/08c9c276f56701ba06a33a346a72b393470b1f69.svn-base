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

#include <memory.hpp>
#include <types.hpp>
#include <tuple.hpp>
#include <address.hpp>
#include <stack.hpp>
#include "config.h"

/// The main data type of the VM.
/**
 * It can hold
 * \li a Number;
 * \li a Tuple;
 * \li an Address; or
 * \li nothing at all (ie. 'undefined' or 'not set').
 */
class Data {
	
	public:
		/// The type of a Data object.
		enum Type {
			Type_undefined,
			Type_number,
			Type_tuple,
			Type_address
		};
		
	protected:
		
		// The value_type specifies how the value should be interpreted.
		Type value_type;
		
		// The value can contain a Number, a Tuple or an Address.
		// Because some of these types have non-trivial constructors, the union does not (and can not) contain members of these types.
		// So, instead, it only contains members of the same size as the types it should be able to contain.
		// A reinterpret_cast should be used to interface the value as a specific type.
		// Please note that we have to take care of calling constructors and deconstructors ourselves.
		union Value {
			Alignment alignment;
			char  number_data[sizeof(Number )];
			char   tuple_data[sizeof(Tuple  )];
			char address_data[sizeof(Address)];
		} value;
		
	public:
		
		inline Data(                       ) : value_type(Type_undefined) {                                } ///< Get a Data object representing 'undefined' (ie. 'not set').
		inline Data(Number  const & number ) : value_type(Type_number   ) { new (&value) Number (number ); } ///< Get a Data object containing a Number.
		inline Data(Tuple   const & tuple  ) : value_type(Type_tuple    ) { new (&value) Tuple  (tuple  ); } ///< Get a Data object containing a Tuple.
		inline Data(Address const & address) : value_type(Type_address  ) { new (&value) Address(address); } ///< Get a Data object containing an Address.
		
		/// Copy a Data object.
		inline Data & operator = (Data const & data) {
			switch(data.type()){
				case Type_undefined: reset();                 break;
				case Type_number   : reset(data.asNumber ()); break;
				case Type_tuple    : reset(data.asTuple  ()); break;
				case Type_address  : reset(data.asAddress()); break;
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
		
		inline Number        & asNumber ()       { return *reinterpret_cast<      Number  *>(&value); } ///< Interpret this Data object as a Number.
		inline Number  const & asNumber () const { return *reinterpret_cast<const Number  *>(&value); } ///< Interpret this Data object as a Number.
		
		inline Tuple         & asTuple  ()       { return *reinterpret_cast<      Tuple   *>(&value); } ///< Interpret this Data object as a Tuple.
		inline Tuple   const & asTuple  () const { return *reinterpret_cast<const Tuple   *>(&value); } ///< Interpret this Data object as a Tuple.
		
		inline Address       & asAddress()       { return *reinterpret_cast<      Address *>(&value); } ///< Interpret this Data object as an Address.
		inline Address const & asAddress() const { return *reinterpret_cast<const Address *>(&value); } ///< Interpret this Data object as an Address.
		
		/// Reset the value to 'undefined' (ie. 'not set').
		inline void reset() {
#ifdef GC_COMP
		    switch(value_type){
			  case Type_undefined:                 break;
			  case Type_number   : resetNumber (); break;
			  case Type_tuple    : resetTuple  (); break;
			  case Type_address  : resetAddress(); break;
		    }
#endif
		}
		
		inline void reset(Number  const & number ) { reset(); value_type = Type_number ; new (&value) Number (number ); } ///< Set the value to a Number.
		inline void reset(Tuple   const & tuple  ) { reset(); value_type = Type_tuple  ; new (&value) Tuple  (tuple  ); } ///< Set the value to a Tuple.
		inline void reset(Address const & address) { reset(); value_type = Type_address; new (&value) Address(address); } ///< Set the value to an Address.
		
		/// Make a 'real' copy of the data.
		/**
		 * This will copy the contents of a Tuple, instead of sharing them.
		 * 
		 * \see Shared
		 */
		inline Data copy() const {
			if (value_type == Type_tuple) return Data(asTuple().copy());
			return *this;
		}
		
		/// Deconstruct the data.
		inline ~Data() {
			reset();
		}
		
	protected:
		inline void resetNumber () { asNumber ().~Number (); value_type = Type_undefined; }
		inline void resetTuple  () { asTuple  ().~Tuple  (); value_type = Type_undefined; }
		inline void resetAddress() { asAddress().~Address(); value_type = Type_undefined; }
		
		friend class Stack<Data>;
		
};

template<>
class Stack<Data> : public BasicStack<Data> {
	
	public:
		inline explicit Stack(Size capacity = 0) : BasicStack<Data>(capacity) {}
		
		inline Number  popNumber () { return pop().asNumber();  }
		inline Tuple   popTuple  () { return pop().asTuple();   }
		inline Address popAddress() { return pop().asAddress(); }
		
};

#endif
