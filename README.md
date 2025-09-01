# Certificate Store (C++ + Cassandra + Docker)

## Overview

This project is a minimal backend demo simulating a **Certificate Metadata Store**, built with:

* **C++17** for backend logic
* **Apache Cassandra** (NoSQL database) for storage
* **Docker & Docker Compose** for containerized deployment

It demonstrates how digital certificate data (serial number, subject, issue date, expiry date) can be stored and queried in a distributed database, which is relevant to **PKI (Public Key Infrastructure)** and **CLM (Certificate Lifecycle Management)** systems.

---

## Features

* Connects to Cassandra from a C++ client using the DataStax C++ Driver.
* Automatically ensures the keyspace (`security`) and table (`certificates`) exist.
* Inserts a certificate record with serial and subject.
* Queries and prints the certificate details.

---

## Tech Stack

* **C++17**
* **Apache Cassandra (latest Docker image)**
* **DataStax C++ Driver**
* **Docker / Docker Compose**

---

## Project Structure

```
Certificate_store/
├── Dockerfile              # Builds the C++ app container
├── docker-compose.yml      # Defines multi-container setup
├── main.cpp                # Entry point, DB connection + demo logic
├── db_client.cpp           # DB functions (connect, insert, query)
├── db_client.h             # DB function declarations
```

---

## How to Build and Run

### 1. Clone the Repo

```bash
git clone https://github.com/<your-username>/Certificate_store.git
cd Certificate_store
```

### 2. Build and Start Containers

```bash
docker-compose up --build
```

This will:

* Start a Cassandra container (listening on `9042`).
* Build and run the C++ app container.

### 3. Expected Output

If successful, logs will show:

```
✅ Connected to Cassandra at cassandra:9042
✅ Inserted cert with serial: 67890
🔍 Fetched cert subject: CN=Modular Demo
```

### 4. Verify Data in Cassandra

Open a CQL shell:

```bash
docker exec -it cassandra cqlsh
```

Inside:

```sql
USE security;
SELECT * FROM certificates;
```

You should see the row inserted by the C++ app.

---

## Troubleshooting

* If you see **`connection refused`**, Cassandra may still be starting. The app has retry logic to wait.
* If **`keyspace security does not exist`**, ensure schema creation is included in `main.cpp` (already provided).
* To reset everything:

```bash
docker-compose down --volumes --remove-orphans
docker-compose up --build
```

---

## Future Enhancements

* Interactive CLI for inserting, deleting, and querying certs.
* Query certificates expiring within N days.
* REST API interface on top of the C++ client.
* TLS-encrypted Cassandra connections for production security.

---

## Why This Project

This mini-project demonstrates:

* **C++ backend development**
* **Integration with distributed NoSQL database (Cassandra)**
* **Containerization with Docker**

It aligns with real-world PKI/CLM systems where millions of digital certificates need to be issued, tracked, and validated at scale.
