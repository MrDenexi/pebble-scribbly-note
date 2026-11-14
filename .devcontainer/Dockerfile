# Pebble SDK 4.x (Core Devices / repebble) development container.
# Ubuntu 24.04 is required: the Pebble emulator (QEMU) needs glibc >= 2.38.
FROM mcr.microsoft.com/devcontainers/base:ubuntu-24.04

ARG DEBIAN_FRONTEND=noninteractive

# Runtime deps from https://developer.repebble.com/sdk/ (Linux section),
# plus build-essential and a few emulator/QEMU extras.
RUN apt-get update && apt-get install -y --no-install-recommends \
        nodejs npm \
        libsdl2-2.0-0 libglib2.0-0 libpixman-1-0 zlib1g libsndio7.0 \
        libfdt1 libpng16-16t64 \
        build-essential python3 python3-venv \
        curl ca-certificates git \
    && rm -rf /var/lib/apt/lists/*

# uv (Python tool manager) — official static binaries.
COPY --from=ghcr.io/astral-sh/uv:0.8.0 /uv /uvx /usr/local/bin/

USER vscode
ENV PATH="/home/vscode/.local/bin:${PATH}"

# pebble-tool supports Python 3.10–3.13 (not 3.14); pin explicitly.
RUN uv tool install pebble-tool --python 3.12

# Install the SDK (also downloads the arm-none-eabi toolchain and QEMU)
# into ~/.local/share/pebble-sdk so it is baked into the image.
RUN yes | pebble sdk install latest && pebble sdk list
