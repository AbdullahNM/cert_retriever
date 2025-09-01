#include "db_client.h"
#include <iostream>
#include <thread>
#include <chrono>

// Helper: execute arbitrary CQL (for schema creation)
void execute_query(CassSession* session, const char* query) {
    CassStatement* statement = cass_statement_new(query, 0);
    CassFuture* result = cass_session_execute(session, statement);
    cass_future_wait(result);

    if (cass_future_error_code(result) != CASS_OK) {
        const char* msg; size_t len;
        cass_future_error_message(result, &msg, &len);
        std::cerr << "❌ Schema query failed: " << std::string(msg, len) << std::endl;
    }

    cass_statement_free(statement);
    cass_future_free(result);
}

int main() {
    const std::string contact_point = "cassandra";  // docker-compose service name
    int port = 9042;

    CassCluster* cluster = nullptr;
    CassSession* session = nullptr;

    // Retry logic: try up to 10 times, wait 10s each
    int max_retries = 10;
    int attempt = 0;

    while (attempt < max_retries) {
        session = connect_db(contact_point, port, &cluster);
        if (session) break; // Connected successfully

        std::cout << "⏳ Cassandra not ready yet. Retrying in 10s (attempt "
                  << attempt + 1 << "/" << max_retries << ")..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));
        attempt++;
    }

    if (!session) {
        std::cerr << "❌ Failed to connect to Cassandra after retries." << std::endl;
        return 1;
    }

    // Ensure keyspace and table exist
    execute_query(session,
        "CREATE KEYSPACE IF NOT EXISTS security "
        "WITH replication = {'class':'SimpleStrategy','replication_factor':1};");

    execute_query(session,
        "CREATE TABLE IF NOT EXISTS security.certificates ("
        "serial_no text PRIMARY KEY, "
        "subject text, "
        "issued_on timestamp, "
        "expires_on timestamp);");

    // Demo: insert + query
    insert_cert(session, "67890", "CN=Retry Logic Demo");
    query_cert(session, "67890");

    // Clean shutdown
    close_db(session, cluster);
    return 0;
}
