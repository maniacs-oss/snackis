#include <iostream>
#include <iterator>
#include "snackis/ctx.hpp"
#include "snackis/invite.hpp"
#include "snackis/core/fmt.hpp"
#include "snackis/core/stream.hpp"
#include "snackis/net/imap.hpp"

namespace snackis {
namespace net {
  ImapError::ImapError(const str &msg): Error(str("ImapError: ") + msg) { }

  static size_t on_read(char *ptr, size_t size, size_t nmemb, void *_out) {
    Stream *out = static_cast<Stream *>(_out);
    out->write(ptr, size * nmemb);
    return size * nmemb;  
  }

  static size_t skip_read(char *ptr, size_t size, size_t nmemb, void *_out) {
    return size * nmemb;  
  }

  Imap::Imap(Ctx &ctx): ctx(ctx), client(curl_easy_init()) {
    if (!client) {
      ERROR(Imap, "Failed initializing client");
      return;
    }
    
    curl_easy_setopt(client, 
		     CURLOPT_USERNAME, 
		     get_val(ctx.settings.imap.user)->c_str());
    curl_easy_setopt(client, 
		     CURLOPT_PASSWORD, 
		     get_val(ctx.settings.imap.pass)->c_str());
    curl_easy_setopt(client,
		     CURLOPT_URL,
		     fmt("imaps://%0:%1/INBOX",
			 *get_val(ctx.settings.imap.url),
			 *get_val(ctx.settings.imap.port)).c_str());
    curl_easy_setopt(client, CURLOPT_WRITEFUNCTION, on_read);
    //curl_easy_setopt(client, CURLOPT_VERBOSE, 1L);

    log(ctx, "Connecting to Imap...");
    
    TRY(try_connect);
    noop(*this);
  }

  Imap::~Imap() { curl_easy_cleanup(client); }

  void noop(const struct Imap &imap) {
    curl_easy_setopt(imap.client, CURLOPT_CUSTOMREQUEST, "NOOP");
    curl_easy_setopt(imap.client, CURLOPT_HEADERFUNCTION, nullptr);
    curl_easy_setopt(imap.client, CURLOPT_WRITEFUNCTION, skip_read);
    CURLcode res(curl_easy_perform(imap.client));
 
    if (res != CURLE_OK) {
      ERROR(Imap, fmt("Failed sending NOOP: %0", curl_easy_strerror(res))); 
    }
  }

  static void delete_uid(const struct Imap &imap, const str &uid) {
    curl_easy_setopt(imap.client,
		     CURLOPT_CUSTOMREQUEST,
		     fmt("UID STORE %0 +FLAGS.SILENT \\Deleted", uid).c_str());

    curl_easy_setopt(imap.client, CURLOPT_HEADERFUNCTION, nullptr);
    curl_easy_setopt(imap.client, CURLOPT_WRITEFUNCTION, skip_read);
    CURLcode res(curl_easy_perform(imap.client));
 
    if (res != CURLE_OK) {
      ERROR(Imap, fmt("Failed deleting uid: %0", curl_easy_strerror(res))); 
    }
  }

  static void expunge(const struct Imap &imap) {
    curl_easy_setopt(imap.client, CURLOPT_CUSTOMREQUEST, "EXPUNGE");

    curl_easy_setopt(imap.client, CURLOPT_HEADERFUNCTION, nullptr);
    curl_easy_setopt(imap.client, CURLOPT_WRITEFUNCTION, skip_read);
    CURLcode res(curl_easy_perform(imap.client));
 
    if (res != CURLE_OK) {
      ERROR(Imap, fmt("Failed expunging: %0", curl_easy_strerror(res)));
    }
  }

  static opt<Msg> fetch_uid(const struct Imap &imap, const str &uid) {
    curl_easy_setopt(imap.client,
		     CURLOPT_CUSTOMREQUEST,
		     fmt("UID FETCH %0 BODY[TEXT]", uid).c_str());

    Stream out;    
    curl_easy_setopt(imap.client, CURLOPT_HEADERFUNCTION, on_read);
    curl_easy_setopt(imap.client, CURLOPT_HEADERDATA, &out);
    CURLcode res(curl_easy_perform(imap.client));
 
    if (res != CURLE_OK) {
      ERROR(Imap, fmt("Failed fetching uid: %0", curl_easy_strerror(res)));
      return nullopt;
    }

    db::Rec<Msg> rec;
    Msg msg(imap.ctx, rec);
    const str body(out.str());
    const str tag("__SNACKIS__\r\n");
    auto i(body.find(tag) + tag.size());

    if (i == str::npos || !decode(msg, body.substr(i))) {
      ERROR(Imap, "Failed decoding message");
      return nullopt;
    }

    return msg;
  }

  void fetch(struct Imap &imap) {
    TRACE("Fetching email");
    Ctx &ctx(imap.ctx);
    
    log(ctx, "Fetching email...");
    curl_easy_setopt(imap.client,
		     CURLOPT_CUSTOMREQUEST,
		     "UID SEARCH Subject \"__SNACKIS__\"");

    Stream out;    
    curl_easy_setopt(imap.client, CURLOPT_HEADERFUNCTION, nullptr);
    curl_easy_setopt(imap.client, CURLOPT_HEADERDATA, nullptr);
    curl_easy_setopt(imap.client, CURLOPT_WRITEFUNCTION, on_read);
    curl_easy_setopt(imap.client, CURLOPT_WRITEDATA, &out);
    CURLcode res(curl_easy_perform(imap.client));
 
    if (res != CURLE_OK) {
      ERROR(Imap, fmt("Failed searching inbox: %0", curl_easy_strerror(res)));
      return;
    }

    std::vector<str> tokens {
      std::istream_iterator<str>{out}, std::istream_iterator<str>{}
    };

    if (tokens.size() < 2 || tokens[1] != "SEARCH") {
      ERROR(Imap, fmt("Invalid fetch result:\n%0", out.str()));
      return;
    }

    int msg_cnt = 0;
    
    if (tokens.size() > 2) {
      for (auto tok = std::next(tokens.begin(), 2); tok != tokens.end(); tok++) {
	db::Trans trans(ctx);
	TRY(try_msg);
	
	const str uid(*tok);
	opt<Msg> msg = fetch_uid(imap, uid);

	if (msg && try_msg.errors.empty()) {
	  receive(*msg);
	  
	  if (try_msg.errors.empty()) {
	    db::commit(trans, nullopt);
	    delete_uid(imap, uid);
	    msg_cnt++;
	  }
	}
      }

      if (msg_cnt) { expunge(imap); }
    }

    log(ctx, fmt("Finished fetching %0 messages", msg_cnt));
  }
}}
