
message(STATUS "Building sqsgen WASM module")


add_executable(core sqsgen.cpp)


set_target_properties(core PROPERTIES
        SUFFIX ".js"
)
set_target_properties(core PROPERTIES COMPILE_FLAGS "-fPIC -Os ")
set_target_properties(core PROPERTIES LINK_FLAGS "-Os --emit-tsd core.d.ts -s EXPORT_ES6=1 -fwasm-exceptions -sPROXY_TO_PTHREAD -sEXIT_RUNTIME=0 -sASSERTIONS=2 -s MODULARIZE=1 --bind -fPIC -pthread")
target_link_libraries(core PRIVATE nlohmann_json::nlohmann_json Eigen3::Eigen Boost::multiprecision embind)
