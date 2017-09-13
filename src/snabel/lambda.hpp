#ifndef SNABEL_LAMBDA_HPP
#define SNABEL_LAMBDA_HPP

#include "snabel/box.hpp"
#include "snabel/env.hpp"

namespace snabel {
  struct Lambda {
    Label &label;
    Env env;

    Lambda(Label &lbl, Scope &scp);
  };

  void call(const LambdaRef &lmb, Scope &scp, bool now);
}

#endif
