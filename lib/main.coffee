pinger = require '../pinger'

module.exports = 
  ping: (host, cb) ->
    return pinger.ping host unless cb?
    pinger.ping host, cb


# TODO: pure js version
###
module.exports =
  ICMP_ECHO_REQUEST: 8 # From /usr/include/linux/icmp.h
  ping: (host, cb) ->
    sock = new net.Socket type: 'unix'
###