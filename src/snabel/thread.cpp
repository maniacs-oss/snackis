#include <iostream>
#include "snackis/core/time.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/io.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  static void do_run(Thread *thd) {
    TRY(try_thread);
    thd->id = std::this_thread::get_id();

    {
      Exec::Lock lock(thd->exec.mutex);

      thd->exec.thread_lookup.emplace(std::piecewise_construct,
				      std::forward_as_tuple(*thd->id),
				      std::forward_as_tuple(thd));
    }
    
    run(*thd, false);
  }
  
  Thread::Thread(Exec &exe, opt<Id> id):
    exec(exe),
    id(id),
    pc(0),
    main(scopes.emplace_back(*this)),
    _stdin(std::make_shared<File>(*this, fileno(stdin), true)),
    _stdout(std::make_shared<File>(*this, fileno(stdout), true)),
    io_counter(0),
    random(std::random_device()())
  {
    stacks.emplace_back();
    poll(*_stdin);
  }

  static void thread_imp(Scope &scp, const Args &args) {
    auto &t(start_thread(scp, args.at(0)));
    push(scp, scp.exec.thread_type, &t);
  }

  static void join_imp(Scope &scp, const Args &args) {
    join(*get<Thread *>(args.at(0)), scp);
  }

  static void resched_imp(Scope &scp, const Args &args) {
    std::this_thread::yield();
  }

  static void sleep_imp(Scope &scp, const Args &args) {
    std::this_thread::sleep_for(usecs(get<int64_t>(args.at(0))));
  }

  void init_threads(Exec &exe) {
    exe.thread_type.supers.push_back(&exe.any_type);

    exe.thread_type.fmt = [](auto &v) {
      return fmt("thread_%0", *get<Thread *>(v)->id);
    };

    exe.thread_type.eq = [](auto &x, auto &y) {
      return get<Thread *>(x) == get<Thread *>(y);
    };

    add_func(exe, "thread", Func::Unsafe, {ArgType(exe.callable_type)}, thread_imp);
    add_func(exe, "join", Func::Unsafe, {ArgType(exe.thread_type)}, join_imp);
    add_func(exe, "resched", Func::Unsafe, {}, resched_imp);
    add_func(exe, "sleep", Func::Unsafe, {ArgType(exe.i64_type)}, sleep_imp);
  }

  Scope &curr_scope(Thread &thd) {
    CHECK(!thd.scopes.empty(), _);
    return thd.scopes.back();
  }

  Stack &curr_stack(Thread &thd) {
    CHECK(!thd.stacks.empty(), _);
    return thd.stacks.back();
  }

  const Stack &curr_stack(const Thread &thd) {
    CHECK(!thd.stacks.empty(), _);
    return thd.stacks.back();
  }

  void push(Thread &thd, const Box &val) {
    curr_stack(thd).push_back(val);
  }

  void push(Thread &thd, const Stack &vals) {
    std::copy(vals.begin(), vals.end(), std::back_inserter(curr_stack(thd)));
  }

  Box *peek(Thread &thd) {
    auto &s(curr_stack(thd));
    if (s.empty()) { return nullptr; }
    return &s.back();
  }

  opt<Box> try_pop(Thread &thd) {
    auto &s(curr_stack(thd));
    if (s.empty()) { return nullopt; }
    auto res(s.back());
    s.pop_back();
    return res;
  }

  Box pop(Thread &thd) {
    auto &s(curr_stack(thd));
    CHECK(!s.empty(), _);
    auto res(s.back());
    s.pop_back();
    return res;
  }

  Stack &backup_stack(Thread &thd, bool copy) {
    if (copy) { return thd.stacks.emplace_back(thd.stacks.back()); }
    return thd.stacks.emplace_back();
  }
  
  void reset_stack(Thread &thd, int64_t depth, bool push_result) {
    if (thd.stacks.size() > depth) {
      opt<Box> last;
      if (push_result && !thd.stacks.back().empty()) { last = pop(thd); }
      while (thd.stacks.size() > depth) { thd.stacks.pop_back(); }
      if (last) { push(thd, *last); }
    }
  }

  Scope &begin_scope(Thread &thd) {
    return thd.scopes.emplace_back(thd.scopes.back());
  }
  
  bool end_scope(Thread &thd) {
    if (thd.scopes.size() < 2) {
      ERROR(Snabel, "Open scope");
      return false;
    }

    thd.scopes.pop_back();
    return true;
  }

  int64_t find_break_pc(Thread &thd) {
    for (auto i(thd.scopes.rbegin()); i != thd.scopes.rend(); i++) {
      if (i->break_pc) { return i->break_pc; }
    }

    return -1;
  }
  
  void idle(Thread &thd) {
    if (thd.io_counter) {
      thd.io_counter = 0;
    } else {
      const int TMIN(2), TMAX(128);
      pollfd fds[POLL_SET_SIZE];
      auto f(thd.poll_queue.begin());
      int timeout(0);

      while (true) {
	int i(0);
	if (f == thd.poll_queue.end()) { f = thd.poll_queue.begin(); }
	
	for (; i < POLL_SET_SIZE && f != thd.poll_queue.end(); i++, f++) {
	  auto &pfd(fds[i]);
	  pfd.fd = (*f)->fd;
	  pfd.events = POLLIN;
	}

	while (i < POLL_SET_SIZE) { fds[i++].fd = -1; }
	
	switch (::poll(fds, POLL_SET_SIZE, timeout)) {
	case -1:
	  ERROR(Snabel, fmt("Failed polling: %0", errno));
	  return;
	case 0:
	    break;
	default:
	  return;
	}
	
	if (timeout < TMAX) { timeout = timeout ? timeout*2 : TMIN; }
      }
    }
  }
  
  bool isa(Thread &thd, const Types &x, const Types &y) {
    if (x.size() != y.size()) { return false; }
    auto i(x.begin()), j(y.begin());

    for (; i != x.end() && j != y.end(); i++, j++) {
      if (!isa(thd, **i, **j)) { return false; }
    }
    
    return i == x.end() && j == y.end();
  }
    
  void start(Thread &thd) { thd.imp = std::thread(do_run, &thd); }

  void join(Thread &thd, Scope &scp) {
    thd.imp.join();
    auto &s(curr_stack(thd));
    if (!s.empty()) { push(scp.thread, s); }
    Exec::Lock lock(scp.exec.mutex);
    thd.exec.thread_lookup.erase(*thd.id);
    thd.exec.threads.erase(thd.iter);
  }

  bool _break(Thread &thd, int64_t depth) {
    while (thd.scopes.size() >= 1) {
      auto &s(thd.scopes.back());

      if (s.break_pc > -1) {
	thd.pc = s.break_pc;
	s.return_pc = s.break_pc = -1;
	depth--;
	if (!depth) { return true; }
      }

      if (thd.scopes.size() == 1) { break; }
      end_scope(thd);
    }

    ERROR(Snabel, "Missing break pc");
    return false;
  }

  bool run(Thread &thd, int64_t break_pc) {
    auto scope_depth(thd.scopes.size());
    
    while (thd.pc < thd.ops.size() && thd.pc != break_pc) {
      auto &op(thd.ops[thd.pc]);
      auto prev_pc(thd.pc);
      
      if (!run(op, curr_scope(thd))) {
	while (thd.scopes.size() > scope_depth) {
	  curr_scope(thd).push_result = false;
	  end_scope(thd);
	}
	
	curr_scope(thd).return_pc = -1;
	return false;
      }
      
      if (thd.pc == prev_pc) { thd.pc++; }
    }

    return true;
  }
}

namespace snackis {
  template <>
  str fmt_arg(const std::thread::id &arg) {
    OutStream buf;
    buf << arg;
    return buf.str();
  }
}
