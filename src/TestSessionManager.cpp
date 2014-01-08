#include "test.h"

class TestSessionManager : public TestFixture
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
   * Test the session manager by storing and loading a session.
   */
  void testSessionManager()
  {
    SessionManager s(dbd);

    s.clear_tables();

    time_t now = 1386810000; // no religious or practical significance

    // a fake session
    int lifespan = 50;
    session_t stored_session;
    make_rstring(32, stored_session.session_id);
    stored_session.hostname = "example.org";
    stored_session.path = "/";
    stored_session.identity = "https://example.org/accounts/id?id=ABig4324Blob_ofLetters90_8And43Numbers";
    stored_session.username = "lizard_king";
    stored_session.expires_on = now + lifespan;

    // try to store it
    success = s.store_session(stored_session.session_id, stored_session.hostname,
                              stored_session.path, stored_session.identity,
                              stored_session.username, lifespan,
                              now);
    CPPUNIT_ASSERT_MESSAGE(
      "Couldn't store session",
      success);

    // try to load it
    success = s.get_session(stored_session.session_id, loaded_session, now);
    CPPUNIT_ASSERT_MESSAGE(
      "Couldn't load session",
      success);

    // verify that stored session and loaded session are identical
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
      "Field not equal between stored and loaded session: session_id",
      stored_session.session_id,
      loaded_session.session_id);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
      "Field not equal between stored and loaded session: hostname",
      stored_session.hostname,
      loaded_session.hostname);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
      "Field not equal between stored and loaded session: path",
      stored_session.path,
      loaded_session.path);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
      "Field not equal between stored and loaded session: identity",
      stored_session.identity,
      loaded_session.identity);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
      "Field not equal between stored and loaded session: username",
      stored_session.username,
      loaded_session.username);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
      "Field not equal between stored and loaded session: expires_on",
      stored_session.expires_on,
      loaded_session.expires_on);

    // go past the session's expiration date and try to load the session again
    success = s.get_session(stored_session.session_id, loaded_session, now + lifespan);
    CPPUNIT_ASSERT_MESSAGE(
      "Should not have been able to retrieve expired session",
      !success);
  }

  CPPUNIT_TEST_SUITE(TestSessionManager);
  CPPUNIT_TEST(testSessionManager);
  CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestSessionManager);
