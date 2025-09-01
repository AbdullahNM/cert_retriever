#ifndef DB_CLIENT_H
#define DB_CLIENT_H

#include <cassandra.h>
#include <string>

/**
 * Establishes a connection to Cassandra and returns a session object.
 * @param contact_point The hostname or IP of Cassandra.
 * @param port The port number (default 9042).
 * @return A pointer to CassSession (active connection).
 */
CassSession* connect_db(const std::string& contact_point, int port, CassCluster** cluster);

/**
 * Gracefully closes the Cassandra session and frees resources.
 * @param session The Cassandra session.
 * @param cluster The Cassandra cluster configuration object.
 */
void close_db(CassSession* session, CassCluster* cluster);

/**
 * Inserts a certificate record into the `certificates` table.
 * @param session Active Cassandra session.
 * @param serial The certificate serial number.
 * @param subject The subject of the certificate (e.g., CN=...).
 */
void insert_cert(CassSession* session,
                 const std::string& serial,
                 const std::string& subject);

/**
 * Queries a certificate by serial number and prints its subject + metadata.
 * @param session Active Cassandra session.
 * @param serial The certificate serial number to query.
 */
void query_cert(CassSession* session,
                const std::string& serial);

#endif
