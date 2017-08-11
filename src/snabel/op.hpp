#ifndef SNABEL_OP_HPP
#define SNABEL_OP_HPP

#include <deque>
#include <utility>

#include "snabel/box.hpp"
#include "snabel/label.hpp"
#include "snackis/core/func.hpp"
#include "snackis/core/str.hpp"

namespace snabel {
  using namespace snackis;

  struct Box;
  struct Func;
  struct FuncImp;
  struct Scope;
  struct Op;

  enum OpCode { OP_BACKUP, OP_BRANCH, OP_CALL, OP_DROP, OP_EXIT, OP_FUNCALL,
	        OP_GET, OP_GROUP, OP_JUMP,  OP_LAMBDA, OP_LET,
		OP_PUSH, OP_RESET, OP_RESTORE, OP_RETURN, OP_STASH, OP_SWAP, 
		OP_TARGET, OP_UNGROUP, OP_UNLAMBDA };

  using OpSeq = std::deque<Op>;

  struct OpImp {
    const OpCode code;
    const str name;
    
    OpImp(OpCode code, const str &name);
    virtual OpImp &get_imp(Op &op) const = 0;
    virtual str info() const;
    virtual void prepare(Scope &scp);
    virtual void refresh(Scope &scp);
    virtual bool trace(Scope &scp);
    virtual bool compile(const Op &op, Scope &scp, OpSeq &out);
    virtual bool run(Scope &scp);
  };

  struct Backup: OpImp {
    bool copy;
    
    Backup(bool copy);
    OpImp &get_imp(Op &op) const override;
    bool trace(Scope &scp) override;
    bool run(Scope &scp) override;
  };

  struct Branch: OpImp {
    Label *label;
    opt<bool> cond;

    Branch();
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool trace(Scope &scp) override;
    bool compile(const Op &op, Scope &scp, OpSeq & out) override;
    bool run(Scope &scp) override;
  };

  struct Call: OpImp {
    Label *label;

    Call();
    Call(Label &label);
    OpImp &get_imp(Op &op) const override;
    str info() const override;

    bool trace(Scope &scp) override;
    bool run(Scope &scp) override;
  };

  struct Drop: OpImp {
    size_t count;

    Drop(size_t count);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool trace(Scope &scp) override;
    bool compile(const Op &op, Scope &scp, OpSeq & out) override;
    bool run(Scope &scp) override;
  };

  struct Exit: OpImp {
    Label *label;
    
    Exit();
    OpImp &get_imp(Op &op) const override;
    bool trace(Scope &scp) override;
    bool compile(const Op &op, Scope &scp, OpSeq & out) override;
    bool run(Scope &scp) override;
  };

  struct Funcall: OpImp {
    Func &fn;
    FuncImp *imp;
    opt<Stack> result;

    Funcall(Func &fn);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool trace(Scope &scp) override;
    bool compile(const Op &op, Scope &scp, OpSeq & out) override;
    bool run(Scope &scp) override;
  };

  struct Get: OpImp {
    str name;
    opt<Box> val;

    Get(const str &name);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool trace(Scope &scp) override;
    bool compile(const Op &op, Scope &scp, OpSeq & out) override;
    bool run(Scope &scp) override;
  };

  struct Group: OpImp {
    bool copy;
    
    Group(bool copy);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool trace(Scope &scp) override;
    bool run(Scope &scp) override;
  };

  struct Jump: OpImp {
    str tag;
    Label *label;
    
    Jump(const str &tag);
    Jump(Label &label);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool trace(Scope &scp) override;
    bool run(Scope &scp) override;
  };

  struct Lambda: OpImp {
    str tag;
    bool compiled;
    
    Lambda();
    OpImp &get_imp(Op &op) const override;
    void prepare(Scope &scp) override;
    bool trace(Scope &scp) override;
    bool compile(const Op &op, Scope &scp, OpSeq & out) override;
    bool run(Scope &scp) override;
  };

  struct Let: OpImp {
    str name;
    opt<Box> val;
    
    Let(const str &name);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    void prepare(Scope &scp) override;
    bool trace(Scope &scp) override;
    bool run(Scope &scp) override;
  };

  struct Push: OpImp {
    Stack vals;
    
    Push(const Box &val);
    Push(const Stack &vals);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool trace(Scope &scp) override;
    bool run(Scope &scp) override;
  };

  struct Reset: OpImp {
    Reset();
    OpImp &get_imp(Op &op) const override;
    bool trace(Scope &scp) override;
    bool run(Scope &scp) override;
  };

  struct Restore: OpImp {
    Restore();
    OpImp &get_imp(Op &op) const override;
    bool trace(Scope &scp) override;
    bool run(Scope &scp) override;
  };

  struct Return: OpImp {
    Return();
    OpImp &get_imp(Op &op) const override;
    bool trace(Scope &scp) override;
    bool run(Scope &scp) override;
  };
  
  struct Stash: OpImp {
    Stash();
    OpImp &get_imp(Op &op) const override;
    bool trace(Scope &scp) override;
    bool run(Scope &scp) override;
  };

  struct Swap: OpImp {
    Swap();
    OpImp &get_imp(Op &op) const override;
    bool trace(Scope &scp) override;
    bool run(Scope &scp) override;
  };

  struct Target: OpImp {
    str tag;
    Label *label;
    int64_t depth, pc;
    
    Target(const str &tag);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    void prepare(Scope &scp) override;
    void refresh(Scope &scp) override;
  };

  struct Ungroup: OpImp {
    Ungroup();
    OpImp &get_imp(Op &op) const override;
    bool trace(Scope &scp) override;
    bool run(Scope &scp) override;
  };

  struct Unlambda: OpImp {
    str tag;
    bool compiled;

    Unlambda();
    OpImp &get_imp(Op &op) const override;
    bool trace(Scope &scp) override;
    bool compile(const Op &op, Scope &scp, OpSeq & out) override;
    bool run(Scope &scp) override;
  };

  using OpData = std::variant<Backup, Branch, Call, Drop, Exit, Funcall, Get, Group,
			      Jump, Lambda, Let, Push, Reset, Restore, Return,
			      Stash, Swap, Target, Ungroup, Unlambda>;

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

  void prepare(Op &op, Scope &scp);
  void refresh(Op &op, Scope &scp);
  bool trace(Op &op, Scope &scp);
  bool compile(Op &op, Scope &scp, OpSeq &out);
  bool run(Op &op, Scope &scp);
}

#endif
