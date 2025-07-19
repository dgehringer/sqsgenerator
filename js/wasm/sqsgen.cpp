#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#include <fmt/format.h>

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

template <string_literal Name, class T>
  requires std::is_arithmetic_v<T>
constexpr auto format_prec() {
  if constexpr (std::is_same_v<double, T>)
    return (Name + string_literal("Double"));
  else if constexpr (std::is_same_v<float, T>)
    return (Name + string_literal("Float"));
};

template <class T> nlohmann::json to_json(T&& value) {
  nlohmann::json j = value;
  return j;
}

template <class T> T from_json(nlohmann::json const& j) { return j.get<T>(); }

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

val parse_config(val const& v) {
  using namespace sqsgen;
  using namespace sqsgen::io;
  using namespace sqsgen::io::config;

  nlohmann::json json = to_internal(v);
  return parse_result_to_js(parse_config(json).collapse<nlohmann::json>(
      [](auto&& config) -> parse_result<nlohmann::json> { return {to_json(config)}; }));
}

val optimize(val const& config, sqsgen::Prec prec, val const& cb) {
  using namespace sqsgen;
  using namespace sqsgen::io::config;

  val::promise_type promise;
  auto shared_cb
      = std::make_shared<val>(cb.isUndefined() ? val::undefined() : cb.call<val>("bind", cb));

  std::thread([promise, prec, shared_cb, config_json = to_internal(config)]() mutable {
    std::variant<configuration<float>, configuration<double>> run_config;
    if (prec == PREC_SINGLE)
      run_config = from_json<configuration<float>>(config_json);
    else
      run_config = from_json<configuration<double>>(config_json);
    thread_local ProxyingQueue proxying_queue_main;

    std::optional<sqs_callback_t> callback;
    if (!shared_cb->isUndefined())
      callback = [&](auto&& ctx) {
        thread_local ProxyingQueue proxying_queue;
        nlohmann::json stats = std::visit(
            [](auto&& callback_ctx) -> nlohmann::json { return to_json(callback_ctx.statistics); },
            ctx);
        proxying_queue.proxyAsync(
            emscripten_main_runtime_thread_id(),
            [shared_cb, stats = std::move(stats), ctx]() mutable {
              val received = (*shared_cb)(to_js(stats));
              if (!received.isUndefined())
                if (received.typeOf().as<std::string>() == "boolean")
                  if (received.as<bool>())
                    std::visit([](auto&& callback_ctx) { callback_ctx.stop(); }, ctx);
            });
      };
    try {
      auto result_pack_json
          = std::visit([](auto&& result_pack) -> nlohmann::json { return to_json(result_pack); },
                       run_optimization(std::move(run_config), spdlog::level::warn, callback));
      proxying_queue_main.proxyAsync(
          emscripten_main_runtime_thread_id(),
          [promise, result_pack_json = std::move(result_pack_json)]() mutable {
            promise.return_value(to_js(result_pack_json));
          });
    } catch (std::exception const& e) {
      proxying_queue_main.proxyAsync(emscripten_main_runtime_thread_id(),
                                     [promise, message = std::string(e.what())]() mutable {
                                       promise.reject_with(val(message));
                                     });
    }
  }).detach();

  return promise.get_return_object();
}

EMSCRIPTEN_BINDINGS(m) {
  // Core optimization function
  register_optional<val>();

  enum_<sqsgen::Prec>("Prec")
      .value("single", sqsgen::Prec::PREC_SINGLE)
      .value("double", sqsgen::Prec::PREC_DOUBLE);

  // Helper functions
  function("parseConfig", &parse_config);
  function("optimizeAsync", &optimize);
}

int main() {
  emscripten_exit_with_live_runtime();
  return EXIT_SUCCESS;
}
