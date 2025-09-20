#!/usr/bin/env bash
if [[ ! -d .git ]]
then
  echo "Script should be called from project root directory!"
fi

IMAGE_NAME=machines-cpp-dev

# Use the project directory on the host as the working directory inside the
# container.  This prevents CMake from hardcoding an inaccessible path (e.g.
# /opt/dev) into its cache which would later break local builds once the
# container exits.  Users can override the container path by setting the
# CONTAINER_WORKDIR environment variable when invoking the script.
HOST_WORKDIR="$(pwd)"
CONTAINER_WORKDIR="${CONTAINER_WORKDIR:-${HOST_WORKDIR}}"
MOUNTS="${HOST_WORKDIR}:${CONTAINER_WORKDIR}"

set -e
echo "Logging into docker shell"
docker run -it \
  -v "${MOUNTS}" \
  -w "${CONTAINER_WORKDIR}" \
  "${IMAGE_NAME}" \
  bash
