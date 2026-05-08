#pragma once
#include <cassandra.h>
#include <string>
#include <vector>

struct CertRecord {
    std::string serial_no;
    std::string subject;
    std::string issued_on;
    std::string expires_on;
};

CassSession* connect_db(const std::string& contact_point, int port, CassCluster** cluster);
void close_db(CassSession* session, CassCluster* cluster);
void ensure_schema(CassSession* session);

bool insert_cert(CassSession* session,
                 const std::string& serial,
                 const std::string& subject,
                 int validity_days = 365);

bool query_cert(CassSession* session,
                const std::string& serial,
                CertRecord& record);

bool delete_cert(CassSession* session,
                 const std::string& serial);

std::vector<CertRecord> list_all_certs(CassSession* session);
std::vector<CertRecord> list_expiring_certs(CassSession* session, int within_days);
