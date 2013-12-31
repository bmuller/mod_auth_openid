namespace modauthopenid {
  using namespace std;

  /**
   * Convenience wrapper for an APR DBD connection.
   */
  class Dbd {
  public:
    /**
     * @param dbd A DBD connection usually provided by mod_dbd.
     */
    Dbd(const ap_dbd_t* dbd);

    /**
     * Wrapper for apr_dbd_query().
     * Execute one statement provided as a string of SQL.
     * Ignores result and row count.
     * Logs error message on failure.
     * @return true iff successful.
     */
    bool query(const char* sql) const;

    /**
     * Wrapper for apr_dbd_pbquery().
     * Execute one previously non-select statement, identified by label.
     * Ignores result and row count.
     * Logs error message on failure.
     * @return true iff successful.
     */
    bool pbquery(const char* label, const void** args) const;

    /**
     * Wrapper for apr_dbd_pvbselect().
     * Execute one previously prepared select statement, identified by label.
     * Assumes select statement results in at most one row.
     * Always allocates results and row.
     * Logs error message on failure.
     * @return true iff select is successful and result contains at least one row.
     */
    bool pbselect1(const char* label, apr_dbd_results_t** results, apr_dbd_row_t** row,
                   const void** args) const;

    /**
     * Get a string column from a result row.
     * Logs error message on failure.
     * @return true iff successful.
     */
    bool getcol_string(apr_dbd_row_t* row, int col, string& data) const;

    /**
     * Get an int64 column from a result row.
     * Logs error message on failure.
     * @return true iff successful.
     */
    bool getcol_int64(apr_dbd_row_t* row, int col, apr_int64_t& data) const;

    /**
     * Consume the unused part of a result set so that DBD can close it.
     * @param row Row storage must be allocated already.
     * @warning Don't call this on a result set that's already at its end.
     *          This results in a double free crash.
     */
    void close(apr_dbd_results_t* results, apr_dbd_row_t** row) const;

    /**
     * Print the contents of a table to stdout.
     * If there's an error, prints an error message to stdout instead of the table contents.
     */
    void print_table(const char* tablename) const;

    /**
     * Enable strict mode on the underlying connection for DBs that have it.
     * If not enabled, MySQL will silently truncate strings that are too long.
     * @return true iff successful, or if the DB is known not to have a strict mode.
     */
    bool enable_strict_mode() const;

    /**
     * Fetch a previously prepared statement by label.
     */
    apr_dbd_prepared_t* get_prepared(const char* label) const;

  private:
    /**
     * Actual DBD connection.
     */
    const ap_dbd_t* dbd;

    /**
     * If rc is a DBD error result code, write a debug log entry explaining it.
     * @param tag Extra error info.
     * @return true iff rc is a success code.
     */
    bool test_dbd(int rc, string& tag) const;

    /**
     * If rc is an APR error result code, write a debug log entry explaining it.
     * @return true iff rc is a success code.
     */
    bool test_apr(apr_status_t rc, string& tag) const;
  };
}
