#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "broker.h"

struct mosquitto *broker_init(config_t *cfg,
	void (*cb)(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg), void *cbarg) {

	int rc;

	mosquitto_lib_init();

	struct mosquitto *mosq = mosquitto_new(NULL, true, cbarg);
	if (!mosq) {
		syslog(LOG_ERR, "Failed to open mosquitto library\n");
		return NULL;
	}

	mosquitto_message_callback_set(mosq, cb);

	if (cfg->certfile[0]) {
		rc = mosquitto_tls_set(mosq, cfg->certfile, NULL, NULL, NULL, NULL);
		if (rc) {
			syslog(LOG_ERR, "Failed to set mosq tls %d\n", rc);
			return NULL;
		}
	}

	rc = mosquitto_username_pw_set(mosq, cfg->username, cfg->password);
	if (rc) {
		syslog(LOG_ERR, "Failed to set mosq credentials %d\n", rc);
		return NULL;
	}

	rc = mosquitto_connect(mosq, cfg->host, cfg->port, 10);
	if (rc) {
		syslog(LOG_ERR, "Could not connect to Broker with return code %d\n", rc);
		return NULL;
	}

	return mosq;
}

int broker_subscribe(struct mosquitto *mosq, list_t *topic_list) {
	int rc;

	list_foreach(topic_list, t) {
		topic_t *topic = (topic_t *)t->data;

		syslog(LOG_NOTICE,"Subscribing topic '%s'\n", topic->name);

		rc = mosquitto_subscribe(mosq, NULL, topic->name, topic->qos);
		if (rc) {
			syslog(LOG_ERR, "Failed to subscribe topic '%s': %d\n",
				topic->name, rc);
			return -1;
		}
	}
	return 0;
}

void broker_step(struct mosquitto *mosq) {
	int rc;

	rc = mosquitto_loop(mosq, -1, 1);
	switch (rc) {
		case MOSQ_ERR_NO_CONN:
		case MOSQ_ERR_CONN_LOST:
			syslog(LOG_ERR, "Broker connection lost (%d), reconnecting...", rc);
			if (mosquitto_reconnect(mosq)) {
				syslog(LOG_ERR, "Reconnecting failed, sleep for 5s");
				sleep(5);
			}
			break;
	}
}

void broker_cleanup(struct mosquitto *mosq) {
	mosquitto_disconnect(mosq);
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
}
