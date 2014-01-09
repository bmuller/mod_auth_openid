#include "test.h"

class TestMoidConsumer : public TestFixture
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
   * Test OpenID association store/load.
   */
  void testAssociations()
  {
    MoidConsumer c(*dbd);

    // store a fake association
    time_t now = 1386810000; // no religious or practical significance
    int lifespan = 50;
    string stored_server("https://example.org/accounts");
    string stored_handle("a.handle");
    string stored_assoc_type("HMAC-SHA256");
    opkele::secret_t stored_secret;
    stored_secret.from_base64("QUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFB"); // 'A' repeated 30 times
    int stored_expires_in = lifespan;
    // TODO: opkele::prequeue_RP interface doesn't have a way to check if storage succeeded
    opkele::assoc_t stored_assoc = c.store_assoc(
      stored_server,
      stored_handle,
      stored_assoc_type,
      stored_secret,
      stored_expires_in,
      now);

    // load the fake association by server + handle
    opkele::assoc_t loaded_assoc;
    CPPUNIT_ASSERT_NO_THROW_MESSAGE(
      "Exception on loading association",
      loaded_assoc = c.retrieve_assoc(stored_server, stored_handle, now));

    // compare to the stored version
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
      "Field not equal between stored and loaded association: server",
      stored_assoc->server(),
      loaded_assoc->server());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
      "Field not equal between stored and loaded association: handle",
      stored_assoc->handle(),
      loaded_assoc->handle());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
      "Field not equal between stored and loaded association: assoc_type",
      stored_assoc->assoc_type(),
      loaded_assoc->assoc_type());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
      "Field not equal between stored and loaded association: secret",
      stored_assoc->secret(),
      loaded_assoc->secret());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
      "Field not equal between stored and loaded association: expires_in",
      stored_assoc->expires_in(),
      loaded_assoc->expires_in());
  }

  CPPUNIT_TEST_SUITE(TestMoidConsumer);
  CPPUNIT_TEST(testAssociations);
  CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestMoidConsumer);
