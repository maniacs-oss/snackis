#include <cstdio>
#include <fstream>
#include <iostream>

#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/io.hpp"
#include "snabel/snabel.hpp"
#include "snackis/core/io.hpp"
#include "snackis/core/opt.hpp"
#include "snackis/core/str.hpp"
#include "snackis/core/stream.hpp"

using namespace snabel;
using namespace snackis;

static void say_imp(Scope &scp, const Args &args) {
  std::cout << *get<StrRef>(args.at(0)) << std::endl;
}

int main(int argc, char** argv) {
  error_handler = [](auto &errors) {
    for (auto e: errors) { std::cerr << e->what << std::endl; }
  };

  Exec exe;
  add_func(exe, "say", Func::Unsafe, {ArgType(exe.str_type)}, say_imp);

  if (argc > 1) {
    for (int i=2; i < argc; i++) {
      push(exe.main_scope, exe.str_type, std::make_shared<str>(argv[i]));
    }
    
    auto in(slurp(argv[1]));
    if (in) { run(exe, *in); }
    return 0;
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

      if (run(exe, in.str())) {
	auto res(try_pop(exe.main));
      
	if (res) {
	  std::cout << fmt("%0\n%1\n", res->type->dump(*res), name(res->type->name));
	} else {
	  std::cout << "nil" << std::endl;
	}
      }

      std::cout << std::endl;
      in.str("");
    } else {
      in << ' ' << line;
    }
  }
  
  return 0;
}
