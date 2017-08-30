#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include "snabel/exec.hpp"
#include "snabel/io.hpp"

namespace snabel {
  IOBuf::IOBuf(int64_t size):
    data(size), rpos(0)
  { }

  File::File(const Path &path, int flags):
    path(path), fd(open(path.c_str(), flags | O_NONBLOCK, 0666))
  {
    if (fd == -1) {
      ERROR(Snabel, fmt("Failed opening file '%0': %1", path.string(), errno));
    }
  }

  File::~File() {
    if (fd != -1) { close(fd); }
  }

  ReadIter::ReadIter(Exec &exe, Type &elt, const Box &in):
    Iter(exe, get_iter_type(exe, elt)),
    in(in), elt(elt)
  { }
  
  opt<Box> ReadIter::next(Scope &scp) {
    if (!out) { out.emplace(elt, std::make_shared<Bin>(READ_BUF_SIZE)); }
    auto res((*in.type->read)(in, *get<BinRef>(*out)));
    
    switch (res) {
    case READ_OK: {
      auto res(out);
      out.reset();
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
    if (!in_buf) {
      auto nxt(in->next(scp));
      if (!nxt) { return nullopt; }
      in_buf = get<BinRef>(*nxt);
    }

    auto &b(*in_buf);
    auto &res(get<int64_t>(result));
    res = (*out.type->write)(out, &b[wpos], b.size()-wpos);
    if (res == -1) { return nullopt; }
    wpos += res;
    
    if (wpos == b.size()) {
      in_buf.reset();
      wpos = 0;
    }
    
    return result;
  }

  static void rfile_imp(Scope &scp, const Args &args) {
    push(scp.thread,
	 scp.exec.rfile_type,
	 std::make_shared<File>(get<Path>(args.at(0)), O_RDONLY));
  }

  static void rwfile_imp(Scope &scp, const Args &args) {
    push(scp.thread,
	 scp.exec.rwfile_type,
	 std::make_shared<File>(get<Path>(args.at(0)), O_RDWR | O_CREAT | O_TRUNC));
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
    push(scp.thread, out);
    push(scp.thread,
	 get_iter_type(exe, exe.i64_type),
	 IterRef(new WriteIter(exe, (*in.type->iter)(in), out)));
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

  void init_io(Exec &exe){
    exe.file_type.supers.push_back(&exe.any_type);
    exe.file_type.fmt = [](auto &v) {
      auto &f(*get<FileRef>(v));
      return fmt("File(%0)", f.fd);
    };
    
    exe.file_type.eq = [](auto &x, auto &y) {
      return get<FileRef>(x) == get<FileRef>(y);
    };

    exe.rfile_type.supers.push_back(&exe.file_type);
    exe.rfile_type.supers.push_back(&exe.readable_type);
    exe.rfile_type.fmt = [](auto &v) {
      return fmt("RFile(%0)", get<FileRef>(v)->fd);
    };
    
    exe.rfile_type.eq = exe.file_type.eq;
    exe.rfile_type.read = [](auto &in, auto &out) {
      auto &f(*get<FileRef>(in));
      auto res(read(f.fd, &out[0], out.size()));
      if (!res) { return READ_EOF; }

      if (res == -1) {
	if (errno == EAGAIN) { return READ_AGAIN; }
	ERROR(Snabel, fmt("Failed reading from file: %0", errno));
	return READ_ERROR;
      }

      out.resize(res);
      return READ_OK;
    };

    exe.rwfile_type.supers.push_back(&exe.file_type);
    exe.rwfile_type.supers.push_back(&exe.writeable_type);

    exe.rwfile_type.fmt = [](auto &v) {
      return fmt("RWFile(%0)", get<FileRef>(v)->fd);
    };
    
    exe.rwfile_type.eq = exe.file_type.eq;
    exe.rwfile_type.read = exe.rfile_type.read;
    exe.rwfile_type.write = [](auto &out, auto data, auto len) {
      auto &f(*get<FileRef>(out));
      int res(write(f.fd, data, len));

      if (res == -1) {
	if (errno == EAGAIN) { return 0; }
	ERROR(Snabel, fmt("Failed writing to file %0: %1", f.fd, errno));
      }
      
      return res;
    };

    add_conv(exe, exe.str_type, exe.path_type, [&exe](auto &v) {	
	v.type = &exe.path_type;
	v.val = Path(get<str>(v));
	return true;
      });

    add_conv(exe, exe.rwfile_type, exe.rfile_type, [&exe](auto &v) {	
	v.type = &exe.rfile_type;
	return true;
      });

    add_func(exe, "rfile", {ArgType(exe.path_type)}, rfile_imp);
    add_func(exe, "rwfile", {ArgType(exe.path_type)}, rwfile_imp);
    add_func(exe, "file?", {ArgType(exe.path_type)}, file_p_imp);
    add_func(exe, "read", {ArgType(exe.readable_type)}, read_imp);

    add_func(exe, "write",
	     {ArgType(get_iterable_type(exe, exe.bin_type)),
		 ArgType(exe.writeable_type)},
	     write_imp);
    
    add_func(exe, "ls", {ArgType(exe.path_type)}, ls_imp);    
    add_func(exe, "ls-r", {ArgType(exe.path_type)}, ls_r_imp);    
  }
}
