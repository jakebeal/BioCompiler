
function State() {
   this.data = null;
   this.is_executed = false;
   this.is_set = false;
}

State.prototype.setData = function(data) {
   this.data = data;
   this.is_set = true;
}

State.prototype.isSet = function() {
   return this.is_set;
}
