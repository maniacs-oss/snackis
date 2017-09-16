#ifndef SNABEL_TABLE_HPP
#define SNABEL_TABLE_HPP

#include <deque>
#include "snabel/box.hpp"
#include "snabel/iter.hpp"

namespace snabel {
  using Table = std::map<Box, Box>;
  using TableRef = std::shared_ptr<Table>;

  struct TableIter: Iter {
    Type &elt;
    TableRef in;
    Table::const_iterator it;
    
    TableIter(Exec &exe, Type &elt, const TableRef &in);
    opt<Box> next(Scope &scp) override;
  };

  void init_tables(Exec &exe);
  Type &get_table_type(Exec &exe, Type &key, Type &val);
  void uneval(const Table &tbl, std::ostream &out);
  str dump(const Table &tbl);
  str table_fmt(const Table &tbl);
}

#endif
