

PYTHON_EXECUTABLE=$(python3 -c "import sys; print(sys.executable)")

BUILD_PATH=$1


VCPKG_ROOT="$(pwd)/vcpkg"
PATH="${PATH}:${VCPKG_ROOT}"
TOOLCHAIN_FILE="$(pwd)/vcpkg/scripts/buildsystems/vcpkg.cmake"

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

THIS_DIR="$(dirname $0)"

python ${THIS_DIR}/package-templates.py templates

echo "SQSGEN-VERSION: ${MAJOR_VERSION}.${MINOR_VERSION}.${BUILD_NUMBER}"

source ${EMSDK}/emsdk_env.sh

cmake \
 -DBUILD_CLI=OFF \
 -DBUILD_TESTS=OFF \
 -DBUILD_BENCHMARKS=OFF \
 -DWITH_MPI=OFF \
 -DBUILD_PYTHON=OFF \
 -DBUILD_WASM=ON \
 -DSQSGEN_BUILD_BRANCH=${GIT_BRANCH} \
 -DSQSGEN_BUILD_COMMIT=${GIT_COMMIT_HASH} \
 -DSQSGEN_MAJOR_VERSION=${MAJOR_VERSION} \
 -DSQSGEN_MINOR_VERSION=${MINOR_VERSION} \
 -DSQSGEN_BUILD_NUMBER=${BUILD_NUMBER} \
 -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE} \
 -DVCPKG_CXX_FLAGS=-fPIC \
 -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=${EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
 -DVCPKG_TARGET_TRIPLET=wasm32-emscripten \
 -DCMAKE_VERBOSE_MAKEFILE=ON \
 -B ${BUILD_PATH} \
 -S .

cmake --build ${BUILD_PATH}
