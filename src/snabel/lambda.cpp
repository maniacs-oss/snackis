#include <iostream>
#include "snabel/lambda.hpp"
#include "snabel/scope.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  Lambda::Lambda(Label &lbl, Scope &scp):
    label(lbl), safe_level(scp.safe_level)
  {
    for (auto s(scp.thread.scopes.rbegin());
	 s != std::prev(scp.thread.scopes.rend()) &&
	 s->safe_level == scp.safe_level;
	 s++) {
      std::copy(s->env.begin(), s->env.end(), std::inserter(env, env.end()));
    }
  }

  bool call(const LambdaRef &lmb, Scope &scp, bool now) {
    scp.thread.lambda = lmb;
    return call(scp, lmb->label, now);
  }
}
