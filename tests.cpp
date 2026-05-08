#include "test_runner.h"
#include "db_client.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>

static CassSession* g_session = nullptr;
static CassCluster* g_cluster = nullptr;

static void init() {
    const char* host = std::getenv("CASSANDRA_HOST");
    std::string contact = host ? host : "cassandra";
    int port = 9042;

    for (int i = 0; i < 15; i++) {
        g_session = connect_db(contact, port, &g_cluster);
        if (g_session) break;
        std::cout << "Waiting for Cassandra (" << i+1 << "/15)..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    if (!g_session) {
        std::cerr << "Cannot connect to Cassandra for tests." << std::endl;
        std::exit(1);
    }
    ensure_schema(g_session);
}

static void cleanup() {
    close_db(g_session, g_cluster);
}

TEST(InsertAndQuery) {
    ASSERT_TRUE(insert_cert(g_session, "T001", "CN=Test One", 365));
    CertRecord rec;
    ASSERT_TRUE(query_cert(g_session, "T001", rec));
    ASSERT_EQ(rec.serial_no, "T001");
    ASSERT_EQ(rec.subject, "CN=Test One");
}

TEST(QueryNonExistent) {
    CertRecord rec;
    ASSERT_TRUE(!query_cert(g_session, "NONEXISTENT_SERIAL_XYZ", rec));
}

TEST(DeleteCert) {
    ASSERT_TRUE(insert_cert(g_session, "T002", "CN=Delete Me", 30));
    CertRecord rec;
    ASSERT_TRUE(query_cert(g_session, "T002", rec));
    ASSERT_TRUE(delete_cert(g_session, "T002"));
    ASSERT_TRUE(!query_cert(g_session, "T002", rec));
}

TEST(DeleteNonExistent) {
    ASSERT_TRUE(delete_cert(g_session, "DOES_NOT_EXIST_999"));
}

TEST(ListAll) {
    insert_cert(g_session, "L001", "CN=List One", 365);
    insert_cert(g_session, "L002", "CN=List Two", 365);
    insert_cert(g_session, "L003", "CN=List Three", 365);
    auto records = list_all_certs(g_session);
    ASSERT_NE(records.size(), 0);
}

TEST(ListExpiring) {
    insert_cert(g_session, "E001", "CN=Expiring Soon", 1);
    insert_cert(g_session, "E002", "CN=Not Expiring", 365);
    auto records = list_expiring_certs(g_session, 30);
    bool found_expiring = false;
    for (const auto& r : records) {
        if (r.serial_no == "E001") found_expiring = true;
    }
    ASSERT_TRUE(found_expiring);
}

TEST(InsertCustomValidity) {
    ASSERT_TRUE(insert_cert(g_session, "V001", "CN=Zero Days", 0));
    auto records = list_expiring_certs(g_session, 0);
    bool found = false;
    for (const auto& r : records) {
        if (r.serial_no == "V001") found = true;
    }
    ASSERT_TRUE(found);
}

int main() {
    std::cout << "=== Certificate Store Integration Tests ===" << std::endl;
    init();
    int result = run_all_tests();
    cleanup();
    return result;
}
