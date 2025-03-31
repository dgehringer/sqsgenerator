

NINJA_EXECUTABLE=$(which ninja)
PYTHON_EXECUTABLE=$(python3 -c "import sys; print(sys.executable)")

TARGET_NAME="_core"
BUILD_PATH="build"

OUTPUT_PATH="$(pwd)/python/sqsgenerator/core"

VERSION_FILE="$(pwd)/python/version"


GIT_COMMIT_HASH=$(git rev-parse HEAD)
GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD)

MAJOR_VERSION=$(python -c "f = open('${VERSION_FILE}'); l = f.readline(); f.close(); print(l.split('.')[0])")
MINOR_VERSION=$(python -c "f = open('${VERSION_FILE}'); l = f.readline(); f.close(); print(l.split('.')[1])")
BUILD_NUMBER=$(python -c "f = open('${VERSION_FILE}'); l = f.readline(); f.close(); print(l.split('.')[2])")

cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_MAKE_PROGRAM=${NINJA_EXECUTABLE} \
  -DBUILD_PYTHON=ON \
  -DMPPP_BUILD_STATIC_LIBRARY=ON \
  -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${OUTPUT_PATH} \
  -G Ninja \
  -DPython_EXECUTABLE=${PYTHON_EXECUTABLE} \
  -DPython3_EXECUTABLE=${PYTHON_EXECUTABLE} \
  -DSQSGEN_MAJOR_VERSION=${MAJOR_VERSION} \
  -DSQSGEN_MINOR_VERSION=${MINOR_VERSION} \
  -DSQSGEN_BUILD_NUMBER=${BUILD_NUMBER} \
  -DSQSGEN_BUILD_BRANCH=${GIT_BRANCH} \
  -DSQSGEN_BUILD_COMMIT=${GIT_COMMIT_HASH} \
  -DCMAKE_VERBOSE_MAKEFILE=ON \
  -B ${BUILD_PATH}


cmake --build ${BUILD_PATH} --target ${TARGET_NAME}

TMP_DIR=$(pwd)

cd ${OUTPUT_PATH} && \
stubgen \
  -m _core \
  -o "." && \
cd ${TMP_DIR}
