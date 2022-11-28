#ifndef CONFIG_H
#define CONFIG_H

#include "list.h"
#include "db.h"

#define CONFIG_NAME "mqttsub"
#define CONFIG_PATH "/etc/config/mqttsub"
#define CONFIG_STRLEN 64

#define PARAM_TYPE_NUM 0
#define PARAM_TYPE_STR 1

#define QOS_ATMOST_ONCE 0
#define QOS_ATLEAST_ONCE 1
#define QOS_EXACTLY_ONCE 2

#define CONDITION_EQ 0 /* == */
#define CONDITION_GE 1 /* >= */
#define CONDITION_GT 2 /* > */
#define CONDITION_LT 3 /* < */
#define CONDITION_LE 4 /* <= */
#define CONDITION_NE 5 /* != */

typedef struct event_config {
	char param[CONFIG_STRLEN];
	int type;
	int condition;
	double threshold;
	list_t *email_list;
} event_t;

typedef struct topic_config {
	char name[CONFIG_STRLEN];
	list_t *event_list;
	int qos;
} topic_t;

typedef struct smtp_config {
	char host[CONFIG_STRLEN];
	char username[CONFIG_STRLEN];
	char password[CONFIG_STRLEN];
	char certfile[CONFIG_STRLEN];
} smtp_t;

typedef struct mqttsub_config {
	char host[CONFIG_STRLEN];
	char username[CONFIG_STRLEN];
	char password[CONFIG_STRLEN];
	char certfile[CONFIG_STRLEN];
	int port;
	smtp_t smtp;
	list_t *topic_list;
	sqlite3 *db;
} config_t;

config_t *config_load(void);
void config_dump(config_t *cfg);
void config_free(config_t *cfg);

#endif
