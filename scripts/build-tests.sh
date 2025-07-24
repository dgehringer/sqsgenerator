BUILD_PATH=$1

THIS_DIR=$(dirname "$(readlink -f "$0")")

PROJECT_DIR="${THIS_DIR}/.."
TOOLCHAIN_FILE="${PROJECT_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
PYBIND11_CMAKE_DIR=$(python -c "import pybind11; print(pybind11.get_cmake_dir())")

cmake \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=${PYBIND11_CMAKE_DIR} \
  -DVCPKG_MANIFEST_DIR="${PROJECT_DIR}" \
  -DVCPKG_MANIFEST_FEATURES="tests" \
  -DVCPKG_INSTALLED_DIR="${PROJECT_DIR}/vcpkg_installed" \
  -B "${BUILD_PATH}" \
  -S "${PROJECT_DIR}/tests"

cmake --build ${BUILD_PATH}
