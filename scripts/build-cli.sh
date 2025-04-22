

PYTHON_EXECUTABLE=$(python3 -c "import sys; print(sys.executable)")

BUILD_PATH=$1

VCPKG_ROOT="$(pwd)/vcpkg"
PATH="${PATH}:${VCPKG_ROOT}"
TOOLCHAIN_FILE="$(pwd)/vcpkg/scripts/buildsystems/vcpkg.cmake"
PYBIND11_CMAKE_DIR=$(python -c "import pybind11; print(pybind11.get_cmake_dir())")

GIT_COMMIT_HASH=$(git rev-parse HEAD)
GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD)

VERSION_FILE="$(pwd)/vcpkg.json"

function version_field() {
  local index=$1;
  echo $(python -c "import json; f = open('${VERSION_FILE}'); d = json.loads(f.read()); f.close(); print(d['version-string'].split('.')[${index}])");
}

MAJOR_VERSION=$(version_field 0)
MINOR_VERSION=$(version_field 1)
BUILD_NUMBER=$(version_field 2)

echo "SQSGEN-VERSION: ${MAJOR_VERSION}.${MINOR_VERSION}.${BUILD_NUMBER}"

cmake \
  -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE} \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_PYTHON=OFF \
  -DBUILD_TESTS=OFF \
  -DBUILD_CLI=ON \
  -DCMAKE_PREFIX_PATH=${PYBIND11_CMAKE_DIR} \
  -DPython_EXECUTABLE=${PYTHON_EXECUTABLE} \
  -DPython3_EXECUTABLE=${PYTHON_EXECUTABLE} \
  -DSQSGEN_MAJOR_VERSION=${MAJOR_VERSION} \
  -DSQSGEN_MINOR_VERSION=${MINOR_VERSION} \
  -DSQSGEN_BUILD_NUMBER=${BUILD_NUMBER} \
  -DSQSGEN_BUILD_BRANCH=${GIT_BRANCH} \
  -DSQSGEN_BUILD_COMMIT=${GIT_COMMIT_HASH} \
  -DCMAKE_VERBOSE_MAKEFILE=ON \
  -B ${BUILD_PATH} \
  -S .

cmake --build ${BUILD_PATH}
