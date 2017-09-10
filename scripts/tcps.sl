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

func: do-send {(
  let: q; _
  let: c;
  | yield

  @q fifo @c write &yield _for
)};

func: do-recv {(
  let: in;
  | yield

  @in read {
    { let: data; _

      @clients {
        unzip $1 @in = {_} {@data push} if
      } for

      @out @data push
    } when

    yield1
  } for

  @clients @in del _
  @in close
  'disconnect' say
)};

func: do-server {(
  | yield

  @server {
    { 'connect' say
      do-recv proc @procs $1 push
      Bin list
      do-send proc @procs $1 push
      @clients $2 $2 put
    } when

    yield1
    idle
  } for
)} call proc;

func: do-out {(
  | yield
  @out fifo stdout write &yield _for
)} call proc;

let: procs [&do-server &do-out];
@procs run