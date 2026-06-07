#!/usr/bin/env bash
set -euo pipefail

TOOLCHAIN_VERSION="15.2.rel1"
TOOLCHAIN_DIR=".dependencies/arm-gnu-toolchain"
TARBALL="arm-gnu-toolchain-${TOOLCHAIN_VERSION}-x86_64-arm-none-eabi.tar.xz"
URL="https://developer.arm.com/-/media/Files/downloads/gnu/${TOOLCHAIN_VERSION}/binrel/${TARBALL}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPS_DIR="${SCRIPT_DIR}/.dependencies"

# Check if already installed
if [ -x "${SCRIPT_DIR}/${TOOLCHAIN_DIR}/bin/arm-none-eabi-gcc" ]; then
    echo "ARM GNU Toolchain already installed at ${TOOLCHAIN_DIR}"
    "${SCRIPT_DIR}/${TOOLCHAIN_DIR}/bin/arm-none-eabi-gcc" --version | head -1
    exit 0
fi

echo "Downloading ARM GNU Toolchain ${TOOLCHAIN_VERSION}..."
mkdir -p "${DEPS_DIR}"

curl -L --progress-bar -o "${DEPS_DIR}/${TARBALL}" "${URL}"

echo "Extracting..."
tar -xf "${DEPS_DIR}/${TARBALL}" -C "${DEPS_DIR}"

# The tarball extracts to a versioned directory name — rename to a stable path
EXTRACTED_DIR=$(find "${DEPS_DIR}" -maxdepth 1 -type d -name "arm-gnu-toolchain-*" | head -1)
if [ -d "${EXTRACTED_DIR}" ]; then
    mv "${EXTRACTED_DIR}" "${SCRIPT_DIR}/${TOOLCHAIN_DIR}"
fi

# Clean up tarball
rm -f "${DEPS_DIR}/${TARBALL}"

echo "Done! Toolchain installed at ${TOOLCHAIN_DIR}"
"${SCRIPT_DIR}/${TOOLCHAIN_DIR}/bin/arm-none-eabi-gcc" --version | head -1
