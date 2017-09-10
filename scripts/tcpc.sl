#!/usr/local/bin/snabel

/*
   tcpc.sl

   Opens a TCP/IP connection to ADDR:PORT and forwards data from/to stdin/stdout.

   Usage:
   tcpc.sl ADDR PORT
*/

let: port stoi64; _
let: addr; _
let: server tcp-socket @addr @port connect;
let: in Bin list;
let: out Bin list;

func: do-send (
  |yield
  @in fifo @server write &yield _for
);

func: do-recv (
  |yield

  @server read {
    {@out $1 push} when
    yield1
  } for

  @server close
  @do-send stop
  @do-in stop
  @do-out stop
);

func: do-in (
  |yield

  stdin read {
    {@in $1 push} when
    yield1
    idle
  } for
);


func: do-out (
  |yield
  @out fifo stdout write &yield _for
);

let: do-send do-send proc;
let: do-in do-in proc;
let: do-out do-out proc;

stdin unblock
[@do-send do-recv proc @do-in @do-out] run