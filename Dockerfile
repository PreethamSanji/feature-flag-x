### Stage 1: Build
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    ninja-build \
    libssl-dev \
    libpq-dev \
    uuid-dev \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# Install vcpkg
WORKDIR /opt
RUN git clone https://github.com/microsoft/vcpkg.git && \
    cd vcpkg && \
    git checkout 2026.03.18 && \
    cd .. && \
    ./vcpkg/bootstrap-vcpkg.sh -disableMetrics

ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

# Copy manifest and install dependencies first (cache layer)
WORKDIR /app
COPY vcpkg.json .
RUN vcpkg install --triplet x64-linux

# Copy source and build
COPY CMakeLists.txt .
COPY src/ src/
COPY tests/ tests/

RUN mkdir build && cd build && \
    cmake -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
          -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

### Stage 2: Runtime
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libpq5 \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/featureflagx .

EXPOSE 8080

CMD ["./featureflagx"]
