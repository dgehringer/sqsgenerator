#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include <emscripten/proxying.h>
#include <emscripten/threading.h>

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

template <string_literal Name, class T> void bind_statistics_data() {
  class_<sqsgen::sqs_statistics_data<T>>(format_prec<Name, T>().c_str())
      .property("finished", &sqsgen::sqs_statistics_data<T>::finished)
      .property("working", &sqsgen::sqs_statistics_data<T>::working)
      .property("best_rank", &sqsgen::sqs_statistics_data<T>::best_rank)
      .property("best_objective", &sqsgen::sqs_statistics_data<T>::best_objective);
}

template <string_literal Name, class T> void bind_callback_context() {
  class_<sqsgen::sqs_callback_context<T>>(format_prec<Name, T>().c_str())
      .function("stop", &sqsgen::sqs_callback_context<T>::stop)
      .property("statistics", &sqsgen::sqs_callback_context<T>::statistics);
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

  auto config_json = to_internal(config);
  std::variant<configuration<float>, configuration<double>> run_config;
  if (prec == PREC_SINGLE)
    run_config = from_json<configuration<float>>(config_json);
  else
    run_config = from_json<configuration<double>>(config_json);

  val::promise_type promise;

  std::thread([&promise, run_config = std::move(run_config), cb]() mutable {
    thread_local ProxyingQueue proxying_queue;
    try {
      val keepaliveCb = cb.isUndefined() ? val::undefined() : cb.call<val>("bind", cb);
      std::optional<sqs_callback_t> callback;
      if (!keepaliveCb.isUndefined())
        callback = [&](auto&& ctx) {
          proxying_queue.proxyAsync(emscripten_main_runtime_thread_id(), [keepaliveCb, &ctx]() {
            std::visit([&keepaliveCb](auto&& cb_ctx) { keepaliveCb(cb_ctx); }, ctx);
          });
        };

      val result_pack = to_js(
          std::visit([](auto&& result_pack) -> nlohmann::json { return to_json(result_pack); },
                     run_optimization(std::move(run_config), spdlog::level::warn, callback)));

      proxying_queue.proxyAsync(emscripten_main_runtime_thread_id(),
                                [&promise, result_pack]() { promise.return_value(result_pack); });
    } catch (...) {
      std::exception_ptr eptr = std::current_exception();
      proxying_queue.proxyAsync(emscripten_main_runtime_thread_id(), [&promise, eptr]() {
        try {
          if (eptr) std::rethrow_exception(eptr);
        } catch (const std::exception& e) {
          promise.reject_with(val(e.what()));
        }
      });
    }
  }).detach();

  return promise.get_return_object();
}

val optimize_two(val const& config, sqsgen::Prec prec, val const& cb) {
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
            [shared_cb, stats = std::move(stats)]() { (*shared_cb)(to_js(stats)); });
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

  bind_statistics_data<"SqsStatisticsData", float>();
  bind_statistics_data<"SqsStatisticsData", double>();

  bind_callback_context<"SqsCallbackContext", float>();
  bind_callback_context<"SqsCallbackContext", double>();

  // Helper functions
  function("parseConfig", &parse_config);
  function("optimize", &optimize, return_value_policy::take_ownership());
  function("optimizeAsync", &optimize_two);
}

int main() {
  emscripten_exit_with_live_runtime();
  return EXIT_SUCCESS;
}
