
//Array of Instruction indexed by opcode
var instructions = new Array();

//Map of InstructionName -> Instruction
var instructionMap = {};

function Instruction() {
   this.opcode = -1;
   this.name = null; //String
   this.n = null;

   //function(Machine, [optional] n)
   this.toExec = null;
}

Instruction.prototype.exec = function(machine) {
   if(typeof this.n != "undefined") {
      this.toExec(machine, this.n);
   } else {
      this.toExec(machine);
   }
}

function addInstruction(opcode, name, toExec, n) {
   // Create a new instruction
   var instruction = new Instruction();
   instruction.opcode = opcode;
   instruction.name = name + "_OP";
   instruction.toExec = toExec;
   if(typeof n != "undefined") {
      instruction.n = n;
      instruction.name = name + "_" + n + "_OP";
   } // otherwise, default to non-varArg

   // Add it to the array/map
   instructions[instruction.opcode] = instruction;
   instructionMap[instruction.name] = instruction;
}

function getInstruction(nameOrOpcode) {
   if(isNumber(nameOrOpcode)) {
      // Lookup by opcode
      return instructions[nameOrOpcode];
   } else {
      // Lookup by name
      var i = instructionMap[nameOrOpcode];
      if(!i) {
         // Try appending _OP
         i = instructionMap[nameOrOpcode+"_OP"];
      }
      return i;
   }
}
