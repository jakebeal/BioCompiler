
var script = [
   "DEF_VM_OP", 
   0, 
   0, 
   0, 
   1, 
   0, 
   0, 
   1, 
   1, 
   "DEF_FUN_2_OP", 
   "LIT_3_OP", 
   "RET_OP", 
   "EXIT_OP"
];

function stepWhileNotFinished(machine) {
   var numSteps = 0;
   while(!machine.finished() && numSteps < 300) {
      console.log("Not finished, starting a step...");
      machine.step();
      numSteps++;
   }
   machine.deliverMessage(machine);
}

var machine = new Machine();

console.log("Installing...");
machine.install(script);
stepWhileNotFinished(machine);
console.log("Installed.");

console.log("Running...");
machine.run(0);
stepWhileNotFinished(machine);
console.log("Done.");

