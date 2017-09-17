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

func: do-send() (
  |_yield
  @in fifo @server write &_yield _for
);

func: do-recv() (
  |_yield

  @server read {
    {@out $1 push} when
    _yield1
  } for

  @server close
  @do-send stop
  @do-in stop
  @do-out stop
);

func: do-in() (
  |_yield

  stdin read {
    {@in $1 push} when
    _yield1
    idle
  } for
);

func: do-out() (
  |_yield
  @out fifo stdout write &_yield _for
);

let: do-send do-send;
let: do-in do-in;
let: do-out do-out;

stdin unblock
[@do-send do-recv @do-in @do-out] run