#ifndef SNABEL_OP_HPP
#define SNABEL_OP_HPP

#include <deque>
#include <utility>

#include "snabel/box.hpp"
#include "snabel/iter.hpp"
#include "snabel/label.hpp"
#include "snackis/core/func.hpp"
#include "snackis/core/str.hpp"

namespace snabel {
  using namespace snackis;

  struct Box;
  struct Func;
  struct FuncImp;
  struct Iter;
  struct Scope;
  struct Op;

  enum OpCode { OP_BACKUP, OP_BRANCH, OP_CALL, OP_DEREF, OP_DROP, OP_DUP,
		OP_FOR, OP_FUNCALL, OP_GETENV, OP_GROUP, OP_JUMP, OP_LAMBDA,
		OP_PUSH, OP_PUTENV, OP_RECALL, OP_RESET, OP_RESTORE, OP_RETURN,
		OP_STASH, OP_SWAP, OP_TARGET, OP_UNGROUP, OP_UNLAMBDA };

  using OpSeq = std::deque<Op>;

  struct OpImp {
    const OpCode code;
    const str name;
    
    OpImp(OpCode code, const str &name);
    virtual OpImp &get_imp(Op &op) const = 0;
    virtual str info() const;
    virtual bool prepare(Scope &scp);
    virtual bool refresh(Scope &scp);
    virtual bool compile(const Op &op, Scope &scp, OpSeq &out);
    virtual bool run(Scope &scp);
  };

  struct Backup: OpImp {
    bool copy;
    
    Backup(bool copy);
    OpImp &get_imp(Op &op) const override;
    bool run(Scope &scp) override;
  };

  struct Branch: OpImp {
    opt<Box> target;
    
    Branch(opt<Box> target=nullopt);
    OpImp &get_imp(Op &op) const override;
    bool prepare(Scope &scp) override;
    bool run(Scope &scp) override;
  };

  struct Call: OpImp {
    opt<Box> target;
    
    Call(opt<Box> target=nullopt);
    OpImp &get_imp(Op &op) const override;
    bool run(Scope &scp) override;
  };

  struct Drop: OpImp {
    size_t count;

    Drop(size_t count);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool compile(const Op &op, Scope &scp, OpSeq & out) override;
    bool run(Scope &scp) override;
  };

  struct Deref: OpImp {
    str name;

    Deref(const str &name);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool compile(const Op &op, Scope &scp, OpSeq & out) override;
    bool run(Scope &scp) override;
  };

  struct Dup: OpImp {
    Dup();
    OpImp &get_imp(Op &op) const override;
    bool run(Scope &scp) override;
  };

  struct For: OpImp {
    bool compiled;
    opt<Iter> iter;
    
    For();
    OpImp &get_imp(Op &op) const override;
    bool compile(const Op &op, Scope &scp, OpSeq & out) override;
    bool run(Scope &scp) override;
  };

  struct Funcall: OpImp {
    Func &fn;

    Funcall(Func &fn);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool run(Scope &scp) override;
  };

  struct Getenv: OpImp {
    const str id;
    
    Getenv(const str &id="");
    OpImp &get_imp(Op &op) const override;
    bool compile(const Op &op, Scope &scp, OpSeq & out) override;
    bool run(Scope &scp) override;
  };

  struct Group: OpImp {
    bool copy;
    
    Group(bool copy);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool refresh(Scope &scp) override;
    bool run(Scope &scp) override;
  };

  struct Jump: OpImp {
    str tag;
    Label *label;
    
    Jump(const str &tag);
    Jump(Label &label);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool refresh(Scope &scp) override;
    bool run(Scope &scp) override;
  };

  struct Lambda: OpImp {
    str tag;
    Label *exit_label;
    bool compiled;
    
    Lambda();
    OpImp &get_imp(Op &op) const override;
    bool prepare(Scope &scp) override;
    bool refresh(Scope &scp) override;
    bool compile(const Op &op, Scope &scp, OpSeq & out) override;
    bool run(Scope &scp) override;
  };

  struct Push: OpImp {
    Stack vals;
    
    Push(const Box &val);
    Push(const Stack &vals);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool run(Scope &scp) override;
  };

  struct Putenv: OpImp {
    str name;
    opt<Box> val;
    
    Putenv(const str &name);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool prepare(Scope &scp) override;
    bool run(Scope &scp) override;
  };

  struct Recall: OpImp {
    Label *label;
    
    Recall();
    OpImp &get_imp(Op &op) const override;
    bool refresh(Scope &scp) override;
    bool run(Scope &scp) override;
  };

  struct Reset: OpImp {
    Reset();
    OpImp &get_imp(Op &op) const override;
    bool run(Scope &scp) override;
  };

  struct Restore: OpImp {
    Restore();
    OpImp &get_imp(Op &op) const override;
    bool run(Scope &scp) override;
  };

  struct Return: OpImp {
    bool scoped;
    
    Return(bool scoped);
    OpImp &get_imp(Op &op) const override;
    bool refresh(Scope &scp) override;
    bool run(Scope &scp) override;
  };
  
  struct Stash: OpImp {
    Stash();
    OpImp &get_imp(Op &op) const override;
    bool run(Scope &scp) override;
  };

  struct Swap: OpImp {
    Swap();
    OpImp &get_imp(Op &op) const override;
    bool run(Scope &scp) override;
  };

  struct Target: OpImp {
    str tag;
    Label *label;
    
    Target(const str &tag);
    Target(Label &label);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool prepare(Scope &scp) override;
    bool refresh(Scope &scp) override;
  };

  struct Ungroup: OpImp {
    Ungroup();
    OpImp &get_imp(Op &op) const override;
    bool refresh(Scope &scp) override;
    bool run(Scope &scp) override;
  };

  struct Unlambda: OpImp {
    str tag;
    bool compiled;

    Unlambda();
    OpImp &get_imp(Op &op) const override;
    bool refresh(Scope &scp) override;
    bool compile(const Op &op, Scope &scp, OpSeq & out) override;
    bool run(Scope &scp) override;
  };

  using OpData = std::variant<Backup, Branch, Call, Deref, Drop, Dup, For, Funcall,
			      Getenv, Group, Jump, Lambda, Push, Putenv, 
			      Recall, Reset, Restore, Return, Stash, Swap, Target,
			      Ungroup, Unlambda>;

  struct Op {
    OpData data;
    OpImp &imp;
    bool prepared;
    
    template <typename ImpT>
    Op(const ImpT &imp);
    Op(const Op &src);
  };

  template <typename ImpT>
  Op::Op(const ImpT &imp): data(imp), imp(get<ImpT>(data)), prepared(false)
  { }

  bool prepare(Op &op, Scope &scp);
  bool refresh(Op &op, Scope &scp);
  bool compile(Op &op, Scope &scp, OpSeq &out);
  bool run(Op &op, Scope &scp);
}

#endif
