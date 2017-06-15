#include <cassert>
#include <iostream>
#include <iterator>
#include "snackis/ctx.hpp"
#include "snackis/snackis.hpp"
#include "snackis/core/fmt.hpp"
#include "snackis/core/stream.hpp"
#include "snackis/net/smtp.hpp"

namespace snackis {
  SmtpError::SmtpError(const str &msg): Error(str("SmtpError: ") + msg) { }

  static size_t on_read(char *ptr, size_t size, size_t nmemb, void *_out) {
    Stream *out = static_cast<Stream *>(_out);
    out->write(ptr, size * nmemb);
    return size*nmemb;  
  }

  static size_t on_write(void *ptr, size_t size, size_t nmemb, void *_smtp) {
    if (!size || !nmemb || ((size*nmemb) < 1)) {
      return 0;
    }
  
    Smtp *smtp = static_cast<Smtp *>(_smtp);
    const size_t len(smtp->data.size());
    if (len == 0) { return 0; }
    memcpy(ptr, &smtp->data[0], len);
    smtp->data.clear();
    return len;
  }
  
  Smtp::Smtp(Ctx &ctx): ctx(ctx), trans(ctx), client(curl_easy_init()) {
    if (!client) { ERROR(Smtp, "Failed initializing client"); }
    curl_easy_setopt(client, 
		     CURLOPT_USERNAME, 
		     get_val(ctx.settings.smtp_user)->c_str());
    curl_easy_setopt(client, 
		     CURLOPT_PASSWORD, 
		     get_val(ctx.settings.smtp_pass)->c_str());
    curl_easy_setopt(client,
		     CURLOPT_URL,
		     fmt("smtp://%0:%1",
			 *get_val(ctx.settings.smtp_url),
			 *get_val(ctx.settings.smtp_port)).c_str());
    curl_easy_setopt(client, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
    curl_easy_setopt(client, CURLOPT_READFUNCTION, on_write);
    curl_easy_setopt(client, CURLOPT_READDATA, this);
    curl_easy_setopt(client, CURLOPT_WRITEFUNCTION, on_read);
    //curl_easy_setopt(client, CURLOPT_VERBOSE, 1L);
    
    log(ctx, "Connecting to Smtp...");

    try {
      noop(*this);
    } catch (const SmtpError &e) {
      ERROR(Smtp, fmt("Failed connecting: %0", e.what()));
    }
  }

  Smtp::~Smtp() { curl_easy_cleanup(client); }

  void noop(const struct Smtp &smtp) {
    curl_easy_setopt(smtp.client, CURLOPT_CUSTOMREQUEST, "NOOP");
    curl_easy_setopt(smtp.client, CURLOPT_UPLOAD, 0L);

    Stream resp_buf;
    curl_easy_setopt(smtp.client, CURLOPT_WRITEDATA, &resp_buf);
    CURLcode res(curl_easy_perform(smtp.client));
 
    if (res != CURLE_OK) {
      ERROR(Smtp, fmt("Failed sending NOOP: %0", curl_easy_strerror(res))); 
    }

    std::vector<str> resp {
      std::istream_iterator<str>{resp_buf}, std::istream_iterator<str>{}
    };

    if (resp.size() < 3 || resp[2] != "OK") {
      ERROR(Smtp, fmt("Invalid NOOP response: %0", resp_buf.str()));
    }
  }

  void send(struct Smtp &smtp, Msg &msg) {
    Peer &me(whoami(smtp.ctx));
    msg.from = me.email;
    
    curl_easy_setopt(smtp.client, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(smtp.client, CURLOPT_MAIL_FROM, msg.from.c_str());
    struct curl_slist *to = nullptr;
    to = curl_slist_append(to, msg.to.c_str());
    curl_easy_setopt(smtp.client, CURLOPT_MAIL_RCPT, to);
    
    smtp.data.clear();
    const str msg_str(fmt("From: %0\r\n"
			  "To: %1\r\n"
			  "Subject: __SNACKIS__ %2\r\n\r\n"
			  "This message was generated by Snackis v%3, "
			  "visit https://github.com/andreas-gone-wild/snackis "
			  "for more information.\r\n\r\n"
			  "__SNACKIS__\r\n",
			  msg.from, msg.to, msg.id, version_str()) +
		      encode(msg));
    smtp.data.assign(msg_str.begin(), msg_str.end());
		  
    Stream resp_buf;
    curl_easy_setopt(smtp.client, CURLOPT_WRITEDATA, &resp_buf);
    CURLcode res(curl_easy_perform(smtp.client));
    
    if (res != CURLE_OK) {
      ERROR(Smtp, fmt("Failed sending email: %0", curl_easy_strerror(res)));
    }

    if (resp_buf.str() != "") {
      ERROR(Smtp, fmt("Invalid send response: %0", resp_buf.str()));
    }
  }
  
  void send(struct Smtp &smtp) {
    TRACE("Sending email");
    db::Table<Msg> &tbl(smtp.ctx.db.outbox);
    log(smtp.ctx, "Sending %0 messages...", tbl.recs.size());
    
    while (tbl.recs.size() > 0) {
      auto i = tbl.recs.begin();
      Msg msg(tbl, *i);
      send(smtp, msg);
      erase(tbl, *i);
    }
    
    db::commit(smtp.trans);
    log(smtp.ctx, "Finished sending email");
  }
}
