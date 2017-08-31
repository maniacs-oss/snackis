#include <iostream>

#include "snabel/exec.hpp"
#include "snabel/snabel.hpp"
#include "snackis/core/str.hpp"
#include "snackis/core/stream.hpp"

using namespace snabel;
using namespace snackis;

int main(int argc, char** argv) {
  Exec exe;

  add_func(exe, "say", {ArgType(exe.str_type)},
	   [](auto &scp, auto &args) {
	     std::cout << *get<StrRef>(args.at(0)) << std::endl;
	   });

  for (int i=1; i < argc; i++) {
    push(exe.main, exe.str_type, std::make_shared<str>(argv[i]));
  }
  
  OutStream in;
  std::cout << fmt("Snabel v%0\n\n", version_str());
  std::cout << fmt("Press Return on empty line to evaluate,\nCtrl-C exits.\n\n");
  str line;
  
  while (true) {
    std::cout << (line.empty() ? "S: " : "   ");
    std::getline(std::cin, line);

    if (line.empty()) {
      TRY(try_run);
      
      run(exe, in.str());
      in.str("");
      auto res(try_pop(exe.main));
      
      if (res) {
	std::cout << fmt("%0\n%1!\n", res->type->fmt(*res), name(res->type->name));
      }

      std::cout << std::endl;
    } else {
      in << ' ' << line;
    }
  }
  
  return 0;
}
