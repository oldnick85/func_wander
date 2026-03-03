FROM ubuntu:25.10

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    clang \
    cmake \
    # Python and Conan
    python3-pip \
    python3-venv \
    # Version control
    git \
    # Formatting and static analysis
    clang-format \
    clang-tidy \
    # Clean up to reduce image size
    && rm -rf /var/lib/apt/lists/*

# Install Conan 2 (package manager)
RUN pip3 install --no-cache-dir conan==2.23.0 --break-system-packages

# Set working directory
WORKDIR /workspace

# Copy entrypoint script
COPY scripts/docker/linux/entrypoint.sh /opt/linux/entrypoint.sh
RUN chmod +x /opt/linux/entrypoint.sh

# Default entrypoint
ENTRYPOINT ["/opt/linux/entrypoint.sh"]