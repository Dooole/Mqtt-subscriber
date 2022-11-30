#ifndef SQLITEDB_H
#define SQLITEDB_H

#include <sqlite3.h>

#define SQLITEDB_PATH "/log/mqttsub.db"

int init_db(sqlite3 **db);
int write_to_db(sqlite3 *db, char *topic, char *msg);
void close_db(sqlite3 *db);

#endif 