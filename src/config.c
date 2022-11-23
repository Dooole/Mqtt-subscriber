#include <stdlib.h>
#include <string.h>
#include <uci.h>

#include "config.h"

static int parse_broker(config_t *cfg,
		struct uci_context *uci, struct uci_package *pkg) {
	struct uci_option *opt;
	struct uci_element *elm;

	uci_foreach_element(&pkg->sections, elm) {
		struct uci_section *sec = uci_to_section(elm);
		if (!strcmp(sec->type, "broker")) {
			opt = uci_lookup_option(uci, sec, "host");
			if (opt && opt->type == UCI_TYPE_STRING)
				strncpy(cfg->broker.host, opt->v.string, CONFIG_STRLEN);
			else
				return -1;

			opt = uci_lookup_option(uci, sec, "username");
			if (opt && opt->type == UCI_TYPE_STRING)
				strncpy(cfg->broker.username, opt->v.string, CONFIG_STRLEN);
			else
				return -1;

			opt = uci_lookup_option(uci, sec, "password");
			if (opt && opt->type == UCI_TYPE_STRING)
				strncpy(cfg->broker.password, opt->v.string, CONFIG_STRLEN);
			else
				return -1;

			opt = uci_lookup_option(uci, sec, "certfile");
			if (opt && opt->type == UCI_TYPE_STRING)
				strncpy(cfg->broker.certfile, opt->v.string, CONFIG_STRLEN);
			else
				return -1;

			opt = uci_lookup_option(uci, sec, "port");
			if (opt && opt->type == UCI_TYPE_STRING) {
				cfg->broker.port = atoi(opt->v.string);
				if (!cfg->broker.port)
					return -1;
			 } else
				return -1;

			break;
		}
	}

	return 0;
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
			else
				return -1;

			opt = uci_lookup_option(uci, sec, "username");
			if (opt && opt->type == UCI_TYPE_STRING)
				strncpy(cfg->smtp.username, opt->v.string, CONFIG_STRLEN);
			else
				return -1;

			opt = uci_lookup_option(uci, sec, "password");
			if (opt && opt->type == UCI_TYPE_STRING)
				strncpy(cfg->smtp.password, opt->v.string, CONFIG_STRLEN);
			else
				return -1;

			opt = uci_lookup_option(uci, sec, "certfile");
			if (opt && opt->type == UCI_TYPE_STRING)
				strncpy(cfg->smtp.certfile, opt->v.string, CONFIG_STRLEN);
			else
				return -1;

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
		if (!cfg->topic_list)
			return -1;
	}

	uci_foreach_element(&pkg->sections, elm) {
		struct uci_section *sec = uci_to_section(elm);
		if (!strcmp(sec->type, "topic")) {
			opt_name = uci_lookup_option(uci, sec, "name");
			if (!(opt_name && opt_name->type == UCI_TYPE_STRING))
				return -1;

			opt_qos = uci_lookup_option(uci, sec, "qos");
			if (!(opt_qos && opt_qos->type == UCI_TYPE_STRING))
				return -1;

			topic_t *topic = list_newnode(cfg->topic_list, sizeof(topic_t));
			strncpy(topic->name, opt_name->v.string, CONFIG_STRLEN);
			topic->qos = atoi(opt_qos->v.string);

			topic->event_list = list_create();
			if (!topic->event_list)
				return -1;
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
	}

	opt = uci_lookup_option(uci, sec, "threshold");
	if (opt && opt->type == UCI_TYPE_STRING) {
		event->threshold = atof(opt->v.string);
	}

	opt = uci_lookup_option(uci, sec, "type");
	if (opt && opt->type == UCI_TYPE_STRING) {
		if (!strcmp(opt->v.string, "number"))
			event->type = PARAM_TYPE_NUM;
		else if (!strcmp(opt->v.string, "string"))
			event->type = PARAM_TYPE_STR;
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
	}

	if (!event->email_list) {
		event->email_list = list_create();
		if (!event->email_list)
			return -1;
	}

	struct uci_element *elm;

	opt = uci_lookup_option(uci, sec, "email");
	if (opt && opt->type == UCI_TYPE_LIST) {
		uci_foreach_element(&opt->v.list, elm) {   
			char *email = list_newnode(event->email_list, CONFIG_STRLEN);
			strncpy(email, elm->name, CONFIG_STRLEN);
		}
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

	if (!cfg->topic_list)
		return -1;

	uci_foreach_element(&pkg->sections, elm) {
		struct uci_section *sec = uci_to_section(elm);
		if (!strcmp(sec->type, "event")) {
			opt = uci_lookup_option(uci, sec, "topic");
			if (!(opt && opt->type == UCI_TYPE_STRING))
				break;

			topic_t *topic = find_topic(cfg->topic_list, opt->v.string);
			if (topic) {
				event_t *event = list_newnode(topic->event_list, sizeof(event_t));
				if (!event)
					break;
				parse_event(event, uci, sec);
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

	if (parse_broker(cfg, uci, pkg))
		goto error;

	if (parse_smtp(cfg, uci, pkg))
		goto error;

	if (parse_topics(cfg, uci, pkg))
		goto error;

	if (parse_events(cfg, uci, pkg))
		goto error;

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
	printf("broker.host=%s\n", cfg->broker.host);
	printf("broker.username=%s\n", cfg->broker.username);
	printf("broker.password=%s\n", cfg->broker.password);
	printf("broker.certfile=%s\n", cfg->broker.certfile);
	printf("broker.port=%d\n", cfg->broker.port);

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
			printf("[%d]topic.[%d]event.threshold=%f\n", i, j, event->threshold);
			printf("[%d]topic.[%d]event.type=%d\n", i, j, event->type);
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
