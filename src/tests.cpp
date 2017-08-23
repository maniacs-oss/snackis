#include <iostream>

#include "snackis/ctx.hpp"
#include "snackis/snackis.hpp"
#include "snackis/core/chan.hpp"
#include "snackis/core/data.hpp"
#include "snackis/core/bool_type.hpp"
#include "snackis/core/int64_type.hpp"
#include "snackis/core/set_type.hpp"
#include "snackis/core/str_type.hpp"
#include "snackis/core/str.hpp"
#include "snackis/core/stream.hpp"
#include "snackis/core/time_type.hpp"
#include "snackis/core/uid_type.hpp"
#include "snackis/crypt/key.hpp"
#include "snackis/crypt/secret.hpp"
#include "snackis/db/col.hpp"
#include "snackis/db/proc.hpp"
#include "snackis/db/table.hpp"
#include "snackis/net/imap.hpp"

using namespace snackis;
using namespace snackis::db;

/*
static void str_tests() {
  CHECK(find_ci("foo", "bar"), _ == str::npos);
  CHECK(find_ci("foobar", "BAR"), _ == 3);
}

static void crypt_secret_tests() {
  using namespace snackis::crypt;
  str key("secret key");
  Secret sec;
  init_salt(sec);
  init(sec, key);
  str msg("secret message");
  
  Data cmsg(encrypt(sec, (const unsigned char *)msg.c_str(), msg.size())),
    dmsg(decrypt(sec, &cmsg[0], cmsg.size()));

  CHECK(str(dmsg.begin(), dmsg.end()), _ == msg);
}

static void crypt_key_tests() {
  using namespace snackis::crypt;
  PubKey foo_pub, bar_pub;
  crypt::Key foo(foo_pub), bar(bar_pub);
  str msg("secret message");

  Data cmsg(encrypt(foo, bar_pub, (const unsigned char *)msg.c_str(), msg.size())),
    dmsg(decrypt(bar, foo_pub, &cmsg[0], cmsg.size()));

  CHECK(str(dmsg.begin(), dmsg.end()), _ == msg);
}

static void chan_tests() {
  const int MAX(100);
  Chan<int> c(MAX);

  CHECK(get(c, false), !_);
  for (int i = 0; i < MAX; i++) { CHECK(put(c, i), _); }
  CHECK(put(c, 42, false), !_);
  for (int i = 0; i < MAX; i++) { CHECK(get(c), *_ == i); }
  CHECK(get(c, false), !_);
  CHECK(put(c, 42), _);

  close(c);
}

struct Foo {
  int64_t fint64;
  str fstr;
  Time ftime;
  UId fuid;
  std::set<int64_t> fset;
  Foo(): fint64(0), ftime(now()), fuid(true) { }

  Foo(db::Table<Foo, UId> &tbl, const db::Rec<Foo> &rec) {
    copy(tbl, *this, rec);
  }
};

namespace snackis {
  template <>
  str fmt_arg(const Foo &arg) {
    return "Foo";
  }
}

static void fmt_tests() {
  CHECK(fmt("%0 %1\n", "abc", 42), _ == "abc 42\n");
  CHECK(fmt("%0 %%1 %1", "abc", 42), _ == "abc %1 42");
  CHECK(fmt("%0 %1 %2", "abc", Foo(), "42"), _ == "abc Foo 42");
}

static void schema_tests() {
  const Col<Foo, int64_t> col("int64", int64_type, &Foo::fint64); 
  Schema<Foo> scm({&col});

  Rec<Foo> foo, bar;
  set(foo, col, int64_t(42));
  CHECK(compare(scm, foo, bar), _ == -1);

  set(bar, col, int64_t(42));
  CHECK(compare(scm, foo, bar), _ == 0);
  
  set(bar, col, int64_t(43));
  CHECK(compare(scm, foo, bar), _ == -1);
}

const Col<Foo, int64_t> int64_col("int64", int64_type, &Foo::fint64); 
const Col<Foo, str> str_col("str", str_type, &Foo::fstr); 
const Col<Foo, Time> time_col("time", time_type, &Foo::ftime); 
const Col<Foo, UId> uid_col("uid", uid_type, &Foo::fuid); 
SetType<int64_t> int64_set(int64_type);
const Col<Foo, std::set<int64_t>> set_col("set",
					  int64_set,
					  &Foo::fset); 

const size_t MAX_BUF(32);

void table_insert_tests() {
  Proc proc("testdb/", MAX_BUF);
  db::Ctx ctx(proc, MAX_BUF);
  Table<Foo, UId> tbl(ctx, "insert_tests", db::make_key(uid_col),
		 {&int64_col, &str_col, &time_col});
  
  Foo foo;
  Trans trans(ctx);
  CHECK(insert(tbl, foo), _);
  CHECK(load(tbl, foo), _);
  CHECK(!insert(tbl, foo), _);
  CHECK(load(tbl, foo), _);
}

static void table_slurp_tests() {
  Proc proc("testdb/", MAX_BUF);
  db::Ctx ctx(proc, MAX_BUF);
  Table<Foo, UId> tbl(ctx, "slurp_tests", db::make_key(uid_col),
		      {&int64_col, &str_col, &time_col});

  Stream buf;
  
  Foo foo, bar;
  Trans trans(ctx);
  CHECK(insert(tbl, foo), _);
  CHECK(insert(tbl, bar), _);
  commit(trans, nullopt);

  dump(tbl, buf);
  tbl.recs.clear();
  slurp(tbl, buf);

  CHECK(load(tbl, foo), _);
  CHECK(load(tbl, bar), _);
}

static void read_write_tests() {
  Proc proc("testdb/", MAX_BUF);
  db::Ctx ctx(proc, MAX_BUF);
  Table<Foo, UId> tbl(ctx, "read_write_tests", db::make_key(uid_col),
		      {&int64_col, &str_col, &time_col, &set_col});
  
  crypt::Secret sec;
  init(sec, "secret key");

  Foo foo;
  foo.fint64 = 42;
  foo.fstr = "abc";
  foo.ftime = now();
  for (int i = 0; i < 100; i++) { foo.fset.insert(i); }
  
  Rec<Foo> rec;
  copy(tbl, rec, foo);
  
  Stream buf;
  write(rec, buf, sec);
  Rec<Foo> rrec;
  read(tbl, buf, rrec, sec);
  CHECK(compare(tbl, rrec, rec), _ == 0);
}

static void email_tests() {
  TRACE("Running email_tests");
  Proc proc("testdb/", MAX_BUF);
  snackis::Ctx ctx(proc, MAX_BUF);
  ctx.db.inbox.recs.clear();
  Imap imap(ctx);
  fetch(imap);
}
*/

namespace snabel {
  void all_tests();
}

int main() {
  TRY(try_tests);
  std::cout << "Snackis v" << version_str() << std::endl;
  
  /*  str_tests();
  fmt_tests();
  crypt_secret_tests();
  crypt_key_tests();
  chan_tests();
  schema_tests();
  table_insert_tests();
  table_slurp_tests();
  read_write_tests();
  email_tests();*/
  snabel::all_tests();
  return 0;
}
