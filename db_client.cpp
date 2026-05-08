#include "db_client.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <ctime>

static std::string get_str_col(const CassRow* row, size_t col) {
    const CassValue* val = cass_row_get_column(row, col);
    if (!val || cass_value_is_null(val)) return "NULL";
    const char* data = nullptr;
    size_t length = 0;
    cass_value_get_string(val, &data, &length);
    return std::string(data, length);
}

static std::string get_ts_col(const CassRow* row, size_t col) {
    const CassValue* val = cass_row_get_column(row, col);
    if (!val || cass_value_is_null(val)) return "NULL";
    cass_int64_t micros = 0;
    cass_value_get_int64(val, &micros);
    time_t secs = micros / 1000000;
    char buf[32];
    struct tm tm_buf;
    localtime_r(&secs, &tm_buf);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
    return buf;
}

static int64_t now_micros() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

CassSession* connect_db(const std::string& contact_point, int port, CassCluster** cluster) {
    *cluster = cass_cluster_new();
    CassSession* session = cass_session_new();

    cass_cluster_set_contact_points(*cluster, contact_point.c_str());
    cass_cluster_set_port(*cluster, port);

    CassFuture* connect_future = cass_session_connect(session, *cluster);
    cass_future_wait(connect_future);

    if (cass_future_error_code(connect_future) == CASS_OK) {
        std::cout << "Connected to Cassandra at " << contact_point << ":" << port << std::endl;
    } else {
        const char* msg; size_t len;
        cass_future_error_message(connect_future, &msg, &len);
        std::cerr << "Connection error: " << std::string(msg, len) << std::endl;
        cass_session_free(session);
        cass_cluster_free(*cluster);
        session = nullptr;
    }

    cass_future_free(connect_future);
    return session;
}

void close_db(CassSession* session, CassCluster* cluster) {
    if (session) {
        CassFuture* close_future = cass_session_close(session);
        cass_future_wait(close_future);
        cass_future_free(close_future);
    }
    cass_session_free(session);
    cass_cluster_free(cluster);
}

void ensure_schema(CassSession* session) {
    auto exec = [&](const char* q) {
        CassStatement* stmt = cass_statement_new(q, 0);
        CassFuture* f = cass_session_execute(session, stmt);
        cass_future_wait(f);
        if (cass_future_error_code(f) != CASS_OK) {
            const char* msg; size_t len;
            cass_future_error_message(f, &msg, &len);
            std::cerr << "Schema error: " << std::string(msg, len) << std::endl;
        }
        cass_statement_free(stmt);
        cass_future_free(f);
    };

    exec("CREATE KEYSPACE IF NOT EXISTS security "
         "WITH replication = {'class':'SimpleStrategy','replication_factor':1};");
    exec("CREATE TABLE IF NOT EXISTS security.certificates ("
         "serial_no text PRIMARY KEY, "
         "subject text, "
         "issued_on timestamp, "
         "expires_on timestamp);");
}

bool insert_cert(CassSession* session,
                 const std::string& serial,
                 const std::string& subject,
                 int validity_days) {
    const char* query =
        "INSERT INTO security.certificates (serial_no, subject, issued_on, expires_on) "
        "VALUES (?, ?, ?, ?);";

    CassStatement* stmt = cass_statement_new(query, 4);
    cass_statement_bind_string(stmt, 0, serial.c_str());
    cass_statement_bind_string(stmt, 1, subject.c_str());

    int64_t issued = now_micros();
    int64_t expires = issued + static_cast<int64_t>(validity_days) * 86'400'000'000LL;
    cass_statement_bind_int64(stmt, 2, issued);
    cass_statement_bind_int64(stmt, 3, expires);

    CassFuture* f = cass_session_execute(session, stmt);
    cass_future_wait(f);
    bool ok = cass_future_error_code(f) == CASS_OK;
    if (!ok) {
        const char* msg; size_t len;
        cass_future_error_message(f, &msg, &len);
        std::cerr << "Insert error: " << std::string(msg, len) << std::endl;
    }
    cass_statement_free(stmt);
    cass_future_free(f);
    return ok;
}

bool query_cert(CassSession* session,
                const std::string& serial,
                CertRecord& record) {
    const char* query =
        "SELECT serial_no, subject, issued_on, expires_on "
        "FROM security.certificates WHERE serial_no = ?;";

    CassStatement* stmt = cass_statement_new(query, 1);
    cass_statement_bind_string(stmt, 0, serial.c_str());

    CassFuture* f = cass_session_execute(session, stmt);
    cass_future_wait(f);

    bool found = false;
    if (cass_future_error_code(f) != CASS_OK) {
        const char* msg; size_t len;
        cass_future_error_message(f, &msg, &len);
        std::cerr << "Query error: " << std::string(msg, len) << std::endl;
    } else {
        const CassResult* res = cass_future_get_result(f);
        const CassRow* row = cass_result_first_row(res);
        if (row) {
            record.serial_no = get_str_col(row, 0);
            record.subject = get_str_col(row, 1);
            record.issued_on = get_ts_col(row, 2);
            record.expires_on = get_ts_col(row, 3);
            found = true;
        }
        cass_result_free(res);
    }

    cass_statement_free(stmt);
    cass_future_free(f);
    return found;
}

bool delete_cert(CassSession* session,
                 const std::string& serial) {
    const char* query = "DELETE FROM security.certificates WHERE serial_no = ?;";

    CassStatement* stmt = cass_statement_new(query, 1);
    cass_statement_bind_string(stmt, 0, serial.c_str());

    CassFuture* f = cass_session_execute(session, stmt);
    cass_future_wait(f);
    bool ok = cass_future_error_code(f) == CASS_OK;
    if (!ok) {
        const char* msg; size_t len;
        cass_future_error_message(f, &msg, &len);
        std::cerr << "Delete error: " << std::string(msg, len) << std::endl;
    }
    cass_statement_free(stmt);
    cass_future_free(f);
    return ok;
}

static std::vector<CertRecord> result_to_records(const CassResult* res) {
    std::vector<CertRecord> records;
    CassIterator* it = cass_iterator_from_result(res);
    while (cass_iterator_next(it)) {
        const CassRow* row = cass_iterator_get_row(it);
        CertRecord r;
        r.serial_no = get_str_col(row, 0);
        r.subject = get_str_col(row, 1);
        r.issued_on = get_ts_col(row, 2);
        r.expires_on = get_ts_col(row, 3);
        records.push_back(std::move(r));
    }
    cass_iterator_free(it);
    return records;
}

std::vector<CertRecord> list_all_certs(CassSession* session) {
    const char* query =
        "SELECT serial_no, subject, issued_on, expires_on "
        "FROM security.certificates;";

    CassStatement* stmt = cass_statement_new(query, 0);
    CassFuture* f = cass_session_execute(session, stmt);
    cass_future_wait(f);

    std::vector<CertRecord> records;
    if (cass_future_error_code(f) != CASS_OK) {
        const char* msg; size_t len;
        cass_future_error_message(f, &msg, &len);
        std::cerr << "List error: " << std::string(msg, len) << std::endl;
    } else {
        records = result_to_records(cass_future_get_result(f));
    }
    cass_statement_free(stmt);
    cass_future_free(f);
    return records;
}

std::vector<CertRecord> list_expiring_certs(CassSession* session, int within_days) {
    int64_t cutoff = now_micros() + static_cast<int64_t>(within_days) * 86'400'000'000LL;

    const char* query =
        "SELECT serial_no, subject, issued_on, expires_on "
        "FROM security.certificates WHERE expires_on <= ? ALLOW FILTERING;";

    CassStatement* stmt = cass_statement_new(query, 1);
    cass_statement_bind_int64(stmt, 0, cutoff);

    CassFuture* f = cass_session_execute(session, stmt);
    cass_future_wait(f);

    std::vector<CertRecord> records;
    if (cass_future_error_code(f) != CASS_OK) {
        const char* msg; size_t len;
        cass_future_error_message(f, &msg, &len);
        std::cerr << "Expiring query error: " << std::string(msg, len) << std::endl;
    } else {
        records = result_to_records(cass_future_get_result(f));
    }
    cass_statement_free(stmt);
    cass_future_free(f);
    return records;
}
