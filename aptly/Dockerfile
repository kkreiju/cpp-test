# Use a lightweight Debian image
FROM debian:bookworm-slim

# Install Aptly, GPG, rng-tools (for entropy), and dos2unix (for Windows formatting)
RUN apt-get update && apt-get install -y \
    aptly \
    gnupg \
    rng-tools \
    dos2unix \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy the .deb file(s) from the current folder (aptly/) to /packages/
COPY *.deb /packages/

# Copy the entrypoint script from the current folder
COPY entrypoint.sh /app/entrypoint.sh

# CRITICAL FIX: Convert Windows line endings (CRLF) to Linux (LF) to prevent crash
RUN dos2unix /app/entrypoint.sh && chmod +x /app/entrypoint.sh

# Expose the port
EXPOSE 8080

# Run the script
ENTRYPOINT ["/app/entrypoint.sh"]