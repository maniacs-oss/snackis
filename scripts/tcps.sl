#!/usr/local/bin/snabel

"tcps.sl"

"Starts a TCP/IP server on ADDR:PORT and forwards incoming data to STDOUT and all"
"connected clients except the sender.                                            "

"Usage:           "
"tcps.sl ADDR PORT"

let: port stoi64; _
let: addr; _
let: queues TCPStream List<Bin> table;
let: out Bin list;

let: server tcp-socket
     @addr @port bind
     1 accept;

let: do-recv {(
  let: client; _
  yield

  @client read {
    {let: data; _
     @queues {$ left @client = {right @data push} unless _} for
     @out @data push _} when
    yield1
  } for

  @client close
  @queues @client del _
  'disconnect' say
)};

let: do-send {(
  let: client; _
  yield

  {@queues @client get
   {fifo @client write &yield2 for yield2} &break if} loop
)};

let: do-server {(
  yield

  @server {
    {'connect' say
     $ @queues $1 Bin list put _
     @do-recv call proc @procs $1 push _
     @do-send call proc @procs $1 push _ _} when

    idle
    yield1
  } for
)} call proc;

let: refresh {(
  yield
  {@out fifo stdout write &yield1 _for yield1} loop
)} call proc;

let: procs [@do-server @refresh];
@procs run &nop _for