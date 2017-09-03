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

func: do-recv {(
  yield
  let: client; _

  @client read {{
    let: data; _

    @queues {$ left @client = {
      unzip @data push _
      @senders $1 get {call} when
    } unless _} for

    @out @data push _
    do-out} when

    yield1
  } for

  @queues @client del _
  @senders @client del _
  @client close
  'disconnect' say
)};

func: do-send {(
  yield
  let: q; _
  let: c; _

  @q fifo @c write {@q z? &yield1 when} _for
)};

func: do-server {(
  yield

  @server {
    {'connect' say
     Bin list
     let: q;
     let: s do-send proc; _
     $ @queues $1 @q put _
     $ @senders $1 @s put _
     do-recv proc @procs $1 push _ _} when

    idle
    yield1
  } for
)} call proc;

func: do-out {(
  yield
  @out fifo stdout write {@out z? &yield1 when} _for
)} call proc;

let: procs [&do-server];
@procs run &nop _for