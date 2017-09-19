let: addr '127.0.0.1';
let: port 31331;

let: server-socket
     tcp-socket;
     
let: server
     @server-socket
     @addr @port bind;

let: client
     tcp-socket
     @addr @port connect;

func: do-connect() (
  |yield
  @client {&break &_yield1 if} for
);

func: do-accept() (
  |yield
  @server 0 accept {&break &_yield1 if} for
);

[do-accept do-connect] run

let: in; _
let: out; _

['hello world' bytes] @out write &nop _for
@out close
@in read unopt words unopt list
@server close