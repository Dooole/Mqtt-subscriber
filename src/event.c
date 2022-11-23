#include <stdlib.h>
#include <stdio.h>

#include "event.h"

static int compare(event_t *event, double value) {
	fprintf(stderr, "comparing value %f with threshold %f\n",
		value, event->threshold);

	switch (event->condition) {
		case CONDITION_EQ:
			if (value == event->threshold) return 0;
			break;
		case CONDITION_GE:
			if (value >= event->threshold) return 0;
			break;
		case CONDITION_GT:
			if (value > event->threshold) return 0;
			break;
		case CONDITION_LT:
			if (value < event->threshold) return 0;
			break;
		case CONDITION_LE:
			if (value <= event->threshold) return 0;
			break;
		case CONDITION_NE:
			if (value != event->threshold) return 0;
			break;
	}

	return -1;
}

static int check_string_param(event_t *event, struct json_object *item) {
	if (json_object_get_type(item) != json_type_string) {
		fprintf(stderr, "Not a string!\n");
		return -1;
	}

	char *valstr = json_object_get_string(item);
	if (!valstr)
		return -1;

	double value = atof(valstr);
	return compare(event, value);
}

static int check_number_param(event_t *event, struct json_object *item) {
	if (json_object_get_type(item) != json_type_double && 
			json_object_get_type(item) != json_type_int) {
		fprintf(stderr, "Not a number!\n");
		return -1;
	}

	double value = json_object_get_double(item);
	return compare(event, value);
}

int event_check(event_t *event, struct json_object *msg) {
	int ret = -1;

	struct json_object *item = json_object_object_get(msg, event->param);
	if (item == NULL) {
		printf("No param: %s\n", event->param);
		return -1;
	}

	switch (event->type) {
		case PARAM_TYPE_NUM:
			ret = check_number_param(event, item);
			break;
		case PARAM_TYPE_STR:
			ret = check_string_param(event, item);
			break;
	}

	return ret;
}
