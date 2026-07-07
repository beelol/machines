# Pin to amd64: on Apple Silicon the default arm64 bionic image is EOL with a separate
# ports.ubuntu.com archive that complicates the fix below. The toolchain cross-compiles to
# x86_64 Windows regardless of host arch, so building the image under amd64 emulation is fine.
FROM --platform=linux/amd64 ubuntu:18.04

USER root

# Passwordless sudo
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# RUN apt-get update --fix-missing && \
#     apt-get install -y \
#         build-essential \
#         file \
#         apt-utils \
#         cmake \
#         cmake-curses-gui \
#         mingw-w64 \
#         libsdl2-dev \
#         libsdl2-image-dev \
#         libopenal-dev \
#         libalure-dev \
#         libenet-dev \
#         libfreetype6-dev \
#         libglew-dev \
#         libglm-dev \
#         libaudio-dev

# Avoid interactive debconf prompts (e.g. config-file conflicts) hanging/failing the build.
ARG DEBIAN_FRONTEND=noninteractive

# Install build deps.
# --force-confold/confdef keep the /etc/sudoers we modified above without dpkg raising an
# interactive conffile prompt (which fails a non-interactive build) when it installs `sudo`.
RUN apt-get update --fix-missing && \
    apt-get install -y \
        -o Dpkg::Options::="--force-confold" \
        -o Dpkg::Options::="--force-confdef" \
        build-essential \
        file \
        apt-utils \
        sudo \
        git \
        cmake \
        cmake-curses-gui \
        mingw-w64 \
        libsdl2-dev \
        libsdl2-image-dev \
        libopenal-dev \
        libalure-dev \
        libenet-dev \
        libfreetype6-dev \
        libglew-dev \
        libglm-dev \
        libaudio-dev && \
    rm -rf /var/lib/apt/lists/*


RUN useradd -ms /bin/bash dev
USER dev
WORKDIR /home/dev
