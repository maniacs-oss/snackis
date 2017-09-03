#!/usr/local/bin/snabel

"tcps.sl"

"Starts a TCP/IP server on ADDR:PORT and forwards incoming data to STDOUT and all"
"connected clients except the sender.                                            "

"Usage:           "
"tcps.sl ADDR PORT"

let: port stoi64; _
let: addr; _
let: queues TCPStream List<Bin> table;
let: senders TCPStream Proc table;
let: out Bin list;

let: server tcp-socket
     @addr @port bind
     1 accept;

let: do-recv {(
  let: client; _
  yield

  @client read {
    {let: data; _
     @queues {$ left @client = {
       unzip @data push _
       @senders $1 get {call} when 
     } unless _} for
     @out @data push _} when
    yield1
  } for

  @client close
  @queues @client del _
  'disconnect' say
)};

func: do-send {(
  let: q; _
  let: c; _
  yield

  @q @c write &yield for
)};

let: do-server {(
  yield

  @server {
    {'connect' say
     Bin list
     let: q;
     let: s fifo do-send proc; _
     $ @queues $1 @q put _
     $ @senders $1 @s put _
     @do-recv call proc @procs $1 push _ _} when

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