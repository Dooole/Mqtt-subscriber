#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>

#include "event.h"

static int compare_number(event_t *event, double value) {
	syslog(LOG_NOTICE, "Comparing value %f with threshold %f\n",
		value, event->threshold.number);

	switch (event->condition) {
		case CONDITION_EQ:
			if (value == event->threshold.number) return 0;
			break;
		case CONDITION_GE:
			if (value >= event->threshold.number) return 0;
			break;
		case CONDITION_GT:
			if (value > event->threshold.number) return 0;
			break;
		case CONDITION_LT:
			if (value < event->threshold.number) return 0;
			break;
		case CONDITION_LE:
			if (value <= event->threshold.number) return 0;
			break;
		case CONDITION_NE:
			if (value != event->threshold.number) return 0;
			break;
	}

	return -1;
}

static int compare_string(event_t *event, const char *value) {
	syslog(LOG_NOTICE, "Comparing value '%s' with threshold '%s'\n",
		value, event->threshold.string);

	switch (event->condition) {
		case CONDITION_EQ:
			if (!strcmp(value, event->threshold.string)) return 0;
			break;
		case CONDITION_NE:
			if (strcmp(value, event->threshold.string)) return 0;
			break;
	}

	return -1;
}

static int check_string_param(event_t *event, struct json_object *item) {
	if (json_object_get_type(item) != json_type_string) {
		syslog(LOG_ERR, "Unexpected JSON data type, must be string\n");
		return -1;
	}

	const char *valstr = json_object_get_string(item);
	if (!valstr) {
		syslog(LOG_ERR, "Failed to get json string!\n");
		return -1;
	}

	return compare_string(event, valstr);
}

static int check_number_param(event_t *event, struct json_object *item) {
	if (json_object_get_type(item) != json_type_double && 
			json_object_get_type(item) != json_type_int) {
		syslog(LOG_ERR, "Unexpected JSON data type, must be number\n");
		return -1;
	}

	double valnum = json_object_get_double(item);
	return compare_number(event, valnum);
}

int event_check(event_t *event, struct json_object *msg) {
	int ret = -1;

	struct json_object *item = json_object_object_get(msg, event->param);
	if (item == NULL) {
		syslog(LOG_ERR, "No param: %s\n", event->param);
		return -1;
	}

	switch (event->threshold.type) {
		case THRESHOLD_TYPE_NUM:
			ret = check_number_param(event, item);
			break;
		case THRESHOLD_TYPE_STR:
			ret = check_string_param(event, item);
			break;
	}

	return ret;
}
