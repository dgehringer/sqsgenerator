find_package(Sphinx REQUIRED)

set(SPHINX_SOURCE ..)
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)

message("HERE: " ${CMAKE_CURRENT_BINARY_DIR} " " ${SPHINX_SOURCE} " " ${SPHINX_BUILD})
add_custom_target(docs-core ALL
        COMMAND
        ${SPHINX_EXECUTABLE} -b html
        ${SPHINX_SOURCE} ${SPHINX_BUILD}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating documentation with Sphinx")
