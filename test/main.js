var ping = require('../');
var should = require('should');
require('mocha');

describe('pinger', function() {
  it('should ping localhost', function(done) {
    ping('localhost', function(err, ms) {
      should.not.exist(err);
      should.exist(ms);
      (ms < 10).should.equal(true);
      done();
    });
  });

  it('should ping google', function(done) {
    ping('google.com', function(err, ms) {
      should.not.exist(err);
      should.exist(ms);
      (ms < 100).should.equal(true);
      done();
    });
  });

});
