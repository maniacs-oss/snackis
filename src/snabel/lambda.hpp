#ifndef SNABEL_LAMBDA_HPP
#define SNABEL_LAMBDA_HPP

#include "snabel/box.hpp"
#include "snabel/env.hpp"
#include "snabel/refs.hpp"

namespace snabel {
  struct Label;
  
  struct Lambda {
    Label &label;
    Env env;
    int64_t safe_level;
    
    Lambda(Label &lbl, Scope &scp);
  };

  bool call(const LambdaRef &lmb, Scope &scp, bool now);
}

#endif
