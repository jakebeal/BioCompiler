
// Utilities...

var MIT_COMPATIBILITY = { 
   NO_MIT : false,
   MIT_ONLY : false 
};

var curOpcode = 0;

var getIndex = function(machine, n) {
   var index = n;
   if(n != null && typeof n != "undefined") {
      //console.log("Has n: " + index);
   } 
   else {
      index = machine.nextVLQInt();
      //console.log("No n: " + index);
   }
   return index;
};

// Instructions...

var def_vm = function(machine) {
   //console.log("Initializing the VM");
   // These aren't being interpreted correctly, but it doesn't matter,
   // because they are being ignored, since everything is dynamically sized.
   machine.nextInt8();
   machine.nextInt8();
   var    export_length = machine.nextInt8();
   var     exports_size = machine.nextInt8();
   var     globals_size = machine.nextInt8();
   var       state_size = machine.nextInt8();
   var       stack_size = machine.nextInt8();
   var environment_size = machine.nextInt8();
   //console.log("["
   //   + export_length + ", "
   //   + exports_size  + ", "
   //   + globals_size  + ", "
   //   + state_size    + ", "
   //   + environment_size + "]"
   //   );

   //machine.      stack.reset(   stack_size+20); // MIT Proto calculates the stack size slightly different than how DelftProto uses it. Add 20 to be safe.
   //machine.environment.reset(environment_size);
   //machine.    globals.reset(    globals_size);
   //machine.    threads.reset(               1);
   //machine.      state.reset(      state_size);
   //machine.       hood.reset(    exports_size);

   // Add myself to my neighborhood
   machine.deliverMessage(machine);
};
addInstruction(curOpcode++, "DEF_VM", def_vm);
addInstruction(curOpcode++, "DEF_VM_EX", def_vm);
addInstruction(curOpcode++, "EXIT", function(machine) {
   machine.callbacks.popN(machine.callbacks.length);
});
addInstruction(curOpcode++, "RET", function(machine) {
   machine.retn();
});
addInstruction(curOpcode++, "ALL", function(machine) {
   var data = machine.stack.peek();
   machine.stack.popN(machine.nextVLQInt());
   machine.stack.push(data);
});
addInstruction(curOpcode++, "NOP", function(machine) {
   // No operation
});
addInstruction(curOpcode++, "MUX", function(machine) {
   var false_value = machine.stack.pop();
   var true_value = machine.stack.pop();
   var condition = machine.stack.pop();
   machine.stack.push(condition ? true_value : false_value);
});

if(MIT_COMPATIBILITY && !MIT_COMPATIBILITY.NO_MIT) {
   addInstruction(curOpcode++, "VMUX", function(machine) {
		machine.nextInt8();
		var false_value = machine.stack.pop();
		var true_value = machine.stack.pop();
		var condition = machine.stack.pop();
		machine.stack.push(condition ? true_value : false_value);
   });
}

var if_func = function(machine) {
   var skip = machine.nextVLQInt();
   if (machine.stack.pop()) 
      machine.skip(skip);
};
addInstruction(curOpcode++, "IF", if_func);

if(MIT_COMPATIBILITY && !MIT_COMPATIBILITY.NO_MIT) {
   addInstruction(curOpcode++, "IF16", if_func);
}

var jmp = function(machine) {
   machine.skip(machine.nextVLQInt());
}
addInstruction(curOpcode++, "JMP", jmp);

if(MIT_COMPATIBILITY && !MIT_COMPATIBILITY.NO_MIT) {
   addInstruction(curOpcode++, "JMP16", jmp);
}

addInstruction(curOpcode++, "DT", function(machine) {
   machine.stack.push(machine.desired_period);
});
addInstruction(curOpcode++, "SET_DT", function(machine) {
   machine.desired_period = machine.stack.peek();
});

addInstruction(curOpcode++, "TRIGGER", function(machine) {
   // Only for threads...
});
addInstruction(curOpcode++, "ACTIVATE", function(machine) {
   // Only for threads...
});
addInstruction(curOpcode++, "DEACTIVATE", function(machine) {
   // Only for threads...
});

var ref = function(machine, n) {
   var index = getIndex(machine, n);
   machine.stack.push(machine.environment.peek(index));
};
addInstruction(curOpcode++, "REF", ref, 0);
addInstruction(curOpcode++, "REF", ref, 1);
addInstruction(curOpcode++, "REF", ref, 2);
addInstruction(curOpcode++, "REF", ref, 3);
addInstruction(curOpcode++, "REF", ref);

var let = function(machine, n) {
   var elements = getIndex(machine, n);
   for(var i = 1; i <= elements; i++) {
      machine.environment.push(machine.stack.peek(elements-i));
   }
   machine.stack.popN(elements);
};
addInstruction(curOpcode++, "LET", let, 1);
addInstruction(curOpcode++, "LET", let, 2);
addInstruction(curOpcode++, "LET", let, 3);
addInstruction(curOpcode++, "LET", let, 4);
addInstruction(curOpcode++, "LET", let);

var pop_let = function(machine, n) {
   var elements = getIndex(machine, n);
   machine.environment.popN(elements);
};
addInstruction(curOpcode++, "POP_LET", pop_let, 1);
addInstruction(curOpcode++, "POP_LET", pop_let, 2);
addInstruction(curOpcode++, "POP_LET", pop_let, 3);
addInstruction(curOpcode++, "POP_LET", pop_let, 4);
addInstruction(curOpcode++, "POP_LET", pop_let);

if(MIT_COMPATIBILITY && !MIT_COMPATIBILITY.NO_MIT) {
   addInstruction(curOpcode++, "DEF", function(machine) {
		machine.globals.push(machine.stack.pop());
   });
   addInstruction(curOpcode++, "DEF_TUP", function(machine) {
		machine.execute(getInstruction("FAB_TUP"));
		machine.globals.push(machine.stack.pop());
   });
   addInstruction(curOpcode++, "DEF_VEC", function(machine) {
		machine.execute(getInstruction("FAB_TUP"));
		machine.globals.push(machine.stack.pop());
   });

   var def_num_vec = function(machine, n) {
      var elements = getIndex(machine, n);
      var tuple = new Array();
      for(i = 0; i < elements; i++) {
         tuple[i] = 0; //XXX: why init to zero?
      }
      machine.globals.push(tuple);
   };
   addInstruction(curOpcode++, "DEF_NUM_VEC", def_num_vec, 1);
   addInstruction(curOpcode++, "DEF_NUM_VEC", def_num_vec, 2);
   addInstruction(curOpcode++, "DEF_NUM_VEC", def_num_vec, 3);
   addInstruction(curOpcode++, "DEF_NUM_VEC", def_num_vec);
}

var glo_ref = function(machine, n) {
   var index = getIndex(machine, n);
   machine.stack.push(machine.globals[index]);
};
addInstruction(curOpcode++, "GLO_REF", glo_ref, 0);
addInstruction(curOpcode++, "GLO_REF", glo_ref, 1);
addInstruction(curOpcode++, "GLO_REF", glo_ref, 2);
addInstruction(curOpcode++, "GLO_REF", glo_ref, 3);
addInstruction(curOpcode++, "GLO_REF", glo_ref);

if(MIT_COMPATIBILITY && !MIT_COMPATIBILITY.NO_MIT) {
   addInstruction(curOpcode++, "GLO_REF16", glo_ref);
}

var def_fun = function(machine, n) {
   var size = getIndex(machine, n);
   //console.log("Defining function with size: " + size);
   machine.globals.push(machine.currentAddress());
   machine.skip(size);
};
addInstruction(curOpcode++, "DEF_FUN", def_fun, 2);
addInstruction(curOpcode++, "DEF_FUN", def_fun, 3);
addInstruction(curOpcode++, "DEF_FUN", def_fun, 4);
addInstruction(curOpcode++, "DEF_FUN", def_fun, 5);
addInstruction(curOpcode++, "DEF_FUN", def_fun, 6);
addInstruction(curOpcode++, "DEF_FUN", def_fun, 7);
addInstruction(curOpcode++, "DEF_FUN", def_fun);

if(MIT_COMPATIBILITY && !MIT_COMPATIBILITY.NO_MIT) {
   addInstruction(curOpcode++, "DEF_FUN16", def_fun);
}

if(MIT_COMPATIBILITY && !MIT_COMPATIBILITY.MIT_ONLY) {
   addInstruction(curOpcode++, "LIT", function(machine) {
		machine.stack.push(machine.nextVLQInt());
   });
}
if(MIT_COMPATIBILITY && !MIT_COMPATIBILITY.NO_MIT) {
   addInstruction(curOpcode++, "LIT8", function(machine) {
		machine.stack.push(machine.nextInt8());
   });
   // Don't have the capability of reading int16s right now
   //addInstruction(curOpcode++, "LIT16", function(machine) {
   //		machine.stack.push(machine.nextInt16());
   //});
}

var lit = function(machine, n) {
   var index = getIndex(machine, n);
   machine.stack.push(index);
};
addInstruction(curOpcode++, "LIT", lit, 0);
addInstruction(curOpcode++, "LIT", lit, 1);
addInstruction(curOpcode++, "LIT", lit, 2);
addInstruction(curOpcode++, "LIT", lit, 3);
addInstruction(curOpcode++, "LIT", lit, 4);
addInstruction(curOpcode++, "LIT_FLO", function(machine) {
   var float_data = bytesToFloat(
      machine.nextInt8(),
      machine.nextInt8(),
      machine.nextInt8(),
      machine.nextInt8());
   machine.stack.push(float_data);
});
addInstruction(curOpcode++, "INF", function(machine) {
   machine.stack.push(INF);
});

if(MIT_COMPATIBILITY && !MIT_COMPATIBILITY.MIT_ONLY) {
   addInstruction(curOpcode++, "NEG_INF", function(machine) {
      machine.stack.push(-INF);
   });
}

addInstruction(curOpcode++, "EQ", function(machine) {
   machine.stack.push(compareMachine(machine) == 0 ? 1 : 0);
});
if(MIT_COMPATIBILITY && !MIT_COMPATIBILITY.MIT_ONLY) {
   addInstruction(curOpcode++, "NEQ", function(machine) {
		machine.stack.push(compareMachine(machine) != 0 ? 1 : 0);
   });
}

function ensureTuple(d) {
   var ret;
   if(isArray(d)) {
      ret = d;
   } else {
      ret = new Array();
      ret.push(d);
   }
   return ret;
}

function compareMachine(machine) {
   var b = machine.stack.pop();
   var a = machine.stack.pop();
   return compare(a, b);
}

function compare(a, b) {
   if(isNumber(a) && isNumber(b)) {
      return a == b ? 0 : a < b ? -1 : 1;
   } else {
      var aa = ensureTuple(a);
      var bb = ensureTuple(b);
      var size = aa.length > bb.length ? aa.length : bb.length;
      for(i = 0; i < size; i++) {
         var aElem = i < aa.length ? aa[i] : 0;
         var bElem = i < bb.length ? bb[i] : 0;
         if (aElem < bElem) return -1;
         if (aElem > bElem) return 1;
      }
      return 0;
   }
}

addInstruction(curOpcode++, "LT", function(machine) {
   machine.stack.push(compareMachine(machine) == -1 ? 1 : 0);
});
addInstruction(curOpcode++, "LTE", function(machine) {
   machine.stack.push(compareMachine(machine) != 1 ? 1 : 0);
});
addInstruction(curOpcode++, "GT", function(machine) {
   machine.stack.push(compareMachine(machine) == 1 ? 1 : 0);
});
addInstruction(curOpcode++, "GTE", function(machine) {
   machine.stack.push(compareMachine(machine) != -1 ? 1 : 0);
});
addInstruction(curOpcode++, "NOT", function(machine) {
   var a = machine.stack.pop();
   machine.stack.push(a ? 0 : 1);
});

addInstruction(curOpcode++, "ADD", function(machine) {
    var b = machine.stack.pop();
    var a = machine.stack.pop();
    if (isNumber(a) && isNumber(b)) {
	machine.stack.push(a + b);
    } else {
	var aa = ensureTuple(a);
	var bb = ensureTuple(b);
	var size = aa.length > bb.length ? aa.length : bb.length;
	var result = new Array();
	for(var i = 0; i < size; i++) {
	    var a_element = i < aa.length ? aa[i] : 0;
	    var b_element = i < bb.length ? bb[i] : 0;
	    result.push(a_element + b_element);
	}
	machine.stack.push(result);
    }
});
addInstruction(curOpcode++, "SUB", function(machine) {
    var b = machine.stack.pop();
    var a = machine.stack.pop();
    if (isNumber(a) && isNumber(b)) {
	machine.stack.push(a - b);
    } else {
	var aa = ensureTuple(a);
	var bb = ensureTuple(b);
	var size = aa.length > bb.length ? aa.length : bb.length;
	var result = new Array();
	for(var i = 0; i < size; i++){
	    var a_element = i < aa.length ? aa[i] : 0;
	    var b_element = i < bb.length ? bb[i] : 0;
	    result.push(a_element - b_element);
	}
	machine.stack.push(result);
    }
});
addInstruction(curOpcode++, "MUL", function(machine) {
		var b = machine.stack.pop();
		var a = machine.stack.pop();
		if (isNumber(a) && isNumber(b)) {
			machine.stack.push(a * b);
		} else {
			var factor = isNumber(a) ? a : b;
			var vector = isNumber(a) ? b : a;
			var result = new Array();
			for(var i = 0; i < vector.length; i++)
            result.push(vector[i] * factor);
			machine.stack.push(result);
		}
});
addInstruction(curOpcode++, "DIV", function(machine) {
		var b = machine.stack.pop();
		var a = machine.stack.pop();
		if (isNumber(a) && isNumber(b)) {
			machine.stack.push(a / b);
		} else {
			var divisor = isNumber(a) ? a : b;
			var vector = isNumber(a) ? b : a;
			var result = new Array();
			for(var i = 0; i < vector.length; i++)
            result.push(vector[i] / divisor);
			machine.stack.push(result);
		}
});

addInstruction(curOpcode++, "ABS", function(machine) {
   var a = machine.stack.pop();
   if(isNumber(a)) {
      machine.stack.push(Math.abs(a));
   } else {
      var s = 0;
      for(var i=0; i<a.length; i++) {
         s += a[i] * a[i];
      }
      machine.stack.push(Math.sqrt(s));
   }
});
addInstruction(curOpcode++, "MAX", function(machine) {
   var b = machine.stack.pop();
   var a = machine.stack.pop();
   machine.stack.push(compare(a,b) > 0 ? a : b);
});
addInstruction(curOpcode++, "MIN", function(machine) {
   var b = machine.stack.pop();
   var a = machine.stack.pop();
   machine.stack.push(compare(a,b) < 0 ? a : b);
});
addInstruction(curOpcode++, "POW", function(machine) {
   var b = machine.stack.pop();
   var a = machine.stack.pop();
   machine.stack.push(Math.pow(a,b));
});
addInstruction(curOpcode++, "REM", function(machine) {
   var b = machine.stack.pop();
   var a = machine.stack.pop();
   machine.stack.push(a % b); //XXX: this isn't quite right for neg numbers
});
addInstruction(curOpcode++, "MOD", function(machine) {
   var b = machine.stack.pop();
   var a = machine.stack.pop();
   machine.stack.push(a % b);
});
addInstruction(curOpcode++, "FLOOR", function(machine) {
   machine.stack.push(
      Math.floor(
         machine.stack.pop()));
});
addInstruction(curOpcode++, "CEIL", function(machine) {
   machine.stack.push(
      Math.ceil(
         machine.stack.pop()));
});
addInstruction(curOpcode++, "ROUND", function(machine) {
   machine.stack.push(
      Math.round(
         machine.stack.pop()));
});
addInstruction(curOpcode++, "LOG", function(machine) {
   machine.stack.push(
      Math.log(
         machine.stack.pop()));
});
addInstruction(curOpcode++, "SQRT", function(machine) {
   machine.stack.push(
      Math.sqrt(
         machine.stack.pop()));
});
addInstruction(curOpcode++, "SIN", function(machine) {
   machine.stack.push(
      Math.sin(
         machine.stack.pop()));
});
addInstruction(curOpcode++, "COS", function(machine) {
   machine.stack.push(
      Math.cos(
         machine.stack.pop()));
});
addInstruction(curOpcode++, "TAN", function(machine) {
   machine.stack.push(
      Math.tan(
         machine.stack.pop()));
});

function sinh(aValue)
{
   var myTerm1 = Math.pow(Math.E, aValue);
   var myTerm2 = Math.pow(Math.E, -aValue);

   return (myTerm1-myTerm2)/2;
}
addInstruction(curOpcode++, "SINH", function(machine) {
   machine.stack.push(
      sinh(
         machine.stack.pop()));
});

function cosh(aValue)
{
   var myTerm1 = Math.pow(Math.E, aValue);
   var myTerm2 = Math.pow(Math.E, -aValue);

   return (myTerm1+myTerm2)/2;
}
addInstruction(curOpcode++, "COSH", function(machine) {
   machine.stack.push(
      cosh(
         machine.stack.pop()));
});

function tanh (arg) {
   return (Math.exp(arg) - Math.exp(-arg)) / (Math.exp(arg) + Math.exp(-arg));
}
addInstruction(curOpcode++, "TANH", function(machine) {
   machine.stack.push(
      tanh(
         machine.stack.pop()));
});
addInstruction(curOpcode++, "ASIN", function(machine) {
   machine.stack.push(
      Math.asin(
         machine.stack.pop()));
});
addInstruction(curOpcode++, "ACOS", function(machine) {
   machine.stack.push(
      Math.acos(
         machine.stack.pop()));
});
addInstruction(curOpcode++, "ATAN2", function(machine) {
   machine.stack.push(
      Math.atan2(
         machine.stack.pop()));
});
addInstruction(curOpcode++, "RND", function(machine) {
   var max = machine.stack.pop();
   var min = machine.stack.pop();
   machine.stack.push(random(min, max));
});

addInstruction(curOpcode++, "ELT", function(machine) {
   var element = machine.stack.pop();
   var tuple = machine.stack.pop();
   machine.stack.push(tuple[element]);
});
addInstruction(curOpcode++, "NUL_TUP", function(machine) {
   machine.stack.push(new Array());
});
if(MIT_COMPATIBILITY && !MIT_COMPATIBILITY.NO_MIT) {
   addInstruction(curOpcode++, "TUP", function(machine) {
      var unused = machine.nextInt8();
      var elements = machine.nextInt8();
      var tuple = new Array();
      for(var i = 0; i < elements; i++) {
         tuple.push(machine.stack.peek(elements-i-1));
      }
      machine.stack.popN(elements);
      machine.stack.push(tuple);
   });
}

addInstruction(curOpcode++, "FAB_TUP", function(machine) {
   var elements = machine.nextInt8();
   var tuple = new Array();
   for(var i = 0; i < elements; i++) {
      tuple.push(machine.stack.pop());
   }
   machine.stack.push(tuple);
});
addInstruction(curOpcode++, "FAB_VEC", function(machine) {
   var elements = machine.nextInt8();
   var element = machine.stack.pop();
   var tuple = new Array();
   for(var i = 0; i < elements; i++) {
      tuple.push(element);
   }
   machine.stack.push(tuple);
});
addInstruction(curOpcode++, "FAB_NUM_VEC", function(machine) {
   var elements = machine.nextInt8();
   var tuple = new Array();
   for(var i = 0; i < elements; i++) {
      tuple.push(0); // fill with all zero's
   }
   machine.stack.push(tuple);
});
addInstruction(curOpcode++, "LEN", function(machine) {
   var tuple = machine.stack.pop();
   machine.stack.push(tuple.length);
});

if(MIT_COMPATIBILITY && !MIT_COMPATIBILITY.NO_MIT) {
   addInstruction(curOpcode++, "VADD", function(machine) {
                machine.nextInt8(); //unused: tuple index
		var b = machine.stack.pop(); //tuple
		var a = machine.stack.pop(); //tuple
		var min_size = a.length < b.length ? a.length : b.length;
		var max_size = a.length < b.length ? b.length : a.length;
		var largest = a.length < b.length ? b : a;
		var result = new Array();
		for(var i = 0       ; i < min_size; i++) result.push(a[i] + b[i]);
		for(var i = min_size; i < max_size; i++) result.push(largest[i]);
		machine.stack.push(result);
   });
   addInstruction(curOpcode++, "VSUB", function(machine) {
                machine.nextInt8(); //unused: tuple index
		var b = machine.stack.pop();
		var a = machine.stack.pop();
		var min_size = a.length < b.length ? a.length : b.length;
		var max_size = a.length < b.length ? b.length : a.length;
		var largest = a.length < b.length ? b : a;
		var padding_sign = a.length < b.length ? -1 : 1;
		var result = new Array();
		for(var i = 0       ; i < min_size; i++) result.push(a[i] - b[i]);
		for(var i = min_size; i < max_size; i++) result.push(padding_sign * largest[i]);
		machine.stack.push(result);
   });
   addInstruction(curOpcode++, "VDOT", function(machine) {
		var result = 0;
		var b = machine.stack.pop();
		var a = machine.stack.pop();
		var min_size = a.length < b.length ? a.length : b.length;
		for(var i = 0; i < min_size; i++) result += a[i] * b[i];
		machine.stack.push(result);
   });
   addInstruction(curOpcode++, "VMUL", function(machine) {
		machine.nextInt8(); //unused?
		var b = machine.stack.pop(); //tuple
		var a = machine.stack.pop(); //number
		var result = new Array();
		for(var i = 0; i < b.length; i++) result.push(a * b[i]);
		machine.stack.push(result);
   });
   addInstruction(curOpcode++, "VSLICE", function(machine) {
		machine.nextInt8(); //unused?
		var source = machine.stack.pop(); //tuple
		var start = machine.stack.pop(); //number
		var stop  = machine.stack.pop(); //number
		start = start >= 0 ? start : source.length + start;
		stop  = stop  >= 0 ? stop  : source.length + stop ;
		var result = new Array();
		for(var i = start; i < stop; i++) result.push(source[i]);
		machine.stack.push(result);
   });
   addInstruction(curOpcode++, "VEQ", function(machine) {
		machine.stack.push(compareMachine(machine) == 0 ? 1 : 0);
   });
   addInstruction(curOpcode++, "VLT", function(machine) {
      machine.stack.push(compareMachine(machine) == -1 ? 1 : 0);
   });
   addInstruction(curOpcode++, "VLTE", function(machine) {
      machine.stack.push(compareMachine(machine) != 1 ? 1 : 0);
   });
   addInstruction(curOpcode++, "VGT", function(machine) {
      machine.stack.push(compareMachine(machine) == 1 ? 1 : 0);
   });
   addInstruction(curOpcode++, "VGTE", function(machine) {
      machine.stack.push(compareMachine(machine) != -1 ? 1 : 0);
   });
   addInstruction(curOpcode++, "VMIN", function(machine) {
      var b = machine.stack.pop();
      var a = machine.stack.pop();
      machine.stack.push(compare(a,b) < 0 ? a : b);
   });
   addInstruction(curOpcode++, "VMAX", function(machine) {
      var b = machine.stack.pop();
      var a = machine.stack.pop();
      machine.stack.push(compare(a,b) > 0 ? a : b);
   });
}

function apply_end(machine) {
   var result = machine.stack.pop();
   var args = machine.stack.pop(); //tuple
   machine.stack.pop(); //address
   machine.stack.push(result);
   machine.environment.popN(args.length);
}
addInstruction(curOpcode++, "APPLY", function(machine) {
   var args = machine.stack.peek(0); // tuple
   var func = machine.stack.peek(1); // address
   for(var i=0; i<args.length; i++) {
      machine.environment.push(args[i]);
   }
   machine.call(func, apply_end);
});

function map_step(machine) {
   machine.environment.pop();
   var new_element = machine.stack.pop();
   var result = machine.stack.peek(0); //tuple
   var values = machine.stack.peek(1); //tuple
   result.push(new_element);
   if(result.length < values.lengh) {
      machine.environment.push(values[result.length]);
      var filter = machine.stack.peek(2); //address
      machine.call(filter, map_step);
   } else {
      var res = machine.stack.pop(); //tuple
      machine.stack.pop(); //tuple
      machine.stack.pop(); //address
      machine.stack.push(res);
   }
}

function map(machine) {
   var tuple = machine.stack.peek(0);
   if(tuple.length < 1) {
      machine.stack.pop(); //discard the tuple
      machine.stack.pop(); //discard the address for the filter
      machine.stack.push(new Array()); //push an empty tuple
   } else {
      var filterAddr = machine.stack.peek(1);
      machine.stack.push(new Array());
      machine.environment.push(tuple); // or just the first element?
      machine.call(filterAddr, map_step);
   }
}
if(MIT_COMPATIBILITY && !MIT_COMPATIBILITY.MIT_ONLY) {
   addInstruction(curOpcode++, "TUP_MAP", map);
}

if(MIT_COMPATIBILITY && !MIT_COMPATIBILITY.NO_MIT) {
   addInstruction(curOpcode++, "MAP", map);
}

function fold_step(machine) {
   machine.environment.popN(2);
   var result = machine.stack.pop(); //data
   var fold_index = machine.stack.peek(0); //number
   var fold_values = machine.stack.peek(1); //tuple
   var fold_fuse = machine.stack.peek(2); //address
   if(++fold_index < fold_values.length) {
      machine.environment.push(result);
      machine.environment.push(fold_values[fold_index]);
      machine.call(fold_fuse, fold_step);
   } else {
      machine.stack.pop(); //number
      machine.stack.pop(); //tuple
      machine.stack.pop(); //address
      machine.stack.push(result);
   }
}

function fold(machine) {
   var fold_values = machine.stack.pop(); //tuple
   var result = machine.stack.pop();
   var fold_fuse = machine.stack.peek(); //address
   if(fold_values.length < 1) {
      machine.stack.pop(); // unneeded address
      machine.stack.push(result);
   } else {
      machine.stack.push(fold_values);
      machine.stack.push(0);
      machine.environment.push(result);
      machine.environment.push(fold_values); // or just the first element?
      machine.call(fold_fuse, fold_step);
   }
}
addInstruction(curOpcode++, "FOLD", fold);

if(MIT_COMPATIBILITY && !MIT_COMPATIBILITY.NO_MIT) {
   addInstruction(curOpcode++, "VFOLD", fold);
}

addInstruction(curOpcode++, "INIT_FEEDBACK", function(machine) {
   var state_index = machine.nextVLQInt();
   var initialization_function = machine.stack.pop();
   if(machine.state[state_index]) {
      machine.state[state_index].is_executed = true;
      machine.stack.push(machine.state[state_index].value);
   } else {
      machine.stack.push(state_index);

      // Anonymous Instruction INIT_FEEDBACK_SET_STATE
      var init_feedback_set_state = new Instruction();
      init_feedback_set_state.opcode = 9999999; //fake
      init_feedback_set_state.name = "INIT_FEEDBACK_SET_STATE";
      init_feedback_set_state.toExec = function(machine) {
         var fb_state = machine.stack.pop();
         var fb_state_index = machine.stack.pop();
          machine.state[fb_state_index] = new State(fb_state);
          machine.stack.push(fb_state);
      };
      machine.call(initialization_function, init_feedback_set_state);
   }
});
if(MIT_COMPATIBILITY && !MIT_COMPATIBILITY.MIT_ONLY) {
   addInstruction(curOpcode++, "SET_FEEDBACK", function(machine) {
		var state_index = machine.nextVLQInt();
		machine.state[state_index] = machine.stack.peek();
		machine.state[state_index].is_executed = true;
   });
}

addInstruction(curOpcode++, "FEEDBACK", function(machine) {
   var state_index = machine.nextVLQInt();
   var value = machine.stack.pop();
   machine.state[state_index] = new State(value);
   machine.stack.popN(1); //XXX why the 1?
   machine.stack.push(value);
});

addInstruction(curOpcode++, "MID", function(machine) {
   machine.stack.push(machine.id);
});


function fold_hood_plus_step_fuse(machine) {
   machine.environment.popN(2);
   fold_hood_filter_next(machine);
}

function fold_hood_plus_step_filter(machine) {
   machine.environment.popN(1);
   var imprt = machine.stack.pop(); //data
   machine.environment.push(machine.stack.pop());
   machine.environment.push(imprt);
   var fuse = machine.stack.peek(1); //address
   machine.call(fuse, fold_hood_plus_step_fuse);
}

function fold_hood_filter_next(machine) {
   while(machine.getNextNeighbor() &&
      isUndefined(machine.current_neighbor.imports[machine.current_import]));
   if(machine.current_neighbor) {
      machine.environment.push( machine.current_neighbor.imports[machine.current_import]);
      var filter = machine.stack.peek(1); //address
      machine.call(filter, fold_hood_plus_step_filter);
   } else {
      var result = machine.stack.pop();
      machine.stack.popN(2);
      machine.stack.push(result);
   }
}

function fold_hood_plus_first_filter(machine) {
   machine.environment.popN(1);
   fold_hood_filter_next(machine);
}

function fold_hood_plus(machine) {
   var import_index = machine.nextVLQInt();
   var export_value = machine.stack.pop(); //data
   var filter = machine.stack.peek(); //address

   machine.current_import = import_index;
   machine.imports[import_index] = export_value; // update own value
   machine.current_neighbor = machine.hood; //start at head of nbrlist (self)

   // walk hood to find first valid neighbor
   // can safely assume there will be at least one neighbor: self
   while(machine.current_neighbor && 
      isUndefined(machine.current_neighbor.imports[machine.current_import]))
      machine.getNextNeighbor();
   // push value onto stack and call filter
   machine.environment.push(machine.current_neighbor.imports[machine.current_import]);
   machine.call(filter, fold_hood_plus_first_filter);
}

function fold_hood_step(machine) {
   machine.environment.popN(2);
   while(machine.getNextNeighbor() &&
	 isUndefined(machine.current_neighbor.imports[machine.current_import]));
   if(machine.current_neighbor) {
      machine.environment.push(machine.stack.pop());
      machine.environment.push(machine.current_neighbor.imports[machine.current_import])
      var fuse = machine.stack.peek(); //address
      machine.call(fuse, fold_hood_step);
   } else {
      var result = machine.stack.pop(); //data
      machine.stack.popN(1);
      machine.stack.push(result);
   }
}

function fold_hood(machine) {
   var import_index = machine.nextVLQInt();
   var export_value = machine.stack.pop(); //data
   var result = machine.stack.pop(); //data
   var fuse = machine.stack.peek(); //address

   machine.current_import = import_index;
   machine.imports[import_index] = export_value;
   machine.current_neighbor = machine.hood; // start at head of nbrlist

   // walk hood to find first valid neighbor
   // can safely assume there will be at least one neighbor: self
   while(machine.current_neighbor && 
      isUndefined(machine.current_neighbor.imports[machine.current_import]))
      machine.getNextNeighbor();
   // push value onto stack and call filter
   machine.environment.push(result);
   machine.environment.push(machine.current_neighbor.imports[machine.current_import]);
   machine.call(fuse, fold_hood_step);
}

addInstruction(curOpcode++, "FOLD_HOOD", fold_hood);

if(MIT_COMPATIBILITY && !MIT_COMPATIBILITY.NO_MIT) {
   addInstruction(curOpcode++, "VFOLD_HOOD", function(machine) {
      machine.nextInt8();
      fold_hood(machine);
   });
}

addInstruction(curOpcode++, "FOLD_HOOD_PLUS", fold_hood_plus);

if(MIT_COMPATIBILITY && !MIT_COMPATIBILITY.NO_MIT) {
   addInstruction(curOpcode++, "VFOLD_HOOD_PLUS", function(machine) {
      machine.nextInt8();
      fold_hood_plus(machine);
   });
}

// FUNCALL?
