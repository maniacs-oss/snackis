#ifndef SNABEL_OP_HPP
#define SNABEL_OP_HPP

#include <deque>
#include <utility>
#include <variant>
#include <vector>

#include "snabel/box.hpp"
#include "snabel/func.hpp"
#include "snabel/label.hpp"
#include "snabel/type.hpp"
#include "snabel/uid.hpp"
#include "snackis/core/func.hpp"
#include "snackis/core/str.hpp"

namespace snabel {
  using namespace snackis;

  struct Func;
  struct FuncImp;
  struct Scope;
  struct Op;
  
  enum OpCode { OP_BACKUP, OP_BEGIN, OP_BREAK, OP_CALL, OP_CAPTURE, OP_DEFUNC,
		OP_DEREF, OP_DROP, OP_DUP, OP_END, OP_FMT, OP_FOR, OP_FUNCALL, 
		OP_GETENV, OP_JUMP, OP_LOOP, OP_PUSH, OP_PUTENV, OP_RECALL, 
		OP_RESET, OP_RESTORE, OP_RETURN, OP_STASH, OP_SWAP, OP_TARGET,
	        OP_YIELD };

  using OpSeq = std::deque<Op>;

  struct OpImp {
    OpCode code;
    str name;
    int64_t pc;
    
    OpImp(OpCode code, const str &name);
    virtual OpImp &get_imp(Op &op) const = 0;
    virtual str info() const;
    virtual bool prepare(Scope &scp);
    virtual bool refresh(Scope &scp);
    virtual bool compile(const Op &op, Scope &scp, OpSeq &out);
    virtual bool finalize(const Op &op, Scope &scp, OpSeq &out);
    virtual bool run(Scope &scp);
  };

  struct Backup: OpImp {
    bool copy;
    
    Backup(bool copy=false);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool run(Scope &scp) override;
  };

  struct Begin: OpImp {
    Uid tag;
    Label &enter_label, &skip_label;
    bool compiled;
    
    Begin(Exec &exe);
    OpImp &get_imp(Op &op) const override;
    bool refresh(Scope &scp) override;
    bool compile(const Op &op, Scope &scp, OpSeq & out) override;
    bool run(Scope &scp) override;
  };

  struct Break: OpImp {
    int64_t depth;
    
    Break(int64_t dep);
    OpImp &get_imp(Op &op) const override;
    bool run(Scope &scp) override;
  };

  struct Call: OpImp {
    opt<Box> target;
    
    Call(opt<Box> target=nullopt);
    OpImp &get_imp(Op &op) const override;
    bool run(Scope &scp) override;
  };

  struct Capture: OpImp {
    Label &target;
    
    Capture(Label &tgt);
    OpImp &get_imp(Op &op) const override;
    bool run(Scope &scp) override;
  };

  struct Defunc: OpImp {
    Sym name;
    ArgTypes args;
    
    Defunc(const Sym &name, const ArgTypes &args);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool run(Scope &scp) override;
  };

  struct Deref: OpImp {
    Sym name;
    Types args;
    bool compiled;
    
    Deref(const Sym &name);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool prepare(Scope &scp) override;
    bool compile(const Op &op, Scope &scp, OpSeq & out) override;
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

  struct Dup: OpImp {
    Dup();
    OpImp &get_imp(Op &op) const override;
    bool run(Scope &scp) override;
  };

  struct End: OpImp {
    Label *enter_label, *skip_label;
    bool compiled;

    End(Exec &exe);
    OpImp &get_imp(Op &op) const override;
    bool refresh(Scope &scp) override;
    bool compile(const Op &op, Scope &scp, OpSeq & out) override;
    bool run(Scope &scp) override;
  };

  struct Fmt: OpImp {
    const str in;
    std::vector<std::pair<size_t, str>> subs;
    
    Fmt(const str &in);
    OpImp &get_imp(Op &op) const override;
    bool run(Scope &scp) override;
  };

  struct For: OpImp {
    bool push_vals;
    bool compiled;
    
    For(bool push_vals);
    OpImp &get_imp(Op &op) const override;
    bool compile(const Op &op, Scope &scp, OpSeq & out) override;
    bool run(Scope &scp) override;

    struct State {
      IterRef iter;
      Box target;

      State(const IterRef &itr, const Box &tgt);
    };    
  };

  struct Funcall: OpImp {
    Func &fn;
    FuncImp *imp;
    Types args;
    
    Funcall(Func &fn, const Types &args={});
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool prepare(Scope &scp) override;
    bool run(Scope &scp) override;
  };

  struct Getenv: OpImp {
    opt<Sym> id;
    
    Getenv(opt<Sym> id=nullopt);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool run(Scope &scp) override;
  };

  struct Jump: OpImp {
    const Sym tag;
    Label *label;
    
    Jump(const Sym &tag);
    Jump(Label &label);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool refresh(Scope &scp) override;
    bool run(Scope &scp) override;
  };

  struct Loop: OpImp {    
    bool compiled;
    
    Loop();
    OpImp &get_imp(Op &op) const override;
    bool compile(const Op &op, Scope &scp, OpSeq & out) override;    
    bool run(Scope &scp) override;

    struct State {
      Box target;
      State(const Box &cnd);
    };    
  };

  struct Push: OpImp {
    Stack vals;
    
    Push(const Box &val);
    Push(const Stack &vals);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool compile(const Op &op, Scope &scp, OpSeq & out) override;
    bool run(Scope &scp) override;
  };

  struct Putenv: OpImp {
    const Sym key;
    
    Putenv(const Sym &key);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool prepare(Scope &scp) override;
    bool run(Scope &scp) override;
  };

  struct Recall: OpImp {
    Label *label;
    int64_t depth;
    
    Recall(int64_t dep);
    OpImp &get_imp(Op &op) const override;
    bool run(Scope &scp) override;
  };

  struct Reset: OpImp {
    Reset();
    OpImp &get_imp(Op &op) const override;
    bool run(Scope &scp) override;
    bool finalize(const Op &op, Scope &scp, OpSeq & out) override;
  };

  struct Restore: OpImp {
    Restore();
    OpImp &get_imp(Op &op) const override;
    bool run(Scope &scp) override;
  };

  struct Return: OpImp {
    Label *target;
    int64_t depth;
    bool push_result;
    
    Return(int64_t dep, bool push_result);
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
    size_t pos;
    
    Swap(size_t pos=1);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool run(Scope &scp) override;
  };

  struct Target: OpImp {
    Label &label;
    
    Target(Label &label);
    OpImp &get_imp(Op &op) const override;
    str info() const override;
    bool finalize(const Op &op, Scope &scp, OpSeq & out) override;
  };

  struct Yield: OpImp {
    int64_t depth;
    bool push_result;
    
    Yield(int64_t depth, bool push_result);
    OpImp &get_imp(Op &op) const override;
    str info() const override;    
    bool run(Scope &scp) override;
  };

  using OpData = std::variant<Backup, Begin, Break, Call, Capture, Defunc, Deref, 
			      Drop, Dup, End, Fmt, For, Funcall, Getenv, Jump, Loop, 
			      Push, Putenv, Recall, Reset, Restore, Return, Stash,
			      Swap, Target, Yield>;

  using OpState = std::variant<For::State, Loop::State>;

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
  bool finalize(Op &op, Scope &scp, OpSeq &out);
  bool run(Op &op, Scope &scp);
}

#endif
