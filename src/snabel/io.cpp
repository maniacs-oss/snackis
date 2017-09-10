#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include "snabel/exec.hpp"
#include "snabel/io.hpp"

namespace snabel {
  IOBuf::IOBuf(int64_t size):
    data(size), rpos(0)
  { }

  File::File(Thread &thd, int fd, bool block):
    thread(thd), fd(fd)
  {
    if (!block) { unblock(*this); }
  }
  
  File::File(Thread &thd, const Path &path, int flags):
    thread(thd), fd(open(path.c_str(), flags | O_NONBLOCK, 0666))
  {
    if (fd == -1) {
      ERROR(Snabel, fmt("Failed opening file '%0': %1", path.string(), errno));
    }
  }

  File::~File() {
    if (fd != -1) { close(*this); }
  }
  
  ReadIter::ReadIter(Exec &exe, Type &elt, const Box &in):
    Iter(exe, get_iter_type(exe, elt)),
    in(in), elt(elt)
  { }
  
  opt<Box> ReadIter::next(Scope &scp) {
    if (!out) { out.emplace(elt, std::make_shared<Bin>(READ_BUF_SIZE)); }
    auto &buf(*get<BinRef>(*out));
    auto res((*in.type->read)(scp, in, buf));
    
    switch (res) {
    case READ_OK: {
      auto res(*out);
      out.reset();
      scp.thread.io_counter += buf.size();
      return res;
    }
    case READ_AGAIN:
      return Box(elt, nil);  
    case READ_EOF:
    case READ_ERROR:
      return nullopt;
    default:
      ERROR(Snabel, fmt("Invalid read result: %0", res));
    }

    return nullopt;
  }

  WriteIter::WriteIter(Exec &exe, const IterRef &in, const Box &out):
    Iter(exe, get_iter_type(exe, exe.i64_type)),
    in(in), out(out), result(exe.i64_type, (int64_t)-1), wpos(0)
  { }
  
  opt<Box> WriteIter::next(Scope &scp){
    auto &res(get<int64_t>(result));

    if (!in_buf) {
      auto nxt(in->next(scp));
      if (!nxt) { return nullopt; }

      if (empty(*nxt)) {
	res = 0;
	return result;
      }
      
      in_buf = get<BinRef>(*nxt);
    }

    auto &b(*in_buf);
    res = (*out.type->write)(scp, out, &b[wpos], b.size()-wpos);
    if (res == -1) { return nullopt; }
    wpos += res;
    scp.thread.io_counter += res;
    
    if (wpos == b.size()) {
      in_buf.reset();
      wpos = 0;
    }
    
    return result;
  }

  void unblock(File &f) {
    auto flags(fcntl(f.fd, F_GETFL, 0));
    fcntl(f.fd, F_SETFL, flags | O_NONBLOCK);
  }

  void poll(File &f) {    
    auto &thd(f.thread);

    if (f.poll_handle) {
      auto &i(*f.poll_handle);
      if (i == thd.poll_queue.begin()) { return; }
      thd.poll_queue.splice(thd.poll_queue.begin(), thd.poll_queue, i, std::next(i));
    } else {
      thd.poll_queue.emplace_front(&f);
      f.poll_handle = thd.poll_queue.begin();
    }
  }

  void unpoll(File &f) {
    if (f.poll_handle) {
      f.thread.poll_queue.erase(*f.poll_handle);
      f.poll_handle.reset();
    }
  }

  void close(File &f) {
    if (f.fd != -1) {
      if (f.poll_handle) { unpoll(f); }
      ::close(f.fd);
      f.fd = -1;
    }
  }

  static void stdin_imp(Scope &scp, const Args &args) {
    push(scp.thread, scp.exec.rfile_type, scp.thread._stdin);
  }
  
  static void stdout_imp(Scope &scp, const Args &args) {
    push(scp.thread, scp.exec.wfile_type, scp.thread._stdout);
  }

  static void rfile_imp(Scope &scp, const Args &args) {
    push(scp.thread,
	 scp.exec.rfile_type,
	 std::make_shared<File>(scp.thread, get<Path>(args.at(0)), O_RDONLY));
  }

  static void rwfile_imp(Scope &scp, const Args &args) {
    push(scp.thread,
	 scp.exec.rwfile_type,
	 std::make_shared<File>(scp.thread,
				get<Path>(args.at(0)),
				O_RDWR | O_CREAT | O_TRUNC));
  }

  static void unblock_imp(Scope &scp, const Args &args) {
    unblock(*get<FileRef>(args.at(0)));
  }
  
  static void file_p_imp(Scope &scp, const Args &args) {
    push(scp.thread, scp.exec.bool_type, is_file(get<Path>(args.at(0))));
  }

  static void read_imp(Scope &scp, const Args &args) {
    Exec &exe(scp.exec);
    auto &elt(get_opt_type(exe, exe.bin_type));
    
    push(scp.thread,
	 get_iter_type(exe, elt),
	 IterRef(new ReadIter(exe, elt, args.at(0))));
  }

  static void write_imp(Scope &scp, const Args &args) {
    Exec &exe(scp.exec);
    auto &in(args.at(0));
    auto &out(args.at(1));

    push(scp.thread,
	 get_iter_type(exe, exe.i64_type),
	 IterRef(new WriteIter(exe, (*in.type->iter)(in), out)));
  }

  static void close_imp(Scope &scp, const Args &args) {
    close(*get<FileRef>(args.at(0)));
  }

  static void ls_imp(Scope &scp, const Args &args) {
    Exec &exe(scp.exec);
    auto &in(get<Path>(args.at(0)));

    push(scp.thread,
	 get_iter_type(exe, exe.path_type),
	 IterRef(new DirIter<false>(exe, in)));
  }

  static void ls_r_imp(Scope &scp, const Args &args) {
    Exec &exe(scp.exec);
    auto &in(get<Path>(args.at(0)));

    push(scp.thread,
	 get_iter_type(exe, exe.path_type),
	 IterRef(new DirIter<true>(exe, in)));
  }

  static void idle_imp(Scope &scp, const Args &args) {
    idle(scp.thread);
  }
  
  void init_io(Exec &exe){
    exe.file_type.supers.push_back(&exe.ordered_type);

    exe.file_type.fmt = [](auto &v) {
      auto &f(*get<FileRef>(v));
      return fmt("File(%0)", f.fd);
    };
    
    exe.file_type.eq = [](auto &x, auto &y) {
      return get<FileRef>(x) == get<FileRef>(y);
    };

    exe.file_type.lt = [](auto &x, auto &y) {
      return get<FileRef>(x) < get<FileRef>(y);
    };
      
    exe.rfile_type.supers.push_back(&exe.file_type);
    exe.rfile_type.supers.push_back(&exe.readable_type);
    exe.rfile_type.fmt = [](auto &v) {
      return fmt("RFile(%0)", get<FileRef>(v)->fd);
    };
    
    exe.rfile_type.eq = exe.file_type.eq;
    exe.rfile_type.read = [](auto &scp, auto &in, auto &out) {
      auto &f(*get<FileRef>(in));
      if (f.fd == -1) { return READ_EOF; }
      auto res(read(f.fd, &out[0], out.size()));
      if (!res) { return READ_EOF; }

      if (res == -1) {
	if (errno == EAGAIN) { return READ_AGAIN; }
	ERROR(Snabel, fmt("Failed reading from file: %0", errno));
	return READ_ERROR;
      }
      
      out.resize(res);
      if (f.poll_handle) { poll(f); }
      return READ_OK;
    };

    exe.wfile_type.supers.push_back(&exe.file_type);
    exe.wfile_type.supers.push_back(&exe.writeable_type);

    exe.wfile_type.fmt = [](auto &v) {
      return fmt("Wfile(%0)", get<FileRef>(v)->fd);
    };
    
    exe.wfile_type.eq = exe.file_type.eq;

    exe.wfile_type.write = [](auto &scp, auto &out, auto data, auto len) {
      auto &f(*get<FileRef>(out));
      if (f.fd == -1) { return -1; }
      int res(write(f.fd, data, len));

      if (res == -1) {
	if (errno == EAGAIN) { return 0; }
	ERROR(Snabel, fmt("Failed writing to file %0: %1", f.fd, errno));
      }
      
      return res;
    };
    
    exe.rwfile_type.supers.push_back(&exe.file_type);
    exe.rwfile_type.supers.push_back(&exe.writeable_type);

    exe.rwfile_type.fmt = [](auto &v) {
      return fmt("RWFile(%0)", get<FileRef>(v)->fd);
    };
    
    exe.rwfile_type.eq = exe.file_type.eq;
    exe.rwfile_type.read = exe.rfile_type.read;
    exe.rwfile_type.write = exe.wfile_type.write;

    add_conv(exe, exe.str_type, exe.path_type, [&exe](auto &v) {	
	v.type = &exe.path_type;
	v.val = Path(*get<StrRef>(v));
	return true;
      });

    add_conv(exe, exe.rwfile_type, exe.rfile_type, [&exe](auto &v) {	
	v.type = &exe.rfile_type;
	return true;
      });

    add_func(exe, "stdin", {}, stdin_imp);
    add_func(exe, "stdout", {}, stdout_imp);

    add_func(exe, "rfile", {ArgType(exe.path_type)}, rfile_imp);
    add_func(exe, "rwfile", {ArgType(exe.path_type)}, rwfile_imp);
    add_func(exe, "unblock", {ArgType(exe.file_type)}, unblock_imp);
    add_func(exe, "file?", {ArgType(exe.path_type)}, file_p_imp);
    add_func(exe, "read", {ArgType(exe.readable_type)}, read_imp);

    add_func(exe, "write",
	     {ArgType(get_iterable_type(exe, exe.bin_type)),
		 ArgType(exe.writeable_type)},
	     write_imp);

    add_func(exe, "write",
	     {ArgType(get_iterable_type(exe, get_opt_type(exe, exe.bin_type))),
		 ArgType(exe.writeable_type)},
	     write_imp);
    
    add_func(exe, "close", {ArgType(exe.file_type)}, close_imp);

    add_func(exe, "ls", {ArgType(exe.path_type)}, ls_imp);    
    add_func(exe, "ls-r", {ArgType(exe.path_type)}, ls_r_imp);    
    add_func(exe, "idle", {}, idle_imp);    
  }
}
