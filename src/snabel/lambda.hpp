#ifndef SNABEL_LAMBDA_HPP
#define SNABEL_LAMBDA_HPP

#include "snabel/box.hpp"
#include "snabel/env.hpp"

namespace snabel {
  struct Label;
  
  struct Lambda {
    Label &label;
    Env env;
    int64_t safe_level;
    
    Lambda(Label &lbl, Scope &scp);
  };

  using LambdaRef = std::shared_ptr<Lambda>;

  void call(const LambdaRef &lmb, Scope &scp, bool now);
}

#endif
