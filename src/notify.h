#ifndef NOTIFY_H
#define NOTIFY_H

#include "config.h"
#include "list.h"

int notify_recipients(smtp_t *smtp, list_t *email_list, char *msg);

#endif
