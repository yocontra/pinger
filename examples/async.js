var pinger = require('../pinger');

var host = 'google.com';

pinger.ping(host,function(isUp,roundTripTime) {
    console.log(host,isUp,roundTripTime);
});
