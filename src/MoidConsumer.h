/*
Copyright (C) 2007-2010 Butterfat, LLC (http://butterfat.net)

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Created by bmuller <bmuller@butterfat.net>
*/

#pragma once

#include <opkele/types.h>
#include <opkele/association.h>
#include <opkele/prequeue_rp.h>

#include "Dbd.h"

namespace modauthopenid {
  using namespace opkele;
  using namespace std;

  /**
   * Class to handle tasks of consumer.
   * Authentication session is based on a nonce id that is thrown in with the params list.
   */
  class MoidConsumer : public prequeue_RP {
  public:
    /**
     * @param _dbd A DBD connection wrapper.
     * @param _asnonceid The association session nonce.
     * @param _serverurl The return-to value (URL initially requested by user).
     */
    MoidConsumer(Dbd& _dbd, const string& _asnonceid, const string& _serverurl);

    /**
     * @param _dbd A DBD connection wrapper.
     * Use this constructor for table maintenance not associated
     * with an actual OpenID interaction.
     */
    MoidConsumer(Dbd& _dbd);

    /**
     * Create all tables used by this class.
     * @attention Will only try to create tables that don't already exist,
                  and thus cannot update schemas if they change.
     * @return True iff successful.
     */
    bool create_tables();

    /**
     * Drop any tables used by this class.
     * @return True iff successful.
     */
    bool drop_tables();

    /**
     * Delete all rows from tables used by this class.
     * @return True iff successful.
     */
    bool clear_tables();

    /**
     * Store a new assocation.
     * @param now Optional time parameter for tests. If omitted, current time will be used.
     */
    assoc_t store_assoc(const string& server, const string& handle, const string& assoc_type,
                        const secret_t& secret, int expires_in);
    assoc_t store_assoc(const string& server, const string& handle, const string& assoc_type,
                        const secret_t& secret, int expires_in, time_t now);

    /**
     * Retrieve an association.
     * @param now Optional time parameter for tests. If omitted, current time will be used.
     */
    assoc_t retrieve_assoc(const string& server, const string& handle);
    assoc_t retrieve_assoc(const string& server, const string& handle, time_t now);

    /**
     * Invalidate assocation by deleting it from the DB.
     */
    void invalidate_assoc(const string& server, const string& handle);

    /**
     * Find any association for the given server OP.
     * @param now Optional time parameter for tests. If omitted, current time will be used.
     */
    assoc_t find_assoc(const string& server);
    assoc_t find_assoc(const string& server, time_t now);

    /**
     * This is called to store the openid.response_nonce.
     * If it isn't already in the DB, stores the nonce for the same amount of time as the association,
     * after which point a replay attack is impossible since the assocation key is deleted.
     * @throws opkele::id_res_bad_nonce if a previously used nonce is reused.
     */
    void check_nonce(const string& server, const string& nonce);
    void check_nonce(const string& server, const string& nonce, time_t now);

    /**
     * Delete authentication session with the constructor param nonce if it exists.
     */
    void begin_queueing();

    /**
     * Store the given endpoint.
     * There actually is no queue: we just keep track of the first one given,
     * and all subsequent calls are ignored.
     * @param now Optional time parameter for tests. If omitted, current time will be used.
     */
    void queue_endpoint(const openid_endpoint_t& ep);
    void queue_endpoint(const openid_endpoint_t& ep, time_t now);

    // get the endpoint set in queue_endpoint
    const openid_endpoint_t& get_endpoint() const;

    /**
     * Same as begin_queuing.
     * @see begin_queuing
     */
    void next_endpoint();

    /**
     * Set the normalized id for this authentication session.
     */
    void set_normalized_id(const string& nid);

    /**
     * Get the normalized id for this authentication session.
     */
    const string get_normalized_id() const;

    /**
     * Get the url that was given as a constructor parameter.
     */
    const string get_this_url() const;
    
    /**
     * @return Whether a session exists with the nonce session id
     *         given in the constructor.
     */
    bool session_exists();

    /**
     * Print all tables used by this class to stdout.
     * This will only be called from command-line utilities, not the Apache module itself.
     * @return True iff successful.
     */
    bool print_tables();

    /**
     * Delete session with  nonce id given in constructor param list.
     */
    void kill_session();

    /**
     * Append names and SQL for prepared statements used by this class to the provided array.
     * @param statements APR array of labeled_statement_t.
     */
    static void append_statements(apr_array_header_t *statements);

    /**
     * Delete all expired sessions.
     */
    void delete_expired(time_t now);

  private:
    /**
     * Database connection wrapper.
     */
    Dbd& dbd;

    /**
     * The nonce-based authentication session.
     */
    string asnonceid;

    /**
     * The return-to value (URL initially requested by user).
     */
    string serverurl; 

    /**
     * Has any endpoint has been set yet?
     */
    bool endpoint_set;

    /**
     * The normalized id the user has attempted to use.
     */
    mutable string normalized_id;

    /**
     * The endpoint for the user's identity.
     */
    mutable openid_endpoint_t endpoint;

    /**
     * Load association object from associations row, and close result set.
     */
    assoc_t load_assoc(apr_dbd_results_t* results, apr_dbd_row_t** row);
  };
}
