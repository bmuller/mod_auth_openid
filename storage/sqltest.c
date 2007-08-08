#include <stdio.h>
#include <sqlite3.h>

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
  int i;
  for(i=0; i<argc; i++){
    printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  printf("\n");
  return 0;
}

int main(int argc, char **argv){
  sqlite3 *db;
  char *zErrMsg = 0;
  int rc;

  if( argc!=3 ){
    fprintf(stderr, "Usage: %s DATABASE SQL-STATEMENT\n", argv[0]);
    return 1;
  }
  rc = sqlite3_open(argv[1], &db);
  if( rc ){
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;
  }
  //rc = sqlite3_exec(db, argv[2], callback, 0, &zErrMsg);
  sqlite3_stmt *pSelect;
  rc = sqlite3_prepare(db, argv[2], -1, &pSelect, 0);
  if( rc!=SQLITE_OK || !pSelect ){
    return rc;
  }
  rc = sqlite3_step(pSelect);
  while( rc==SQLITE_ROW ){
    fprintf(stdout, "%s;\n", sqlite3_column_text(pSelect, 0));
    fprintf(stdout, "%s;\n", sqlite3_column_text(pSelect, 1));
    rc = sqlite3_step(pSelect);
  }
  rc = sqlite3_finalize(pSelect);
  sqlite3_close(db);
  return 0;
}
