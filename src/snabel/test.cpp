#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/op.hpp"
#include "snabel/test.hpp"

namespace snabel {
  void init_tests(Exec &exe) {
    add_macro(exe, "test:", [&exe](auto pos, auto &in, auto &out) {
	if (in.empty()) {
	  ERROR(Snabel, fmt("Malformed test on row %0, col %1",
			    pos.row, pos.col));
	  return;
	}
	
	str msg(in.at(0).text);
	in.pop_front();
	out.emplace_back(Backup(false));
	auto end(find_end(in.begin(), in.end()));
	compile(exe, TokSeq(in.begin(), end), out);
	in.erase(in.begin(), (end == in.end()) ? end : std::next(end));
	out.emplace_back(Restore());
	out.emplace_back(Test(msg));
      });
  }
}
