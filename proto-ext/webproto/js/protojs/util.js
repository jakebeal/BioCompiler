
Array.prototype.peek = function(index) {
   if(!index) index = 0;
   return this[this.length - index - 1];
}

Array.prototype.popN = function(numToPop) {
   if(!numToPop) numToPop = 0;
   var i = 0;
   while(i < numToPop) {
      this.pop();
      i++;
   }
}

function isNumber(n) {
   return typeof n == 'number';
   //return !isNaN(parseFloat(n)) && isFinite(parseFloat(n));
}

function isArray(n) {
   return n instanceof Array;
}

function isUndefined(n) {
   return typeof(n) == 'undefined';
}

function isFunction(functionToCheck) {
    var getType = {};
     return functionToCheck && getType.toString.call(functionToCheck) === '[object Function]';
}

function random(min, max) {
   return Math.floor(Math.random() * (max - min + 1)) + min;
}

var INF = Number.POSITIVE_INFINITY;
// use -INF for negative infinity

function bytesToFloat(b1, b2, b3, b4) {
   return new BinaryParser(1 /*big endian?*/, true)["toFloat"](
      String.fromCharCode(b4) +
      String.fromCharCode(b3) +
      String.fromCharCode(b2) +
      String.fromCharCode(b1));
}
