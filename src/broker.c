#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "broker.h"

struct mosquitto *broker_init(broker_t *bcfg,
	void (*cb)(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg), void *cbarg) {

	int rc;

	mosquitto_lib_init();

	struct mosquitto *mosq = mosquitto_new(NULL, true, cbarg);
	if (!mosq) {
		printf("Failed to open mosquitto library\n");
		return NULL;
	}

	mosquitto_message_callback_set(mosq, cb);

	rc = mosquitto_username_pw_set(mosq, bcfg->username, bcfg->password);
	if (rc) {
		printf("Failed to set mosq credentials %d\n", rc);
		return NULL;
	}

	rc = mosquitto_connect(mosq, bcfg->host, bcfg->port, 10);
	if (rc) {
		printf("Could not connect to Broker with return code %d\n", rc);
		return NULL;
	}

	return mosq;
}

int broker_subscribe(struct mosquitto *mosq, list_t *topic_list) {
	int rc;

	list_foreach(topic_list, t) {
		topic_t *topic = (topic_t *)t->data;

		printf("Subscribing topic '%s'\n", topic->name);

		rc = mosquitto_subscribe(mosq, NULL, topic->name, topic->qos);
		if (rc) {
			printf("Failed to subscribe topic '%s': %d\n",
				topic->name, rc);
			return -1;
		}
	}
}

void broker_step(struct mosquitto *mosq) {
	mosquitto_loop(mosq, -1, 1);
}

void broker_cleanup(struct mosquitto *mosq) {
	mosquitto_disconnect(mosq);
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
}
