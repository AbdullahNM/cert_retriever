FROM ubuntu:22.04

# 1. Install build essentials and dependencies
RUN apt-get update && apt-get install -y \
    g++ cmake make git wget curl \
    libssl-dev libuv1-dev zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# 2. Build & install Cassandra C++ driver from source
WORKDIR /tmp

# Clone official driver repo
RUN git clone https://github.com/datastax/cpp-driver.git


# Build and install driver
WORKDIR /tmp/cpp-driver
RUN mkdir build && cd build && cmake .. && make && make install

# Ensure linker finds libcassandra.so
RUN echo "/usr/local/lib" > /etc/ld.so.conf.d/cassandra.conf && ldconfig

# 3. Copy project code into container
WORKDIR /app
COPY . /app

# 4. Build your app (main.cpp + db_client.cpp)
# -lcassandra = link against driver
RUN g++ main.cpp db_client.cpp -o cert_store -lcassandra -luv -lpthread -lssl -lcrypto

# 5. Run the app when container starts
CMD ["./cert_store"]
