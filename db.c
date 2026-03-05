#include "db.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

sqlite3* init_db(const char *db_name) {
    sqlite3 *db;
    int rc = sqlite3_open(db_name, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return NULL;
    }
    
    const char *sql_create = 
        "CREATE TABLE IF NOT EXISTS games ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "path TEXT NOT NULL,"
        "author TEXT,"
        "year INTEGER,"
        "description TEXT"
        ");";
    
    char *errmsg = NULL;
    rc = sqlite3_exec(db, sql_create, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        return NULL;
    }
    
    return db;
}

char* get_rom_path(sqlite3 *db, int id) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT path FROM games WHERE id = ?;";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return NULL;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    char *path = NULL;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *text = (const char*)sqlite3_column_text(stmt, 0);
        if (text) {
            path = malloc(strlen(text) + 1);
            strcpy(path, text);
        }
    }
    
    sqlite3_finalize(stmt);
    return path;
}

int add_game(sqlite3 *db, const char *name, const char *path, const char *author, int year, const char *description) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO games (name, path, author, year, description) VALUES (?, ?, ?, ?, ?);";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Prepare error: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, path, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, author, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, year);
    sqlite3_bind_text(stmt, 5, description, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? 0 : -1;
}

void close_db(sqlite3 *db) {
    if (db) sqlite3_close(db);
}

void populate_default_games(sqlite3 *db) {
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM games;", -1, &stmt, NULL);
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    
    if (count == 0) {
        add_game(db, "Pong", "roms/pong.ch8", "Nolan Bushnell", 1972, "Classic Pong");
        add_game(db, "Space Invaders", "roms/invaders.ch8", "Tomohiro Nishikado", 1978, "Space Invaders");
        add_game(db, "Tetris", "roms/tetris.ch8", "Alexey Pajitnov", 1984, "Tetris");
        add_game(db, "Brix", "roms/brix.ch8", "Andreas Gustafsson", 0, "Breakout clone");
        add_game(db, "IBM", "roms/ibm.ch8", "Unknown", 1981, "IBM logo demo");
    }
}