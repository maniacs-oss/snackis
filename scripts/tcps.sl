#!/usr/local/bin/snabel

"tcps.sl"

"Starts a TCP/IP server on ADDR:PORT and forwards incoming data to STDOUT and all"
"connected clients except the sender.                                            "

"Usage:           "
"tcps.sl ADDR PORT"

let: port stoi64; _
let: addr; _

struct: Client
  queue  List<Bin>
  sender Proc;

let: clients TCPStream Client table;
let: out Bin list;

let: server tcp-socket
     @addr @port bind
     1 accept;

func: do-recv {(
  let: in;
  | yield

  @in read {{
    let: data; _

    @clients {unzip $1 @in = {
      queue @data push _
      sender call
    } unless _} for

    @out @data push _
    do-out} when

    yield1
  } for

  @clients @in del _
  @in close
  'disconnect' say
)};

func: do-send {(
  let: q; _
  let: c; _
  | yield

  @q fifo @c write {@q z? &yield1 when} _for
)};

func: do-server {(
  | yield

  @server {
    { 'connect' say
      
      let: in;
      Bin list
      let: q;
      let: s do-send proc; _ _

      @clients @in
        Client new
          @q set-queue
          @s set-sender
      put _
      
      @procs @in do-recv proc $1 _ push _
    } when

    idle
    yield1
  } for
)} call proc;

func: do-out {(
  | yield
  @out fifo stdout write {@out z? &yield1 when} _for
)} call proc;

let: procs [&do-server];
@procs run &nop _for