#include "test.h"

class TestPreparedStatements : public TestFixture
{
protected:
  apr_pool_t* pool;
  Dbd* dbd;

public:
  void setUp()
  {
    pool = g_pool;
    dbd = g_dbd;
  }

  /**
   * Confirm that all statements were prepared during test setup.
   */
  void testPreparedStatements()
  {
    // collect statements
    apr_array_header_t *statements = apr_array_make(
      pool,
      18 /* size hint: expected number of statements */,
      sizeof(labeled_statement_t));
    MoidConsumer  ::append_statements(statements);
    SessionManager::append_statements(statements);

    // try to retrieve statements
    for (int i = 0; i < statements->nelts; i++) {
      labeled_statement_t *statement;
      apr_dbd_prepared_t *prepared_statement;
      statement = &APR_ARRAY_IDX(statements, i, labeled_statement_t);
      prepared_statement = dbd->get_prepared(statement->label);
      CPPUNIT_ASSERT_MESSAGE(
        ("Failed to prepare statement: " + string(statement->label)).c_str(),
        prepared_statement != NULL);
    }
  }

  CPPUNIT_TEST_SUITE(TestPreparedStatements);
  CPPUNIT_TEST(testPreparedStatements);
  CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestPreparedStatements);
