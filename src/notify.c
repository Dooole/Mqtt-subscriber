#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <syslog.h>

#include "notify.h"

static char payload[2048];
static const char *payload_format =
  "Date: \r\n"
  "To: admins\r\n"
  "From: noreply@mqttsub.com\r\n"
  "Subject: MQTTSUB Notification\r\n"
  "\r\n"
  "%s\r\n"
  "\r\n"
  "\r\n";
 
struct upload_status {
  size_t bytes_read;
};
 
static size_t payload_source(char *ptr, size_t size, size_t nmemb, void *userp)
{
  struct upload_status *upload_ctx = (struct upload_status *)userp;
  const char *data;
  size_t room = size * nmemb;
 
  if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
    return 0;
  }
 
  data = &payload[upload_ctx->bytes_read];
 
  if (data) {
    size_t len = strlen(data);
    if(room < len)
      len = room;
    memcpy(ptr, data, len);
    upload_ctx->bytes_read += len;
 
    return len;
  }
 
  return 0;
}
 
int notify_recipients(smtp_t *smtp, list_t *email_list, char *msg) {
  CURL *curl;
  CURLcode res = CURLE_OK;
  struct curl_slist *recipients = NULL;
  struct upload_status upload_ctx = { 0 };
  char url[128];

  curl = curl_easy_init();
  if (curl) {

    curl_easy_setopt(curl, CURLOPT_USERNAME, smtp->username);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, smtp->password);
    sprintf(url, "smtps://%s:465", smtp->host);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    
    curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_CAPATH, "/etc/certificates/");

    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, "noreply@mqttsub.com");
    list_foreach(email_list, e) {
        char *addr = (char *)e->data;
        recipients = curl_slist_append(recipients, addr);
	}
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
    sprintf(payload, payload_format, msg);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
    curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    res = curl_easy_perform(curl);
 
    if(res != CURLE_OK)
      syslog(LOG_ERR, "Curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));

    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);
  }
 
  return (int)res;
}
