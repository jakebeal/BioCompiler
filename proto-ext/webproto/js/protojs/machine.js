
nextMid = 0;
function getNextMid() {
   return nextMid++;
}

function State(value) {
    this.is_executed = true;
    this.value = value;
}

function Machine() {
   this.id = getNextMid();
   this.stack = new Array();
   this.environment = new Array();
   this.globals = new Array();
   this.state = new Array(); // array of State
   this.script = null; //array of integers
   this.instruction_pointer = 0;
   this.start_time = 0;
   this.last_time = 0;
   this.callbacks = new Array(); //Not sure what this should be...
   this.desired_period = 1;
   this.hood = null; // Neighbors forming a double-linked list
   this.imports = new Array();
   this.current_neighbor = null; // Neighbor for traversal
   this.current_import = null; // import from current neighbor
   this.result = null;
   this.crashed = false;
   this.sensors = {
      testSensor : [false, false, false]
   };
   this.x = 0.0;
   this.y = 0.0;
   this.z = 0.0;
   this.dx = 0.0;
   this.dy = 0.0;
   this.dz = 0.0;
   this.red = 0;
   this.green = 0;
   this.blue = 0;
   this.radius = 0.0;
}

Machine.prototype.jump = function(address) {
   this.instruction_pointer = address;
}

Machine.prototype.skip = function(distance) {
   this.instruction_pointer += parseInt(distance);
}

Machine.prototype.currentScript = function() {
   return this.script;
}

Machine.prototype.currentAddress = function() {
   return this.instruction_pointer;
}

Machine.prototype.startTime = function() {
   return this.start_time;
}

Machine.prototype.depth = function() {
   return this.callbacks.length;
}

Machine.prototype.nextInt8 = function() {
    return this.script[this.instruction_pointer++];;
}

Machine.prototype.nextVLQInt = function() {
    var value = 0;
    // read bytes using variable-length quantity (VLQ)
    // format  as used in the MIDI file format.
    // http://en.wikipedia.org/wiki/Variable-length_quantity
    more_bytes = true;
    while(more_bytes) {
	var next = this.script[this.instruction_pointer++];
	if (next > 128) {
	    value = (value+(next-128))*128; 
	} else {
	    value = value+next;
	    more_bytes = false;
	}
    }
    return value;
}

Machine.prototype.install = function(rawScript) {
   this.crashed = false;
   this.script = new Array();
   for(var i=0; i<rawScript.length; i++) {
      if(!isNumber(rawScript[i])) {
         var rawNum = parseInt(rawScript[i]);
         if(!isNaN(rawNum)) {
            this.script[i] = rawNum;
         } else {
            var instruction = getInstruction(rawScript[i]);
            if(instruction) {
               this.script[i] = instruction.opcode;
            } else {
               console.warn("Couldn't find instruction for: " + rawScript[i]);
               return false;
            }
         }
      } else {
         this.script[i] = parseInt(rawScript[i]);
      }
   }
   this.jump(0);
   this.callbacks.push(null);
}

run_callback = function(machine) {
   machine.result = machine.stack.pop();
   machine.last_time = machine.start_time;
}

Machine.prototype.run = function(startTime) {
   this.start_time = startTime;
   this.jump(this.globals.peek());
   this.callbacks.push(run_callback);
}

Machine.prototype.step = function() {
   var opcode = this.nextInt8();
   if(opcode != null) {
      var instruction = instructions[opcode];
      if(instruction) {
         //console.log("Executing instruction: " + instruction.name);
         this.execute(instruction);
      } else {
         console.warn("Unidentified opcode: " + opcode);
	 this.crashed = true;
      }
   } else {
      console.warn("Could not get opcode, instruction pointer: " + this.instruction_pointer + " w/ script length: " + this.script.length);
      this.crashed = true;
   }
}

Machine.prototype.finished = function() {
   return this.crashed || this.callbacks.length <= 0;
}

Machine.prototype.execute = function(instruction) {
   if(instruction && instruction.exec) {
      instruction.exec(this);
   } else {
      console.log("Could not execute null instruction: " + instruction);
   }
}

Machine.prototype.call = function(address, callback) {
   if(!callback) callback = 0;
   this.callbacks.push(callback);
   this.stack.push(this.instruction_pointer);
   this.jump(address);
}

Machine.prototype.retn = function() {
   var callback = this.callbacks.pop();
   var result = this.stack.pop();
   //console.log("Result: " + result);
   if(this.callbacks.length > 0)
      this.jump(this.stack.pop());
   if(this.callbacks.length > 0 || callback)
      this.stack.push(result);
   if(callback && isFunction(callback))
      callback(this);
}

var DATA_STALE_AGE = 1;

Machine.prototype.executeRound = function(time) {
   // Remove any neighbors that we haven't heard from in a while...
   var neighbor = this.hood;
   while(neighbor) {
      if(neighbor.data_age > DATA_STALE_AGE) {
         neighbor = this.deleteNbr(neighbor);
      } else {
         neighbor.data_age++;
         neighbor = neighbor.nextNbr;
      }
   }
   // create a fresh imports and dummy neighbor for self
   var selfNbr = new Neighbor(); selfNbr.id = this.id;
   this.imports = selfNbr.imports;
   //selfNbr.imports = this.imports;
   this.pushNbr(selfNbr);

    // mark all states as not-yet-executed
   for (var i in this.state) {
     this.state[i].is_executed = false;
   }

   // Now execute...
   this.run(time);
   while(!this.finished()) {
      this.step();
   }

   // remove dummy nbr
   this.deleteNbr(selfNbr);
   // remove any states that didn't execute
   for(var i in this.state) {
     if(!this.state[i].is_executed) delete this.state[i];
   }
}

Machine.prototype.deleteNbr = function(nbr) {
   next = nbr.nextNbr;
   if(nbr.nextNbr) { nbr.nextNbr.prevNbr = nbr.prevNbr; }
   if(nbr.prevNbr) { nbr.prevNbr.nextNbr = nbr.nextNbr; }
   else { this.hood = nbr.nextNbr; }
   delete nbr;
   return next;
}
Machine.prototype.deleteNbrWithId = function(id) {
   var neighbor = this.hood;
   while(neighbor) {
      if(neighbor.id == id) {
         return this.deleteNbr(neighbor);
      }
      else neighbor = neighbor.nextNbr;
   }
}
Machine.prototype.pushNbr = function(nbr) {
   // Delete any old version, if it exists
   this.deleteNbrWithId(nbr.id);
   // add the new to the front of the linked list
   nbr.nextNbr = this.hood; 
   if(this.hood)
      this.hood.prevNbr = nbr;
   this.hood = nbr;
}

// Put my imports in the other machine's import array,
// plus space-time measures
Machine.prototype.deliverMessage = function(otherMachine) {
   // create new neighbor record for import
   nbr = new Neighbor();
   nbr.id = this.id;
   nbr.imports = this.imports; // doesn't clone, but this.imports isn't reused
   nbr.x = this.x - otherMachine.x;
   nbr.y = this.y - otherMachine.y;
   nbr.z = this.z - otherMachine.z;
   // push into beginning of neighbor list
   otherMachine.pushNbr(nbr);
}

Machine.prototype.getNextNeighbor = function() {
   this.current_neighbor = this.current_neighbor.nextNbr;
   return this.current_neighbor;
}

Machine.prototype.getNeighborhoodSize = function() {
   var neighbor = this.hood;
   var ret = 0;
   while(neighbor) {
      ret++;
      neighbor = neighbor.nextNbr;
   }
   return ret;
}

Machine.prototype.getSensor = function(testSensorNum) {
   return this.sensors.testSensor[testSensorNum];
}

Machine.prototype.setSensor = function(testSensorNum, enabled) {
   this.sensors.testSensor[testSensorNum] = enabled;
}

