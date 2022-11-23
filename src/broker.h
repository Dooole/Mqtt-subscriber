#ifndef BROKER_H
#define BROKER_H

#include <mosquitto.h>

#include "config.h"

struct mosquitto *broker_init(broker_t *bcfg,
	void (*cb)(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg), void *cbarg);
int broker_subscribe(struct mosquitto *mosq, list_t *topic_list);
void broker_step(struct mosquitto *mosq);
void broker_cleanup(struct mosquitto *mosq);

#endif
