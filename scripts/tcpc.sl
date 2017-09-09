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

func: do-send {(
  | yield
  @in fifo @server write &yield _for
)} call proc;

func: do-recv {(
  | yield

  @server read {
    {@out $1 push _} when
    yield1
  } for

  @server close
  &do-send stop
  &do-in stop
  &do-out stop
)} call proc;

func: do-in {(
  | yield

  stdin read {
    {@in $1 push _} when
    yield1
    idle
  } for
)} call proc;

func: do-out {(
  | yield
  @out fifo stdout write &yield _for
)} call proc;

stdin unblock _
[&do-send &do-recv &do-in &do-out] run