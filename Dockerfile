FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    g++ cmake make git wget curl \
    libssl-dev libuv1-dev zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /tmp
RUN git clone https://github.com/datastax/cpp-driver.git
WORKDIR /tmp/cpp-driver
RUN mkdir build && cd build && \
    cmake .. -DCASS_BUILD_STATIC=OFF \
    -DCASS_USE_STD_ATOMIC=ON && \
    make -j$(nproc) && make install
RUN echo "/usr/local/lib" > /etc/ld.so.conf.d/cassandra.conf && ldconfig

WORKDIR /app
COPY . /app
RUN mkdir -p build && cd build && cmake .. && make -j$(nproc)

CMD ["./build/cert_store"]
