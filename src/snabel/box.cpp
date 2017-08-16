#include "snabel/box.hpp"
#include "snabel/exec.hpp"

namespace snabel {
  const Undef undef;

  bool Undef::operator ()(const Box &box) const {
    return box.val.index() == 0;
  }

  Box::Box(Type &t, const Val &v):
    type(&t), val(v)
  { }

  bool operator ==(const Box &x, const Box &y) {
    return x.type == y.type && x.type->eq(x, y);
  }
  
  bool operator !=(const Box &x, const Box &y) {
    return x.type != y.type || !x.type->eq(x, y);
  }

  str dump(const List &lst) {
    OutStream buf;
    buf << '[';
    
    if (lst.size() < 4) {
      for (size_t i(0); i < lst.size(); i++) {
	if (i > 0) { buf << ' '; }
	auto &v(lst[i]);
	buf << v.type->dump(v);
      };
    } else {
      buf <<
	lst.front().type->dump(lst.front()) <<
	"..." <<
	lst.back().type->dump(lst.back());
    }
    
    buf << ']';
    return buf.str();
  }

  str list_fmt(const List &lst) {
    OutStream buf;
    buf << '[';
    
    if (lst.size() < 4) {
      for (size_t i(0); i < lst.size(); i++) {
	if (i > 0) { buf << ' '; }
	auto &v(lst[i]);
	buf << v.type->fmt(v);
      };
    } else {
      buf <<
	lst.front().type->fmt(lst.front()) <<
	"..." <<
	lst.back().type->fmt(lst.back());
    }
    
    buf << ']';
    return buf.str();
  }

  str dump(const Pair &pr) {
    auto &l(pr.first), &r(pr.second);
    return fmt("%0 %1.", l.type->dump(l), r.type->dump(r));
  }

  str pair_fmt(const Pair &pr) {
    auto &l(pr.first), &r(pr.second);
    return fmt("%0 %1.", l.type->fmt(l), r.type->fmt(r));
  }
}

namespace snackis {
  template <>
  str fmt_arg(const snabel::Box &arg) {
    return fmt("%0 %1!",
	       arg.type->dump(arg),
	       arg.type->name);
  }
}
