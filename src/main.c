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

struct mqttsub_opts {
    int debug;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    
    struct mqttsub_opts *opts = state->input;

    switch (key)
    {
        case 'd': 
            opts->debug = 1;
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
    printf("New message with topic %s: %s\n", msg->topic, (char *)msg->payload);

    char text[256];
    topic_t *found_topic = NULL;

    config_t *cfg = (config_t *)obj;
    if (write_to_db(cfg->db, msg->topic, (char *)msg->payload)) {
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
        fprintf(stderr, "Received unconfigured topic '%s'\n", msg->topic);
        return;
    } else {
        fprintf(stderr, "Processing JSON on topic '%s'\n", msg->topic);
    }

    struct json_object *root = json_tokener_parse((char *)msg->payload);
    if (!root) {
        fprintf(stderr, "Failed to parse json\n");
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
    struct mqttsub_opts opts;

    struct argp_option options[] = 
    {
        {"debug",    'd', 0,    0, "Debug mode"},
        {0}
    };
    struct argp argp = {options, parse_opt};

    if (argp_parse(&argp, argc, argv, 0, 0, &opts)) {
        syslog(LOG_ERR, "Failed to parse command line arguments\n");
        exit(EXIT_FAILURE);
    }

    setlogmask(LOG_UPTO(LOG_NOTICE));
    openlog("mqttsub", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

    if (opts.debug)
        syslog(LOG_DEBUG, "Started in debug mode!\n");

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    config_t *cfg = config_load();
    if (cfg == NULL) {
        fprintf(stderr, "Failed to load config\n");
        exit(EXIT_FAILURE);
    }

    config_dump(cfg);

    if (init_db(&cfg->db)) {
        syslog(LOG_ERR, "Failed to init db\n");
        exit(EXIT_FAILURE);
    }

    struct mosquitto *mosq = broker_init(&cfg->broker, message_cb, cfg);
    if (!mosq) {
        fprintf(stderr, "Failed to init broker\n");
        exit(EXIT_FAILURE);
    }

    if (broker_subscribe(mosq, cfg->topic_list)) {
        fprintf(stderr, "Failed to subscribe\n");
        exit(EXIT_FAILURE);
    }

    while (!interrupted) {
        broker_step(mosq);
    }

    close_db(cfg->db);
    broker_cleanup(mosq);
    config_free(cfg);
    closelog();

    exit(EXIT_SUCCESS);
}
