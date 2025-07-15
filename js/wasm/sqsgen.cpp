#include <emscripten/bind.h>
#include <emscripten/emscripten.h>

#include "sqsgen/core/helpers.h"
#include "sqsgen/core/results.h"
#include "sqsgen/io/config/combined.h"
#include "sqsgen/io/json.h"
#include "sqsgen/io/parsing.h"
#include "sqsgen/io/structure.h"
#include "sqsgen/log.h"
#include "sqsgen/sqs.h"
#include "sqsgen/types.h"

using namespace emscripten;
using namespace sqsgen::core::helpers;

template <class T> nlohmann::json to_json(T&& value) {
  nlohmann::json j = value;
  return j;
}

nlohmann::json to_internal(val const& v) {
  return nlohmann::json::parse(val::global("JSON").call<std::string>("stringify", v));
}

val to_js(nlohmann::json const& j) {
  return val::global("JSON").call<val, std::string>("parse", j.dump());
}

val error_to_js(sqsgen::io::parse_error const& e) {
  return to_js(nlohmann::json{
      {"key", e.key},
      {"msg", e.msg},
      {"code", static_cast<int>(e.code)},
      {"parameter", e.parameter},
  });
}

val parse_result_to_js(sqsgen::io::parse_result<nlohmann::json> const& r) {
  return r.failed() ? error_to_js(r.error()) : to_js(r.result());
}

val parse_config(std::string const& v) {
  using namespace sqsgen;
  using namespace sqsgen::io;
  using namespace sqsgen::io::config;

  nlohmann::json json = nlohmann::json::parse(v);
  return parse_result_to_js(parse_config(json).collapse<nlohmann::json>(
      [](auto&& config) -> parse_result<nlohmann::json> { return {to_json(config)}; }));
}

EMSCRIPTEN_BINDINGS(m) {
  // Core optimization function

  // Helper functions
  function("parseConfig", &parse_config);
}
