#!/usr/local/bin/snabel

/*
   tcps.sl

   Starts a TCP/IP server on ADDR:PORT and forwards incoming data to STDOUT and all
   connected clients except the sender.

   Usage:
   tcps.sl ADDR PORT
*/

let: port stoi64; _
let: addr; _

let: clients TCPStream List<Bin> table;
let: out Bin list;

let: server tcp-socket
     @addr @port bind
     1 accept;

func: do-send (
  let: q; _
  let: c;
  |_yield

  @q fifo @c write &_yield _for
);

func: do-recv (
  let: in;
  |_yield

  @in read {
    { let: data; _

      @clients {
        unzip $1 @in = {_} {@data push} if
      } for

      @out @data push
    } when

    _yield1
  } for

  @clients @in del
  @in close
  'disconnect' say
);

func: do-server (
  |_yield

  @server {
    { 'connect' say
      do-recv @procs $1 push
      Bin list
      do-send @procs $1 push
      @clients $2 $2 put
    } when

    _yield1
    idle
  } for
);

func: do-out (
  |_yield
  @out fifo stdout write &_yield _for
);

let: procs [do-server do-out];
@procs run