#include "apr_errno.h"
#include "apr_pools.h"
#include "apr_dbd.h"
#include "mod_dbd.h"

/**
 * Print an error message and quit if an APR call fails.
 */
void exit_on_apr_err(apr_status_t rc, const char* tag);

/**
 * Initialize APR and create a memory pool.
 * @attention Side effects modify argc/argv.
 */
apr_pool_t* init_apr(int* argc, const char* const** argv);

/**
 * Create a DBD connection from a pool and DB connection params.
 */
ap_dbd_t* init_dbd(apr_pool_t* pool, const char* dbd_drivername, const char* dbd_params);

/**
 * Close connection before we exit.
 */
void cleanup_dbd(const ap_dbd_t* dbd);

/**
 * Try to prepare the SQL statements used by our classes.
 * @attention Adds successfully prepared statements to c_dbd->prepared hashtable.
 */
void prepare_statements(const ap_dbd_t* c_dbd);
