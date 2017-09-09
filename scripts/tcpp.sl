#!/usr/local/bin/snabel

/*
   tcpp.sl

   Starts a TCP/IP server on IN_ADDR:IN_PORT, opens a connection to OUT_ADDR:OUT_PORT
   for each client and forwards data in both directions until either side disconnects.

   Usage:                                 
   tcpp.sl IN_ADDR IN_PORT OUT_ADDR OUT_PORT
*/

let: out-port stoi64; _
let: out-addr; _
let: in-port stoi64; _
let: in-addr; _

let: server tcp-socket
     @in-addr @in-port bind
     1 accept;

func: do-send {(
  let: out; _
  let: in;
  | yield
  
  @out read @in write &yield _for
  
  @in close
  @out close
)};

func: do-recv {(
  let: out; _
  let: in;
  | yield
  
  @in read @out write &yield _for

  @in close
  @out close
  'disconnect' say
)};

func: do-server {(
  | yield

  @server {
    {'connect' say
     tcp-socket @out-addr @out-port connect
     do-recv proc @procs $1 push _
     do-send proc @procs $1 push _ _ _} when

    yield1
    idle
  } for
)} call proc;

let: procs [&do-server];
@procs run