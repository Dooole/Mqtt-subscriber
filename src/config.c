#include <stdlib.h>
#include <string.h>
#include <uci.h>
#include <syslog.h>

#include "config.h"

static void apply_defaults(config_t *cfg) {
	cfg->port = 1883;
}

static int parse_smtp(config_t *cfg,
		struct uci_context *uci, struct uci_package *pkg) {
	struct uci_option *opt;
	struct uci_element *elm;

	uci_foreach_element(&pkg->sections, elm) {
		struct uci_section *sec = uci_to_section(elm);
		if (!strcmp(sec->type, "smtp")) {
			opt = uci_lookup_option(uci, sec, "host");
			if (opt && opt->type == UCI_TYPE_STRING)
				strncpy(cfg->smtp.host, opt->v.string, CONFIG_STRLEN);
			else {
				syslog(LOG_ERR, "No host in smtp config\n");
				return -1;
			}

			opt = uci_lookup_option(uci, sec, "username");
			if (opt && opt->type == UCI_TYPE_STRING)
				strncpy(cfg->smtp.username, opt->v.string, CONFIG_STRLEN);
			else {
				syslog(LOG_ERR, "No username in smtp config\n");
				return -1;
			}

			opt = uci_lookup_option(uci, sec, "password");
			if (opt && opt->type == UCI_TYPE_STRING)
				strncpy(cfg->smtp.password, opt->v.string, CONFIG_STRLEN);
			else {
				syslog(LOG_ERR, "No password in smtp config\n");
				return -1;
			}

			opt = uci_lookup_option(uci, sec, "certfile");
			if (opt && opt->type == UCI_TYPE_STRING)
				strncpy(cfg->smtp.certfile, opt->v.string, CONFIG_STRLEN);
			else {
				syslog(LOG_ERR, "No certfile in smtp config\n");
				return -1;
			}

			break;
		}
	}

	return 0;
}

static int parse_topics(config_t *cfg,
		struct uci_context *uci, struct uci_package *pkg) {
	struct uci_option *opt_name, *opt_qos;
	struct uci_element *elm;

	if (!cfg->topic_list) {
		cfg->topic_list = list_create();
		if (!cfg->topic_list) {
			syslog(LOG_ERR, "Failed to create topics list\n");
			return -1;
		}
	}

	uci_foreach_element(&pkg->sections, elm) {
		struct uci_section *sec = uci_to_section(elm);
		if (!strcmp(sec->type, "topic")) {
			opt_name = uci_lookup_option(uci, sec, "name");
			if (!(opt_name && opt_name->type == UCI_TYPE_STRING)) {
				syslog(LOG_ERR, "No name in topics config\n");
				return -1;
			}

			opt_qos = uci_lookup_option(uci, sec, "qos");
			if (!(opt_qos && opt_qos->type == UCI_TYPE_STRING)) {
				syslog(LOG_ERR, "No qos in topics config\n");
				return -1;
			}

			topic_t *topic = list_newnode(cfg->topic_list, sizeof(topic_t));
			strncpy(topic->name, opt_name->v.string, CONFIG_STRLEN);
			topic->qos = atoi(opt_qos->v.string);

			topic->event_list = list_create(); 
			if (!topic->event_list) {
				syslog(LOG_ERR, "Failed to create event list\n");
				return -1;
			}
		}
	}

	return 0;
}

static int parse_event(event_t *event,
		struct uci_context *uci, struct uci_section *sec) {
	struct uci_option *opt;

	opt = uci_lookup_option(uci, sec, "param");
	if (opt && opt->type == UCI_TYPE_STRING) {
		strncpy(event->param, opt->v.string, CONFIG_STRLEN);
	} else {
		syslog(LOG_ERR, "No param in event config\n");
		return -1;
	}

	opt = uci_lookup_option(uci, sec, "type");
	if (opt && opt->type == UCI_TYPE_STRING) {
		if (!strcmp(opt->v.string, "number"))
			event->threshold.type = THRESHOLD_TYPE_NUM;
		else if (!strcmp(opt->v.string, "string"))
			event->threshold.type = THRESHOLD_TYPE_STR;
		else {
			syslog(LOG_ERR, "Not a valid type in event config\n");
			return -1;
		}
	} else {
		syslog(LOG_ERR, "No type in event config\n");
		return -1;
	}

	opt = uci_lookup_option(uci, sec, "threshold");
	if (opt && opt->type == UCI_TYPE_STRING) {
		switch (event->threshold.type) {
			case THRESHOLD_TYPE_NUM:
				event->threshold.number = atof(opt->v.string);
				break;
			case THRESHOLD_TYPE_STR:
				strncpy(event->threshold.string, opt->v.string, CONFIG_STRLEN);
				break;
		}
	} else {
		syslog(LOG_ERR, "No threshold in event config\n");
		return -1;
	}

	opt = uci_lookup_option(uci, sec, "condition");
	if (opt && opt->type == UCI_TYPE_STRING) {
		if (!strcmp(opt->v.string, "=="))
			event->condition = CONDITION_EQ;
		else if (!strcmp(opt->v.string, "!="))
			event->condition = CONDITION_NE;
		else if (!strcmp(opt->v.string, ">"))
			event->condition = CONDITION_GT;
		else if (!strcmp(opt->v.string, "<"))
			event->condition = CONDITION_LT;
		else if (!strcmp(opt->v.string, ">="))
			event->condition = CONDITION_GE;
		else if (!strcmp(opt->v.string, "<="))
			event->condition = CONDITION_LE;
		else {
			syslog(LOG_ERR, "Not a valid condition in event config\n");
			return -1;
		}
	} else {
		syslog(LOG_ERR, "No condition in event config\n");
		return -1;
	}

	if (!event->email_list) {
		event->email_list = list_create();
		if (!event->email_list) {
			syslog(LOG_ERR, "Failed to create email list\n");
			return -1;
		}
	}

	struct uci_element *elm;

	opt = uci_lookup_option(uci, sec, "email");
	if (opt && opt->type == UCI_TYPE_LIST) {
		uci_foreach_element(&opt->v.list, elm) {   
			char *email = list_newnode(event->email_list, CONFIG_STRLEN);
			strncpy(email, elm->name, CONFIG_STRLEN);
		}
	} else {
		syslog(LOG_ERR, "No email in event config\n");
		return -1;
	}

	return 0;
}

static topic_t *find_topic(list_t *topic_list, char *name) {
	list_foreach(topic_list, t) {
		topic_t *topic = (topic_t *)t->data;
		if (!strcmp(topic->name, name))
			return topic;
	}
	return NULL;
}

static int parse_events(config_t *cfg,
		struct uci_context *uci, struct uci_package *pkg) {
	struct uci_option *opt;
	struct uci_element *elm;

	if (!cfg->topic_list) {
		syslog(LOG_ERR, "No topics, skipping events parsing\n");
		return -1;
	}

	uci_foreach_element(&pkg->sections, elm) {
		struct uci_section *sec = uci_to_section(elm);
		if (!strcmp(sec->type, "event")) {
			opt = uci_lookup_option(uci, sec, "topic");
			if (!(opt && opt->type == UCI_TYPE_STRING)) {
				return -1;
			}

			topic_t *topic = find_topic(cfg->topic_list, opt->v.string);
			if (topic) {
				event_t *event = list_newnode(topic->event_list, sizeof(event_t));
				if (!event || parse_event(event, uci, sec)) {
					return -1;
				}	
			} else {
				return -1;
			}
		}
	}

	return 0;
}

config_t *config_load(void) {
	config_t *cfg = (config_t *)malloc(sizeof(config_t));
	if (cfg == NULL)
		return NULL;

	memset(cfg, 0, sizeof(config_t));

	struct uci_context *uci = uci_alloc_context();
	if (uci == NULL) {
		free(cfg);
		return NULL;
	}

	struct uci_package *pkg;
	if (uci_load(uci, CONFIG_NAME, &pkg) || pkg == NULL) {
		uci_free_context(uci);
		free(cfg);
		return NULL;
	}

	apply_defaults(cfg);

	if (parse_smtp(cfg, uci, pkg)) {
		syslog(LOG_ERR, "Failed to parse smtp config\n");
		goto error;
	}

	if (parse_topics(cfg, uci, pkg)) {
		syslog(LOG_ERR, "Failed to parse topics config\n");
		goto error;
	}

	if (parse_events(cfg, uci, pkg)) {
		syslog(LOG_ERR, "Failed to parse events config\n");
		goto error;
	}

	uci_free_context(uci);
	return cfg;

error:
	uci_free_context(uci);
	free(cfg);
	return NULL;
}

void config_free(config_t *cfg) {
	list_foreach(cfg->topic_list, t) {
		topic_t *topic = (topic_t *)t->data;
		list_foreach(topic->event_list, e) {
			event_t *event = (event_t *)e->data;
			list_delete(event->email_list);
		}

		list_delete(topic->event_list);
	}

	list_delete(cfg->topic_list);
	free(cfg);
}

void config_dump(config_t *cfg) {
	printf("host=%s\n", cfg->host);
	printf("username=%s\n", cfg->username);
	printf("password=%s\n", cfg->password);
	printf("certfile=%s\n", cfg->certfile);
	printf("port=%d\n", cfg->port);

	printf("smtp.host=%s\n", cfg->smtp.host);
	printf("smtp.username=%s\n", cfg->smtp.username);
	printf("smtp.password=%s\n", cfg->smtp.password);
	printf("smtp.certfile=%s\n", cfg->smtp.certfile);

	int i = 0, j, k;
	list_foreach(cfg->topic_list, t) {
		topic_t *topic = (topic_t *)t->data;
		printf("[%d]topic.name=%s\n", i, topic->name);
		printf("[%d]topic.qos=%d\n", i, topic->qos);

		j = 0;
		list_foreach(topic->event_list, e) {
			event_t *event = (event_t *)e->data;
			printf("[%d]topic.[%d]event.param=%s\n", i, j, event->param);
			switch (event->threshold.type) {
				case THRESHOLD_TYPE_NUM:
					printf("[%d]topic.[%d]event.threshold=%f\n", i, j, event->threshold.number);
					break;
				case THRESHOLD_TYPE_STR:
					printf("[%d]topic.[%d]event.threshold='%s'\n", i, j, event->threshold.string);
					break;
			}
			printf("[%d]topic.[%d]event.type=%d\n", i, j, event->threshold.type);
			printf("[%d]topic.[%d]event.condition=%d\n", i, j, event->condition);

			k = 0;
			list_foreach(event->email_list, m) {
				char *email = (char *)m->data;
				printf("[%d]topic.[%d]event.[%d]email=%s\n", i, j, k, email);
			}
			j++;
		}
		i++;
	}
}
