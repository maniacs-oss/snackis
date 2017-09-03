#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "snabel/exec.hpp"
#include "snabel/error.hpp"
#include "snabel/io.hpp"
#include "snabel/net.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  AcceptIter::AcceptIter(Thread &thd, const FileRef &in):
    Iter(thd.exec, get_iter_type(thd.exec,
				 get_opt_type(thd.exec, thd.exec.tcp_stream_type))),
    in(in)
  {
    poll(thd, this->in);
  }
  
  opt<Box> AcceptIter::next(Scope &scp) {
    struct sockaddr_in addr;
    socklen_t addr_len(sizeof(addr));
    int fd(accept(in->fd, (struct sockaddr *)&addr, &addr_len));

    if (fd == -1) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
	ERROR(Snabel, fmt("Failed accepting client: %0", errno));
      }

      return Box(*type.args.at(0), nil);
    }

    auto f(std::make_shared<File>(fd));
    poll(scp.thread, f);
    return Box(*type.args.at(0), f);
  }
  
  static void tcp_socket_imp(Scope &scp, const Args &args) {
    auto fd(socket(AF_INET, SOCK_STREAM, 0));

    if (fd == -1) {
      ERROR(Snabel, fmt("Failed creating socket: %0", errno));
    }

    int so(1);
    setsockopt(fd, IPPROTO_TCP, TCP_QUICKACK, &so, sizeof(so));
    push(scp.thread, scp.exec.tcp_socket_type, std::make_shared<File>(fd));
  }

  static void tcp_connect_imp(Scope &scp, const Args &args) {
    auto &f(get<FileRef>(args.at(0)));
    auto &a(*get<StrRef>(args.at(1)));
    auto &p(get<int64_t>(args.at(2)));
    
    struct sockaddr_in addr;
    bzero((char *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(a.c_str());
    addr.sin_port = htons(p);

    if (connect(f->fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
      if (errno != EINPROGRESS) {
	ERROR(Snabel, fmt("Failed connecting: %0", errno));
      }
    }

    poll(scp.thread, f);
    push(scp.thread, scp.exec.tcp_stream_type, f);  
  }

  static void tcp_bind_imp(Scope &scp, const Args &args) {
    auto &f(get<FileRef>(args.at(0)));
    auto &a(*get<StrRef>(args.at(1)));

    int so=1;
    if (setsockopt(f->fd, SOL_SOCKET, SO_REUSEADDR, &so, sizeof(so)) == -1) {
      ERROR(Snabel, fmt("Failed setting reuse option: %0", errno));    
    }

    struct sockaddr_in addr;
    bzero((char *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = a.empty() ? INADDR_ANY : inet_addr(a.c_str());
    addr.sin_port = htons(get<int64_t>(args.at(2)));

    if (bind(f->fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
      ERROR(Snabel, fmt("Failed binding socket: %0", errno));    
    }

    push(scp.thread, scp.exec.tcp_server_type, f);  
  }

  static void tcp_accept_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &f(get<FileRef>(args.at(0)));
    auto &backlog(get<int64_t>(args.at(1)));

    if (listen(f->fd, backlog) == -1) {
      ERROR(Snabel, fmt("Failed listening on socket: %0", errno));    
    }

    push(scp.thread,
	 get_iter_type(exe, get_opt_type(exe, exe.tcp_stream_type)),
	 IterRef(new AcceptIter(scp.thread, f)));
  }

  void init_net(Exec &exe) {
    exe.tcp_socket_type.supers.push_back(&exe.file_type);
    
    exe.tcp_socket_type.fmt = [](auto &v) {
      return fmt("TCPSocket(%0)", get<FileRef>(v)->fd);
    };

    exe.tcp_socket_type.eq = exe.file_type.eq;
    
    exe.tcp_server_type.supers.push_back(&exe.any_type);
    exe.tcp_server_type.fmt = [](auto &v) {
      return fmt("TCPServer(%0)", get<FileRef>(v)->fd);
    };
    exe.tcp_server_type.eq = exe.file_type.eq;
    
    exe.tcp_stream_type.supers.push_back(&exe.file_type);
    exe.tcp_stream_type.supers.push_back(&exe.readable_type);
    exe.tcp_stream_type.supers.push_back(&exe.writeable_type);
    exe.tcp_stream_type.read = exe.rwfile_type.read;
    exe.tcp_stream_type.write = exe.rwfile_type.write;
      
    exe.tcp_stream_type.fmt = [](auto &v) {
      return fmt("TCPStream(%0)", get<FileRef>(v)->fd);
    };

    exe.tcp_stream_type.eq = exe.file_type.eq;
    exe.tcp_stream_type.lt = exe.file_type.lt;
    
    add_func(exe, "tcp-socket", {}, tcp_socket_imp);
    
    add_func(exe, "connect",
	     {ArgType(exe.tcp_socket_type),
		 ArgType(exe.str_type), ArgType(exe.i64_type)},
	     tcp_connect_imp);

    add_func(exe, "bind",
	     {ArgType(exe.tcp_socket_type),
		 ArgType(exe.str_type), ArgType(exe.i64_type)},
	     tcp_bind_imp);
    
    add_func(exe, "accept",
	     {ArgType(exe.tcp_server_type), ArgType(exe.i64_type)},
	     tcp_accept_imp);
  }
}