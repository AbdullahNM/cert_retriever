CREATE KEYSPACE IF NOT EXISTS security
WITH replication = {'class':'SimpleStrategy','replication_factor':1};

USE security;

CREATE TABLE IF NOT EXISTS certificates (
    serial_no text PRIMARY KEY,
    subject text,
    issued_on timestamp,
    expires_on timestamp
);
