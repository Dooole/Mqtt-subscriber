#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <syslog.h>

#include "db.h"

int init_db(sqlite3 **db) {

    char *err_msg = NULL;
 	char query[128];
 	int rc;

    rc = sqlite3_open(SQLITEDB_PATH, db);
    if (rc != SQLITE_OK) {
        syslog(LOG_ERR, "Failed to open database: %s\n", sqlite3_errmsg(*db));
        return -1;
    }

    sprintf(query, "CREATE TABLE IF NOT EXISTS MqttSubData(Timestamp INT, Topic TEXT, Message TEXT);");

    rc = sqlite3_exec(*db, query, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
    	sqlite3_free(err_msg); 
        syslog(LOG_ERR, "SQL error: %s\n", err_msg);      
        return -1;
    }
    return 0;
}

int write_to_db(sqlite3 *db, char *topic, char *msg) {

	sqlite3_stmt *res;
    char query[128];
    int rc = 0;

    sprintf(query, "INSERT INTO MqttSubData VALUES(?, ?, ?);");

    rc = sqlite3_prepare_v2(db, query, -1, &res, 0);
    if (rc != SQLITE_OK) {
		syslog(LOG_ERR, "SQL error: %s\n", sqlite3_errmsg(db));
		return -1;
	}

	sqlite3_bind_int(res, 1, (int)time(NULL));
	sqlite3_bind_text(res, 2, topic, -1, NULL);
	sqlite3_bind_text(res, 3, msg, -1, NULL);

	if (sqlite3_step(res) != SQLITE_DONE){
		sqlite3_finalize(res);
		syslog(LOG_ERR, "SQL error: %s\n", sqlite3_errmsg(db));
		return -1;
	}
	sqlite3_finalize(res);
	return 0;
}

static void print_time(int value) {
	struct tm ts;
	char timestr;

	time_t time = (time_t)value;
	ts = *localtime(&time);

	strftime(&timestr, sizeof(timestr), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
	printf("[%d]", timestr);
}


void close_db(sqlite3 *db) {

	if (db == NULL){
	syslog(LOG_ERR, "Not initialized\n");
	return;
	}
	sqlite3_close(db);
}