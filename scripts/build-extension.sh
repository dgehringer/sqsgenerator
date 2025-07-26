#!/bin/bash
set -ex

BUILD_PATH=$1

PYTHON=python

PROJECT_DIR=/Users/dominik/projects/sqsgenerator-refactor

SRC_DIR=$PROJECT_DIR
TOOLCHAIN_FILE="${SRC_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
PYBIND11_CMAKE_DIR=$($PYTHON -c "import pybind11; print(pybind11.get_cmake_dir())")

cmake \
  -DPython3_EXCUTABLE=$PYTHON \
  -DSKBUILD_PROJECT_NAME=sqsgenerator \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=${PYBIND11_CMAKE_DIR} \
  -DVCPKG_MANIFEST_DIR="${SRC_DIR}" \
  -DVCPKG_MANIFEST_FEATURES="python" \
  -B "${BUILD_PATH}" \
  -S "${SRC_DIR}/python"

cmake --build ${BUILD_PATH} --target _core
cmake --install ${BUILD_PATH} --prefix $SRC_DIR/python

stubgen -m python.sqsgenerator.core._core -o $SRC_DIR/python/sqsgenerator/core
