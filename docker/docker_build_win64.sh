#!/usr/bin/env bash
set -e

# Get absolute path to project root (one directory up from where this script lives)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( dirname "$SCRIPT_DIR" )"

IMAGE_NAME=machines-cpp-dev

# Use the same working directory path inside the container as on the host so
# that generated build files do not reference an inaccessible location (e.g.
# /opt/dev).  A custom path can be provided via the CONTAINER_WORKDIR
# environment variable if required.
HOST_WORKDIR="${PROJECT_ROOT}"
CONTAINER_WORKDIR="${CONTAINER_WORKDIR:-${HOST_WORKDIR}}"

# Run cmake + make inside the container, mounting the project root
docker run --rm \
  -v "${HOST_WORKDIR}:${CONTAINER_WORKDIR}" \
  -w "${CONTAINER_WORKDIR}" \
  "${IMAGE_NAME}" \
  bash -c "
    rm -rf buildMingw64 &&
    mkdir buildMingw64 &&
    cd buildMingw64 &&
    cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-mingw64.cmake \
          -DCMAKE_BUILD_TYPE=Release \
          -DDEV_BUILD=OFF \
          -DCMAKE_VERBOSE_MAKEFILE=ON .. &&
    make -j\$(nproc)
  "
