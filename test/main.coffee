pinger = require '../'
should = require 'should'
benji = require 'benji'
require 'mocha'

describe 'pinger', ->
  describe 'async ping()', ->
    it 'should return true for google.com', (done) ->
      pinger.ping 'google.com', (up) ->
        should.exist up
        up.should.equal true
        done()

    it 'should handle many pings to google.com', (done) ->
      count = 5
      donzo = (up) ->
        should.exist up
        up.should.equal true
        done() if --count is 0
      pinger.ping 'google.com', donzo
      pinger.ping 'google.com', donzo
      pinger.ping 'google.com', donzo
      pinger.ping 'google.com', donzo
      pinger.ping 'google.com', donzo

    it 'should return false for pingusdingus1337.com', (done) ->
      pinger.ping 'pingusdingus1337.com', (up) ->
        should.exist up
        up.should.equal false
        done()

  describe 'sync ping()', ->
    it 'should return true for google.com', (done) ->
      up = pinger.ping 'google.com'
      should.exist up
      up.should.equal true
      done()

    it 'should handle many pings to google.com', (done) ->
      count = 5
      donzo = (up) ->
        should.exist up
        up.should.equal true
        done() if --count is 0
      donzo pinger.ping 'google.com'
      donzo pinger.ping 'google.com'
      donzo pinger.ping 'google.com'
      donzo pinger.ping 'google.com'
      donzo pinger.ping 'google.com'

    it 'should return false for pingusdingus1337.com', (done) ->
      up = pinger.ping 'pingusdingus1337.com'
      should.exist up
      up.should.equal false
      done()

  describe 'benchmark', ->
    it 'should spin mad bau5 d0g', (done) ->
      @timeout 900000000
      ping = require 'ping'
      ours = -> pinger.ping 'google.com'
      oursasync = (done) -> pinger.ping 'google.com', done
      theirs = (done) -> ping.sys.probe 'google.com', done

      benji.async 50, oursasync, (ms) ->
        console.log "async pinger: #{ms}"
        ms = benji.sync 50, ours
        console.log "sync pinger: #{ms}"
        benji.async 50, theirs, (ms) ->
          console.log "node-ping: #{ms}"
          done()