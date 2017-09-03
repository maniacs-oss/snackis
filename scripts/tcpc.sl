#!/usr/local/bin/snabel

"tcpc.sl"

"Opens a TCP/IP connection to ADDR:PORT and forwards data from/to stdin/stdout."

"Usage:           "
"tcpc.sl ADDR PORT"

let: port stoi64; _
let: addr; _
let: server tcp-socket @addr @port connect;
let: in Bin list;
let: out Bin list;

func: do-send {(
  let: q @in fifo;
  yield
  
  @q @server write &yield _for
)} call proc;

func: do-recv {(
  yield

  @server read {
    {@out $1 push _} when
    @out +? &do-out when _
    yield1
  } for
)} call proc;

func: do-in {(
  yield

  stdin read {
    {@in $1 push _} when
    @in +? &do-send when _
    do-recv
    idle
    yield1
  } for
)} call proc;

func: do-out {(
  let: q @out fifo;
  yield

  @q stdout write &yield _for
)} call proc;

stdin unblock _
[&do-in] run &nop _for
[&do-send &do-out] run &nop _for
