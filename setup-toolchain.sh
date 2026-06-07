#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPS_DIR="${SCRIPT_DIR}/.dependencies"
mkdir -p "${DEPS_DIR}"

# -----------------------------------------------------------------------------
# 1. ARM GNU Toolchain (arm-none-eabi-gcc)
# -----------------------------------------------------------------------------

TOOLCHAIN_VERSION="15.2.rel1"
TOOLCHAIN_DIR=".dependencies/arm-gnu-toolchain"
TOOLCHAIN_TARBALL="arm-gnu-toolchain-${TOOLCHAIN_VERSION}-x86_64-arm-none-eabi.tar.xz"
TOOLCHAIN_URL="https://developer.arm.com/-/media/Files/downloads/gnu/${TOOLCHAIN_VERSION}/binrel/${TOOLCHAIN_TARBALL}"

if [ -x "${SCRIPT_DIR}/${TOOLCHAIN_DIR}/bin/arm-none-eabi-gcc" ]; then
    echo "ARM GNU Toolchain already installed at ${TOOLCHAIN_DIR}"
    "${SCRIPT_DIR}/${TOOLCHAIN_DIR}/bin/arm-none-eabi-gcc" --version | head -1
else
    echo "Downloading ARM GNU Toolchain ${TOOLCHAIN_VERSION}..."
    curl -L --progress-bar -o "${DEPS_DIR}/${TOOLCHAIN_TARBALL}" "${TOOLCHAIN_URL}"

    echo "Extracting..."
    tar -xf "${DEPS_DIR}/${TOOLCHAIN_TARBALL}" -C "${DEPS_DIR}"

    EXTRACTED_DIR=$(find "${DEPS_DIR}" -maxdepth 1 -type d -name "arm-gnu-toolchain-*" | head -1)
    if [ -d "${EXTRACTED_DIR}" ]; then
        mv "${EXTRACTED_DIR}" "${SCRIPT_DIR}/${TOOLCHAIN_DIR}"
    fi

    rm -f "${DEPS_DIR}/${TOOLCHAIN_TARBALL}"

    echo "Done! Toolchain installed at ${TOOLCHAIN_DIR}"
    "${SCRIPT_DIR}/${TOOLCHAIN_DIR}/bin/arm-none-eabi-gcc" --version | head -1
fi

# -----------------------------------------------------------------------------
# 2. OpenOCD (built from openocd-org master — S32K target support is post-0.12.0)
# -----------------------------------------------------------------------------
#
# Build deps required (install once via your package manager):
#   git autoconf automake libtool pkg-config make texinfo \
#   libusb-1.0-0-dev libhidapi-dev
#

OPENOCD_COMMIT="e6752ecbcf72efe4e213e8418e381ff2e0ffdf54"
OPENOCD_DIR=".dependencies/openocd"
OPENOCD_SRC=".dependencies/openocd-src"
OPENOCD_REPO="https://github.com/openocd-org/openocd.git"

if [ -x "${SCRIPT_DIR}/${OPENOCD_DIR}/bin/openocd" ]; then
    echo "OpenOCD already installed at ${OPENOCD_DIR}"
    "${SCRIPT_DIR}/${OPENOCD_DIR}/bin/openocd" --version 2>&1 | head -1
else
    echo "Cloning OpenOCD (commit ${OPENOCD_COMMIT:0:12})..."
    rm -rf "${SCRIPT_DIR}/${OPENOCD_SRC}"
    git clone "${OPENOCD_REPO}" "${SCRIPT_DIR}/${OPENOCD_SRC}"
    (
        cd "${SCRIPT_DIR}/${OPENOCD_SRC}"
        git checkout "${OPENOCD_COMMIT}"
        git submodule update --init --recursive

        echo "Bootstrapping..."
        ./bootstrap

        echo "Configuring..."
        ./configure \
            --prefix="${SCRIPT_DIR}/${OPENOCD_DIR}" \
            --enable-cmsis-dap \
            --disable-werror

        echo "Building (this takes a few minutes)..."
        make -j"$(nproc)"

        echo "Installing to ${OPENOCD_DIR}..."
        make install
    )

    # Source tree is no longer needed after install
    rm -rf "${SCRIPT_DIR}/${OPENOCD_SRC}"

    echo "Done! OpenOCD installed at ${OPENOCD_DIR}"
    "${SCRIPT_DIR}/${OPENOCD_DIR}/bin/openocd" --version 2>&1 | head -1
fi

echo
echo "Setup complete."