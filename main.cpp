#include "db_client.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <vector>

static void print_help() {
    std::cout << R"(
Commands:
  insert <serial> <subject> [days]  Insert a certificate (days = validity, default 365)
  query <serial>                    Query a certificate
  delete <serial>                   Delete a certificate
  list                              List all certificates
  expiring <days>                   List certificates expiring within N days
  help                              Show this help
  exit                              Exit
)" << std::endl;
}

static void cmd_insert(CassSession* session, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Usage: insert <serial> <subject> [days]" << std::endl;
        return;
    }
    int days = 365;
    if (args.size() >= 3) {
        try { days = std::stoi(args[2]); }
        catch (...) { std::cout << "Invalid days value" << std::endl; return; }
    }
    if (insert_cert(session, args[0], args[1], days))
        std::cout << "Inserted: " << args[0] << std::endl;
}

static void cmd_query(CassSession* session, const std::string& serial) {
    CertRecord rec;
    if (query_cert(session, serial, rec)) {
        std::cout << "Serial:   " << rec.serial_no << "\n"
                  << "Subject:  " << rec.subject << "\n"
                  << "Issued:   " << rec.issued_on << "\n"
                  << "Expires:  " << rec.expires_on << std::endl;
    } else {
        std::cout << "No certificate found with serial: " << serial << std::endl;
    }
}

static void cmd_delete(CassSession* session, const std::string& serial) {
    if (delete_cert(session, serial))
        std::cout << "Deleted: " << serial << std::endl;
}

static void cmd_list(CassSession* session) {
    auto records = list_all_certs(session);
    if (records.empty()) {
        std::cout << "No certificates found." << std::endl;
        return;
    }
    for (const auto& r : records)
        std::cout << "  " << r.serial_no << " | " << r.subject
                  << " | expires: " << r.expires_on << std::endl;
    std::cout << "Total: " << records.size() << std::endl;
}

static void cmd_expiring(CassSession* session, int days) {
    auto records = list_expiring_certs(session, days);
    if (records.empty()) {
        std::cout << "No certificates expiring within " << days << " days." << std::endl;
        return;
    }
    for (const auto& r : records)
        std::cout << "  " << r.serial_no << " | " << r.subject
                  << " | expires: " << r.expires_on << std::endl;
    std::cout << "Total expiring: " << records.size() << std::endl;
}

static std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> parts;
    std::istringstream iss(s);
    std::string tok;
    while (iss >> tok) parts.push_back(tok);
    return parts;
}

int main() {
    std::string contact_point = "cassandra";
    int port = 9042;

    CassCluster* cluster = nullptr;
    CassSession* session = nullptr;

    int max_retries = 10;
    for (int i = 0; i < max_retries; i++) {
        session = connect_db(contact_point, port, &cluster);
        if (session) break;
        std::cout << "Cassandra not ready, retrying in 10s (" << i+1 << "/" << max_retries << ")..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    if (!session) {
        std::cerr << "Failed to connect to Cassandra." << std::endl;
        return 1;
    }

    ensure_schema(session);
    std::cout << "\nCertificate Store CLI. Type 'help' for commands." << std::endl;

    std::string line;
    while (true) {
        std::cout << "\n> ";
        if (!std::getline(std::cin, line)) break;
        auto args = split(line);
        if (args.empty()) continue;

        if (args[0] == "exit") break;
        else if (args[0] == "help") print_help();
        else if (args[0] == "list") cmd_list(session);
        else if (args[0] == "query" && args.size() >= 2) cmd_query(session, args[1]);
        else if (args[0] == "delete" && args.size() >= 2) cmd_delete(session, args[1]);
        else if (args[0] == "expiring" && args.size() >= 2) {
            try { cmd_expiring(session, std::stoi(args[1])); }
            catch (...) { std::cout << "Invalid days value" << std::endl; }
        }
        else if (args[0] == "insert") {
            if (args.size() < 3) {
                std::cout << "Usage: insert <serial> <subject> [days]" << std::endl;
                continue;
            }
            std::vector<std::string> insert_args(args.begin() + 1, args.end());
            cmd_insert(session, insert_args);
        }
        else std::cout << "Unknown command. Type 'help'." << std::endl;
    }

    close_db(session, cluster);
    return 0;
}
