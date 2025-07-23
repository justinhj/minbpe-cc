# Dockerfile is only needed if you want to test the amd x86_64 build locally.
FROM ubuntu:24.04

# Set the working directory inside the container
WORKDIR /app

# Install all necessary dependencies in a single step
# This includes packages for installing Zig and your project's dependencies
RUN apt-get update && \
    apt-get install -y \
    wget \
    xz-utils \
    libboost-all-dev \
    libpcre2-dev \
    libcli11-dev \
    rm -rf /var/lib/apt/lists/*

# Download and install the same Zig version used in the workflow
RUN wget https://ziglang.org/download/0.14.0/zig-linux-x86_64-0.14.0.tar.xz && \
    tar -xf zig-linux-x86_64-0.14.0.tar.xz && \
    rm zig-linux-x86_64-0.14.0.tar.xz

ENV DEFAULT_LIB_PATH="$(realpath --relative-to=$PWD /usr/lib/x86_64-linux-gnu)"
ENV DEFAULT_INCLUDE_PATH="$(realpath --relative-to=$PWD /usr/include)"


# Add Zig to the PATH environment variable
ENV PATH="/app/zig-linux-x86_64-0.14.0:${PATH}"

# Copy your project's source code into the container
COPY . .

# Set the default command to open a shell for interactive debugging
CMD ["bash"]
