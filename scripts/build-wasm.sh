BUILD_PATH=$1

THIS_DIR=$(dirname "$(readlink -f "$0")")

PROJECT_DIR="${THIS_DIR}/.."
TOOLCHAIN_FILE="${PROJECT_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"

source ${EMSDK}/emsdk_env.sh

cmake \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DVCPKG_MANIFEST_DIR="${PROJECT_DIR}" \
  -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=${EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
  -DVCPKG_TARGET_TRIPLET=wasm32-emscripten \
  -B "${BUILD_PATH}" \
  -S "${PROJECT_DIR}/js/wasm"

cmake --build ${BUILD_PATH} --target core
