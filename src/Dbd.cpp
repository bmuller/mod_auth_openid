#include "mod_auth_openid.h"

namespace modauthopenid {
  using namespace std;

  Dbd::Dbd(const ap_dbd_t* dbd)
  {
    this->dbd = dbd;
  }

  bool Dbd::query(const char* sql)
  {
    int n_rows; // rows affected by query: not used here, but can't be NULL
    int rc = apr_dbd_query(dbd->driver, dbd->handle, &n_rows, sql);
    string tag("String query failed: " + string(sql));
    return test_dbd(rc, tag);
  }

  bool Dbd::pbquery(const char* label, const void** args)
  {
    apr_dbd_prepared_t *statement = get_prepared(label);
    if (statement == NULL) {
      fprintf(stderr, "Couldn't get prepared statement %s\n", label);
      return false;
    }

    int n_rows; // ignored
    int rc = apr_dbd_pvbquery(dbd->driver, dbd->pool, dbd->handle,
                              &n_rows, statement, args);
    string tag("Prepared query failed: " + string(label));
    return test_dbd(rc, tag);
  }

  bool Dbd::pbselect1(const char* label, apr_dbd_results_t** results, apr_dbd_row_t** row,
                      const void** args)
  {
    apr_dbd_prepared_t *statement = get_prepared(label);
    if (statement == NULL) {
      fprintf(stderr, "Couldn't get prepared statement %s\n", label);
      return false;
    }

    *results = NULL;
    int rc = apr_dbd_pvbselect(dbd->driver, dbd->pool, dbd->handle,
                               results, statement, DBD_LINEAR_ACCESS, args);
    string tag_select_failed("Prepared select failed: " + string(label));
    if (!test_dbd(rc, tag_select_failed)) {
      return false;
    }

    *row = NULL;
    rc = apr_dbd_get_row(dbd->driver, dbd->pool, *results, row, DBD_NEXT_ROW);
    string tag_no_rows("Prepared select returned zero rows: " + string(label));
    return test_dbd(rc, tag_no_rows);
  }

  bool Dbd::getcol_string(apr_dbd_row_t* row, int col, string& data)
  {
    const char* data_raw = apr_dbd_get_entry(dbd->driver, row, col);
    if (data_raw == NULL) {
      fprintf(stderr, "Couldn't fetch column %d as string\n", col);
      return false;
    }
    data = data_raw;
    return true;
  }

  bool Dbd::getcol_int64(apr_dbd_row_t* row, int col, apr_int64_t& data)
  {
    int rc = apr_dbd_datum_get(dbd->driver, row, col, APR_DBD_TYPE_LONGLONG, (void*)data);
    string tag("Couldn't fetch column as int64");
    return test_apr(rc, tag);
  }

  void Dbd::close(apr_dbd_results_t* results, apr_dbd_row_t** row)
  {
    while (apr_dbd_get_row(dbd->driver, dbd->pool, results, row, DBD_NEXT_ROW) != DBD_NO_MORE_ROWS) {
      // do nothing
    }
  }

  void Dbd::print_table(const char* tablename)
  {
    printf("Printing table: %s.\n", tablename);
    string sql("SELECT * FROM " + string(tablename));
    apr_dbd_results_t *results = NULL;
    int rc = apr_dbd_select(dbd->driver, dbd->pool, dbd->handle, &results, sql.c_str(), DBD_LINEAR_ACCESS);
    if (rc != DBD_SUCCESS) {
      printf("Couldn't fetch contents. DBD error %d: %s\n\n",
             rc, apr_dbd_error(dbd->driver, dbd->handle, rc));
      return;
    }

    int n_cols = apr_dbd_num_cols(dbd->driver, results);
    for (int col = 0; col < n_cols; col++) {
      const char *name = apr_dbd_get_name(dbd->driver, results, col);
      printf("%s\t", name);
    }
    printf("\n");

    apr_dbd_row_t *row = NULL;
    // apr_dbd_num_tuples() row count function doesn't work with "async"/linear row access mode
    int n_rows = 0;
    while (apr_dbd_get_row(dbd->driver, dbd->pool, results, &row, DBD_NEXT_ROW) != DBD_NO_MORE_ROWS) {
      n_rows++;
      for (int col = 0; col < n_cols; col++) {
        const char *value = apr_dbd_get_entry(dbd->driver, row, col);
        printf("%s\t", value);
      }
      printf("\n");
    }

    printf("There are %d rows.\n\n", n_rows);
  }

  bool Dbd::test_dbd(int rc, string& tag)
  {
    if (rc == DBD_SUCCESS) {
      return true;
    }
    const char *err_buf = apr_dbd_error(dbd->driver, dbd->handle, rc);
    fprintf(stderr, "%s: DBD error %d: %s\n", tag.c_str(), rc, err_buf);
    return false;
  }

  bool Dbd::test_apr(apr_status_t rc, string& tag)
  {
    if (rc == APR_SUCCESS) {
      return true;
    }
    char err_buf[256]; // big enough for most messages
    apr_strerror(rc, err_buf, sizeof(err_buf));
    fprintf(stderr, "%s: APR status %d: %s\n", tag.c_str(), rc, err_buf);
    return false;
  }

  apr_dbd_prepared_t* Dbd::get_prepared(const char* label)
  {
    return (apr_dbd_prepared_t *)apr_hash_get(dbd->prepared, label, APR_HASH_KEY_STRING);
  }
}
