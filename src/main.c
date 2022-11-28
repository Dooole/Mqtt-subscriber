#include <stdio.h>
#include <signal.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <argp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include <json-c/json.h>

#include "list.h"
#include "config.h"
#include "broker.h"
#include "event.h"
#include "notify.h"
#include "db.h"

volatile static int interrupted = 0;

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    
    config_t *cfg = state->input;

    switch (key)
    {
        case 'a': 
            strncpy(cfg->host, arg, CONFIG_STRLEN);
            break;
        case 'p': 
            cfg->port = atoi(arg);
            break;
        case 'u': 
            strncpy(cfg->username, arg, CONFIG_STRLEN);
            break;
        case 'w': 
            strncpy(cfg->password, arg, CONFIG_STRLEN);
            break;
        case 'c': 
            strncpy(cfg->certfile, arg, CONFIG_STRLEN);
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static void signal_handler(int signo) {
    syslog(LOG_WARNING, "Received signal: %d, stopping\n", signo);
    interrupted = 1;
}

static void message_cb(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    syslog(LOG_NOTICE,"New message with topic %s: %s\n", msg->topic, (char *)msg->payload);

    char text[256];
    topic_t *found_topic = NULL;

    config_t *cfg = (config_t *)obj;
    if (write_to_db(cfg->db, msg->topic, (char *)msg->payload)) {
        syslog(LOG_ERR, "Failed to write message to db\n");
        return;
    }

    list_foreach(cfg->topic_list, t) {
        topic_t *topic = (topic_t *)t->data;
        if (!strncmp(topic->name, msg->topic, CONFIG_STRLEN)) {
            found_topic = topic;
            break;
        }
    }

    if (found_topic == NULL) {
        syslog(LOG_ERR, "Received unconfigured topic '%s'\n", msg->topic);
        return;
    } else {
        syslog(LOG_WARNING, "Processing JSON on topic '%s'\n", msg->topic);
    }

    struct json_object *root = json_tokener_parse((char *)msg->payload);
    if (!root) {
        syslog(LOG_ERR, "Failed to parse json\n");
        return;
    }

    list_foreach(found_topic->event_list, e) {
        event_t *event = (event_t *)e->data;
        if (!event_check(event, root)) {
            sprintf(text, "%s: %s: threshold event",
                found_topic->name, event->param);
            notify_recipients(&cfg->smtp, event->email_list, text);
        }
    }

    json_object_put(root);
}

int main(int argc, char *argv[])
{
    struct argp_option options[] =
    {
        {"host",     'a', "host",     0,                   "IPv4 address"},
        {"port",     'p', "port",     OPTION_ARG_OPTIONAL, "Port number"},
        {"username", 'u', "username", 0,                   "Username"},
        {"password", 'w', "password", 0,                   "Password"},
        {"certfile", 'c', "certfile", OPTION_ARG_OPTIONAL, "CA Certificate"},
        {0}
    };
    struct argp argp = {options, parse_opt};

    setlogmask(LOG_UPTO(LOG_NOTICE));
    openlog("mqttsub", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

    config_t *cfg = config_load();
    if (cfg == NULL) {
        syslog(LOG_ERR, "Failed to load config\n");

        closelog();

        exit(EXIT_FAILURE);
    }

    if (argp_parse(&argp, argc, argv, 0, 0, cfg)) {
        syslog(LOG_ERR, "Failed to parse command line arguments\n");
        
        config_free(cfg);
        closelog();

        exit(EXIT_FAILURE);
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (init_db(&cfg->db)) {
        syslog(LOG_ERR, "Failed to init db\n");

        config_free(cfg);
        closelog();

        exit(EXIT_FAILURE);
    }

    struct mosquitto *mosq = broker_init(cfg, message_cb, cfg);
    if (!mosq) {
        syslog(LOG_ERR, "Failed to init broker\n");

        close_db(cfg->db);
        config_free(cfg);
        closelog();

        exit(EXIT_FAILURE);
    }

    if (broker_subscribe(mosq, cfg->topic_list)) {
        syslog(LOG_ERR, "Failed to subscribe\n");

        broker_cleanup(mosq);
        close_db(cfg->db);
        config_free(cfg);
        closelog();

        exit(EXIT_FAILURE);
    }

    while (!interrupted) {
        broker_step(mosq);
    }

    broker_cleanup(mosq);
    close_db(cfg->db);
    config_free(cfg);
    closelog();

    exit(EXIT_SUCCESS);
}
