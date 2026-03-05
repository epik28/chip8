#ifndef DB_H
#define DB_H

#include <sqlite3.h>

sqlite3* init_db(const char *db_name);
char* get_rom_path(sqlite3 *db, int id);
int add_game(sqlite3 *db, const char *name, const char *path, const char *author, int year, const char *description);
void close_db(sqlite3 *db);
void populate_default_games(sqlite3 *db);

#endif