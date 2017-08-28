#include <iostream>
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/pair.hpp"
#include "snabel/iters.hpp"
#include "snabel/type.hpp"

namespace snabel {
  FilterIter::FilterIter(Exec &exe, const IterRef &in, Type &elt, const Box &tgt):
    Iter(exe, get_iter_type(exe, elt)),
    in(in), target(tgt)
  { }
  
  opt<Box> FilterIter::next(Scope &scp) {
    auto &thd(scp.thread);

    while (true) {
      auto x(in->next(scp));
      if (!x) { return nullopt; }
      push(thd, *x);
      (*target.type->call)(scp, target, true);
      auto out(peek(thd));
      
      if (!out) {
	ERROR(Snabel, "Filter iterator target failed");
	return nullopt;
      }
      
      if (out->type != &scp.exec.bool_type) {
	ERROR(Snabel, fmt("Invalid filter target result: %0", *out));
	return nullopt;
      }
      
      if (get<bool>(*out)) { return *x; }
    }
    
    return nullopt;
  }

  SplitIter::SplitIter(Exec &exe, const IterRef &in, SplitFn fn):
    Iter(exe, get_iter_type(exe, get_opt_type(exe, exe.str_type))),
    in(in), split(fn)
  { }
  
  opt<Box> SplitIter::next(Scope &scp) {
    Box out(*type.args.at(0), n_a);
    
    while (true) {
      if (in && (!in_buf || in_pos == in_buf->end())) {
	auto nxt(in->next(scp));

	if (nxt) {
	  in_buf = get<BinRef>(*nxt);
	  in_pos = std::find_if_not(in_buf->begin(), in_buf->end(), split);
	} else {
	  in.reset();
	}
      }
      
      if (!in_buf) {
	if (!out_buf.tellp()) { return nullopt; }
	out.val = out_buf.str();
	out_buf.str("");
	break;
      }
      
      auto fnd(std::find_if(in_pos, in_buf->end(), split));
      
      if (fnd == in_buf->end()) {
	auto i(in_pos - in_buf->begin());
	out_buf.write(reinterpret_cast<const char *>(&*in_pos),
		      in_buf->size()-i);
	in_buf.reset();
      } else {
	out_buf.write(reinterpret_cast<const char *>(&*in_pos), fnd-in_pos);
	
	if (out_buf.tellp()) {
	  out.val = out_buf.str();
	  out_buf.str("");
	}

	in_pos = std::find_if_not(fnd+1, in_buf->end(), split);
	break;
      }
    }
    
    return out;
  }
  
  MapIter::MapIter(Exec &exe, const IterRef &in, const Box &tgt):
    Iter(exe, get_iter_type(exe, exe.any_type)),
    in(in), target(tgt)
  { }
  
  opt<Box> MapIter::next(Scope &scp) {
    auto &thd(scp.thread);
    auto x(in->next(scp));
    if (!x) { return nullopt; }
    push(thd, *x);
    (*target.type->call)(scp, target, true);
    auto out(peek(thd));
    
    if (!out) {
      ERROR(Snabel, "Map iterator target failed");
      return nullopt;
    }
    
    return *out;
  }

  RandomIter::RandomIter(Exec &exe, const RandomRef &in):
    Iter(exe, get_iter_type(exe, exe.i64_type)),
    in(in), out(exec.i64_type, (int64_t)0)
  { }
  
  opt<Box> RandomIter::next(Scope &scp) {
    get<int64_t>(out) = (*in)(scp.thread.random);
    return out;		
  }

  ZipIter::ZipIter(Exec &exe, const IterRef &xin, const IterRef &yin):
    Iter(exe, get_iter_type(exe, get_pair_type(exe,
					       *xin->type.args.at(0),
					       *yin->type.args.at(0)))),
    xin(xin), yin(yin)
  { }
  
  opt<Box> ZipIter::next(Scope &scp) {
    auto x(xin->next(scp)), y(yin->next(scp));
    if (!x || !y) { return nullopt; }
    return Box(get_pair_type(exec, *x->type, *y->type),
	       std::make_shared<Pair>(*x, *y));
  }
}
