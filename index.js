var dns = require('dns');
var ping = require('net-ping');

module.exports = function(host, cb) {
  dns.lookup(host, function(err, ip, fam){
    if (err) return cb(err);

    var session = ping.createSession();
    session.pingHost(ip, function(err, target, sent, rcvd){
      if (err) return cb(err);
      var ms = rcvd - sent;
      cb(null, ms);
      session.close();
    });
  });
};