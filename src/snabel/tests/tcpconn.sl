let: addr '127.0.0.1';
let: port 31331;

let: server-socket
     tcp-socket;
     
let: server
     @server-socket
     @addr @port bind
     0 accept;

let: sender tcp-socket
     @addr @port connect {&break &_ if} for;

let: receiver
     @server {&break &_ if} for;

['hello world' bytes] @sender write &nop _for
@sender close
@receiver read unopt words unopt list
@server-socket close
