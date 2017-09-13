#include "snabel/box.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"

namespace snabel {
  Nil nil;

  Box::Box(Scope &scp, Type &t, const Val &v):
    type(&t), val(v), safe_level(scp.safe_level)
  { }

  bool operator ==(const Box &x, const Box &y) {
    return x.type == y.type && x.type->eq(x, y);
  }
  
  bool operator !=(const Box &x, const Box &y) {
    return x.type != y.type || !x.type->eq(x, y);
  }

  bool operator <(const Box &x, const Box &y) {
    if (!x.type->lt) {
      ERROR(Snabel, fmt("Missing lt implementation for value: %0", x));
      return false;
    }
    
    return x.type == y.type && x.type->lt(x, y);
  }

  str dump(const List &lst) {
    OutStream buf;
    buf << '[';
    
    for (size_t i(0); i < lst.size(); i++) {
      if (i > 0) { buf << ' '; }
      auto &v(lst[i]);
      buf << v.type->dump(v);
      
      if (i == 100) {
	buf << "..." << lst.size();
	break;
      }
    }
    
    buf << ']';
    return buf.str();
  }

  str list_fmt(const List &lst) {
    OutStream buf;
    buf << '[';
    
    for (size_t i(0); i < lst.size(); i++) {
      if (i > 0) { buf << ' '; }
      auto &v(lst[i]);
      buf << v.type->fmt(v);
      
      if (i == 100) {
	buf << "..." << lst.size();
	break;
      }
    }
    
    buf << ']';
    return buf.str();
  }

  str dump(const Table &tbl) {
    OutStream buf;
    buf << '[';
    size_t i(0);
    
    for (auto it(tbl.begin()); it != tbl.end(); it++, i++) {
      if (i > 0) { buf << ' '; }
      buf << dump(*it);
      
      if (i == 100) {
	buf << "..." << tbl.size();
	break;
      }
    }
    
    buf << ']';
    return buf.str();
  }

  str table_fmt(const Table &tbl) {
    OutStream buf;
    buf << '[';
    size_t i(0);
    
    for (auto it(tbl.begin()); it != tbl.end(); it++, i++) {
      if (i > 0) { buf << ' '; }
      buf << pair_fmt(*it);
      
      if (i == 100) {
	buf << "..." << tbl.size();
	break;
      }
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

  bool empty(const Box &b) {
    return std::holds_alternative<Nil>(b.val);
  }
}

namespace snackis {
  template <>
  str fmt_arg(const snabel::Box &arg) { return arg.type->dump(arg); }
}
