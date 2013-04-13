var pinger = require('../pinger');

var host = 'google.com';

var isUp = pinger.ping(host);

console.log(host,isUp);
