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
  yield
  @in fifo @server write {@in z? &yield1 when} _for
)} call proc;

func: do-recv {(
  yield

  @server read {
    {@out $1 push _} when
    @out +? &do-out when _
    do-in
    idle
    yield1
  } for
)} call proc;

func: do-in {(
  yield

  stdin read {
    {@in $1 push _} when
    @in +? &do-send when _
    yield1
  } for
)} call proc;

func: do-out {(
  yield
  @out fifo stdout write {@out z? &yield1 when} _for
)} call proc;

stdin unblock _
&do-recv run
[&do-send &do-out] run &nop _for