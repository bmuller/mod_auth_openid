#include <stdio.h>
#include <stdlib.h>

#include <string>

#include "Dbd.h"
#include "MoidConsumer.h"
#include "SessionManager.h"
#include "cli_common.h"

using namespace std;
using namespace modauthopenid;

int main(int argc, const char* const* argv)
{
  apr_pool_t* pool = init_apr(&argc, &argv);

  // print usage message and quit if invoked incorrectly
  if (argc != 4) {
    fprintf(stderr, "usage: %s <DBD driver name> <DB params string as used by apr_dbd_open()> <create|drop|clear|print>\n", argv[0]);
    return EXIT_FAILURE;
  }

  const char *dbd_drivername = argv[1];
  const char *dbd_params     = argv[2];
  string      action          (argv[3]);

  // APR DBD connection
  ap_dbd_t* c_dbd = init_dbd(pool, dbd_drivername, dbd_params);
  // C++ connection wrapper
  Dbd dbd(c_dbd);

  // storage classes
  SessionManager s(dbd);
  MoidConsumer c(dbd);

  bool success = true;
  if        (action == "create") {
    printf("Creating tables: ");
    success &= s.create_tables();
    success &= c.create_tables();
    printf("%s.\n", success ? "successful" : "failed");
  } else if (action == "drop") {
    printf("Dropping tables: ");
    success &= s.drop_tables();
    success &= c.drop_tables();
    printf("%s.\n", success ? "successful" : "failed");
  } else if (action == "clear") {
    printf("Clearing tables: ");
    success &= s.clear_tables();
    success &= c.clear_tables();
    printf("%s.\n", success ? "successful" : "failed");
  } else if (action == "print") {
    success &= s.print_tables();
    success &= c.print_tables();
  } else {
    success = false;
    fprintf(stderr, "Unrecognized action: %s\n", action.c_str());
  }

  cleanup_dbd(c_dbd);

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
