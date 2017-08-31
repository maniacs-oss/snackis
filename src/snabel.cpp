#include <cstdio>
#include <fstream>
#include <iostream>

#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/io.hpp"
#include "snabel/snabel.hpp"
#include "snackis/core/opt.hpp"
#include "snackis/core/str.hpp"
#include "snackis/core/stream.hpp"

using namespace snabel;
using namespace snackis;

static opt<str> slurp(const str &fn) {
  std::ifstream f;
  f.open(fn, std::ifstream::in);
  
  if (f.fail()) {
    ERROR(Snabel, fmt("Failed opening file: %0", fn));
    return nullopt;
  }

  OutStream buf;
  buf << f.rdbuf();
  f.close();
  return buf.str();
}

static void say_imp(Scope &scp, const Args &args) {
  std::cout << *get<StrRef>(args.at(0)) << std::endl;
}

static void stdin_imp(Scope &scp, const Args &args) {
  push(scp.thread, scp.exec.rfile_type, std::make_shared<File>(fileno(stdin)));
}

int main(int argc, char** argv) {
  Exec exe;
  add_func(exe, "say", {ArgType(exe.str_type)}, say_imp);
  add_func(exe, "stdin", {}, stdin_imp);

  if (argc > 1) {
    TRY(try_load);

    for (int i=2; i < argc; i++) {
      push(exe.main, exe.str_type, std::make_shared<str>(argv[i]));
    }
    
    auto in(slurp(argv[1]));
    if (in) { run(exe, *in); }
    return try_load.errors.empty() ? 0 : -1;
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
	std::cout << fmt("%0\n%1!\n", res->type->dump(*res), name(res->type->name));
      }

      std::cout << std::endl;
    } else {
      in << ' ' << line;
    }
  }
  
  return 0;
}
