BUILD_PATH=$1

THIS_DIR=$(dirname "$(readlink -f "$0")")

PROJECT_DIR="${THIS_DIR}/.."
TOOLCHAIN_FILE="${PROJECT_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"

python ${THIS_DIR}/package-templates.py ${PROJECT_DIR}/templates

cmake \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DVCPKG_MANIFEST_DIR="${PROJECT_DIR}" \
  -DVCPKG_INSTALLED_DIR="${PROJECT_DIR}/vcpkg_installed" \
  -DVCPKG_MANIFEST_FEATURES="cli" \
  -B "${BUILD_PATH}" \
  -S "${PROJECT_DIR}/cli"

cmake --build ${BUILD_PATH} --target sqsgen
