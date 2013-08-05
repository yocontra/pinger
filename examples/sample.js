var ping = require('../');

ping("google.com", function(err, ms) {
  console.log(ms);
});