#include "db_client.h"
#include <iostream>

/**
 * Connect to Cassandra cluster and open a session.
 */
CassSession* connect_db(const std::string& contact_point, int port, CassCluster** cluster) {
    *cluster = cass_cluster_new();             // Create cluster object
    CassSession* session = cass_session_new(); // Create session object

    // Configure contact point and port
    cass_cluster_set_contact_points(*cluster, contact_point.c_str());
    cass_cluster_set_port(*cluster, port);

    // Attempt connection
    CassFuture* connect_future = cass_session_connect(session, *cluster);
    cass_future_wait(connect_future);

    if (cass_future_error_code(connect_future) == CASS_OK) {
        std::cout << "✅ Connected to Cassandra at " << contact_point << ":" << port << std::endl;
    } else {
        // Print error message if connection fails
        const char* msg; size_t len;
        cass_future_error_message(connect_future, &msg, &len);
        std::cerr << "❌ Connection error: " << std::string(msg, len) << std::endl;
        cass_session_free(session);
        cass_cluster_free(*cluster);
        session = nullptr;
    }

    cass_future_free(connect_future);
    return session;
}

/**
 * Close Cassandra session and free resources.
 */
void close_db(CassSession* session, CassCluster* cluster) {
    if (session) {
        CassFuture* close_future = cass_session_close(session);
        cass_future_wait(close_future);
        cass_future_free(close_future);
    }
    cass_session_free(session);
    cass_cluster_free(cluster);
}

/**
 * Insert a certificate record into Cassandra.
 */
void insert_cert(CassSession* session,
                 const std::string& serial,
                 const std::string& subject) {
    // CQL insert query
    const char* query =
        "INSERT INTO security.certificates (serial_no, subject, issued_on, expires_on) "
        "VALUES (?, ?, toTimestamp(now()), toTimestamp(now()));";

    // Prepare query statement with 2 bound params
    CassStatement* statement = cass_statement_new(query, 2);
    cass_statement_bind_string(statement, 0, serial.c_str());  // bind serial
    cass_statement_bind_string(statement, 1, subject.c_str()); // bind subject

    // Execute query
    CassFuture* result = cass_session_execute(session, statement);
    cass_future_wait(result);

    // Check for errors
    if (cass_future_error_code(result) != CASS_OK) {
        const char* msg; size_t len;
        cass_future_error_message(result, &msg, &len);
        std::cerr << "❌ Insert error: " << std::string(msg, len) << std::endl;
    } else {
        std::cout << "✅ Inserted cert with serial: " << serial << std::endl;
    }

    // Free memory
    cass_statement_free(statement);
    cass_future_free(result);
}

/**
 * Query a certificate by serial number and print details.
 */
void query_cert(CassSession* session,
                const std::string& serial) {
    const char* query =
        "SELECT subject, issued_on, expires_on FROM security.certificates WHERE serial_no=?;";

    CassStatement* statement = cass_statement_new(query, 1);
    cass_statement_bind_string(statement, 0, serial.c_str());

    CassFuture* result = cass_session_execute(session, statement);
    cass_future_wait(result);

    if (cass_future_error_code(result) != CASS_OK) {
        const char* msg; size_t len;
        cass_future_error_message(result, &msg, &len);
        std::cerr << "❌ Query error: " << std::string(msg, len) << std::endl;
    } else {
        const CassResult* res = cass_future_get_result(result);
        const CassRow* row = cass_result_first_row(res);

        if (row) {
            // Extract string value for subject
            const char* subj; size_t subj_len;
            cass_value_get_string(cass_row_get_column(row, 0), &subj, &subj_len);
            std::cout << "🔍 Fetched cert subject: " << std::string(subj, subj_len) << std::endl;
        } else {
            std::cout << "⚠️ No certificate found with serial: " << serial << std::endl;
        }

        cass_result_free(res);
    }

    cass_statement_free(statement);
    cass_future_free(result);
}
