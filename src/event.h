#ifndef EVENT_H
#define EVENT_H

#include <json-c/json.h>

#include "config.h"

int event_check(event_t *event, struct json_object *msg);

#endif
