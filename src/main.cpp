//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//
// Minimalistic debug/run CLI. This is the only target that may be built with
// MPI (see CMakeLists.txt); the single-process path contains no MPI code and
// MPI differs only in how the run is dispatched.
//

#include <argparse/argparse.hpp>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stop_token>
#include <string>
#include <string_view>
#include <variant>

#include "sqsgen/configuration.h"
#include "sqsgen/io/glaze.h"
#include "sqsgen/log.h"
#include "sqsgen/optimization/optimization.h"
#include "sqsgen/optimization/runner.h"
#include "sqsgen/types.h"

#ifdef SQSGEN_MPI
#  include <mpi.h>

#  include "sqsgen/optimization/mpi/runner.h"
#endif

namespace {

  namespace fs = std::filesystem;
  namespace cfg = sqsgen::configuration;
  namespace io = sqsgen::io;
  namespace opt = sqsgen::optimization;
  namespace log = sqsgen::log;
  using io::wire_format;

  // SIGINT arrives on an arbitrary thread; only a sig_atomic_t flag is touched here.
  volatile std::sig_atomic_t g_interrupted = 0;
  extern "C" void on_sigint(int) { g_interrupted = 1; }

  // Map a file suffix to the wire format read_input / write_any understand.
  std::optional<wire_format> format_from_suffix(const fs::path& path) {
    std::string ext = path.extension().string();
    for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (ext == ".json") return wire_format::json;
    if (ext == ".yaml" || ext == ".yml") return wire_format::yaml;
    if (ext == ".cbor") return wire_format::cbor;
    if (ext == ".msgpack" || ext == ".mpack") return wire_format::msgpack;
    if (ext == ".beve") return wire_format::beve;
    return std::nullopt;
  }

  std::expected<std::string, std::string> read_file(const fs::path& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return std::unexpected("could not open file: " + path.string());
    std::ostringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
  }

  std::expected<void, std::string> write_file(const fs::path& path, std::string_view data) {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) return std::unexpected("could not open file for writing: " + path.string());
    ofs.write(data.data(), static_cast<std::streamsize>(data.size()));
    if (!ofs) return std::unexpected("failed to write file: " + path.string());
    return {};
  }

  std::optional<log::level> parse_log_level(std::string_view name) {
    if (name == "trace") return log::level::trace;
    if (name == "debug") return log::level::debug;
    if (name == "info") return log::level::info;
    if (name == "warn" || name == "warning") return log::level::warn;
    if (name == "error") return log::level::error;
    if (name == "off") return log::level::off;
    return std::nullopt;
  }

  // The rank that owns the terminal: rank 0 under MPI (the head), always true when serial.
  // Only this rank reads/writes files and prints to the console.
  bool is_primary() {
#ifdef SQSGEN_MPI
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    return rank == 0;
#else
    return true;
#endif
  }

  // Same optimize() call drives both modes; only the run context differs. make_mpi_run_context
  // is collective (it reduces the thread counts), so every rank must reach it.
  opt::run_context make_run_context(const cfg::thread_config_t& threads, std::stop_token stop,
                                    opt::progress_callback on_progress) {
#ifdef SQSGEN_MPI
    return opt::mpi::make_mpi_run_context(threads, std::move(stop), std::move(on_progress),
                                          MPI_COMM_WORLD);
#else
    return opt::make_local_run_context(threads, std::move(stop), std::move(on_progress));
#endif
  }

  // Read and validate an input file into a concrete configuration variant.
  std::expected<cfg::configuration_variant_t, std::string> load_config(const fs::path& input) {
    const auto fmt = format_from_suffix(input);
    if (!fmt)
      return std::unexpected("unknown input format for '" + input.string()
                             + "' (expected .json, .yaml, .cbor, .msgpack or .beve)");
    const auto data = read_file(input);
    if (!data) return std::unexpected(data.error());

    auto parsed = cfg::read_input(data.value(), fmt.value());
    if (!parsed) return std::unexpected("parse error: " + parsed.error().msg);

    auto validated = cfg::parse_configuration(parsed.value());
    if (!validated) {
      const auto& err = validated.error();
      std::string msg = "validation error";
      if (!err.key.empty()) msg += " [" + err.key + "]";
      msg += ": " + err.msg;
      return std::unexpected(msg);
    }
    return validated.value();
  }

  // ---------------------------------------------------------------- preprocess

  int cmd_preprocess(const fs::path& input, std::optional<fs::path> output) {
    // Validation/serialization is not collective; only the primary rank does it.
    if (!is_primary()) return EXIT_SUCCESS;

    auto config = load_config(input);
    if (!config) {
      std::cerr << config.error() << '\n';
      return EXIT_FAILURE;
    }

    // Default: foo.json -> foo.proc.json (insert ".proc" before the suffix).
    fs::path out_path;
    if (output) {
      out_path = *output;
    } else {
      out_path = input;
      out_path.replace_extension();
      out_path += ".proc";
      out_path += input.extension();
    }

    const auto out_fmt = format_from_suffix(out_path);
    if (!out_fmt) {
      std::cerr << "unknown output format for '" << out_path.string() << "'\n";
      return EXIT_FAILURE;
    }

    return std::visit(
        [&](const auto& conf) -> int {
          auto dto = conf.to_input();
          auto dumped = io::write_any<decltype(dto)>(dto, out_fmt.value());
          if (!dumped) {
            std::cerr << "failed to serialize configuration: " << dumped.error().msg << '\n';
            return EXIT_FAILURE;
          }
          if (auto written = write_file(out_path, dumped.value()); !written) {
            std::cerr << written.error() << '\n';
            return EXIT_FAILURE;
          }
          std::cerr << "wrote " << out_path.string() << '\n';
          return EXIT_SUCCESS;
        },
        config.value());
  }

  // ----------------------------------------------------------------------- run

  void render_progress(const opt::progress& p) {
    constexpr int width = 30;
    double frac = p.total ? static_cast<double>(p.done) / static_cast<double>(p.total) : 0.0;
    if (frac > 1.0) frac = 1.0;
    const int filled = static_cast<int>(frac * width);
    std::string bar(static_cast<std::size_t>(filled), '#');
    bar.resize(width, ' ');

    std::ostringstream line;
    line << "\rOptimizing [" << bar << "] " << std::fixed << std::setprecision(1) << (frac * 100.0)
         << "%  " << p.done << "/" << p.total;
    if (p.rate > 0.0) line << std::setprecision(3) << "  " << p.rate << " it/s";
    if (p.best) line << std::setprecision(6) << "  best=" << *p.best;
    if (p.remaining_seconds)
      line << std::setprecision(1) << "  eta=" << *p.remaining_seconds << "s";
    std::cerr << line.str() << std::flush;
  }

  int cmd_run(const fs::path& input, bool quiet, log::level level) {
    log::set_level(level);

    auto config = load_config(input);
    if (!config) {
      if (is_primary()) std::cerr << config.error() << '\n';
      return EXIT_FAILURE;
    }

    std::stop_source stop;
    std::signal(SIGINT, on_sigint);

    // Fires on the scheduler's main thread, once per progress interval: it is the
    // place both to draw the bar and to honour a pending Ctrl-C. Under MPI only the
    // head gets this callback (the factory drops it for workers).
    opt::progress_callback on_progress = [&](const opt::progress& p) {
      if (g_interrupted) stop.request_stop();
      if (!quiet) render_progress(p);
    };

    return std::visit(
        [&](const auto& conf) -> int {
          const auto context = make_run_context(conf.thread_config, stop.get_token(), on_progress);
          if (is_primary()) log::info("starting optimization on {} thread(s)", context.num_threads);

          // optimize() is collective under MPI: every rank calls it; only the head
          // comes back with results.
          auto out = opt::optimize(conf, context);
          if (!is_primary()) return EXIT_SUCCESS;
          if (!quiet) std::cerr << '\n';

          std::size_t num_results = 0;
          for (const auto& [objective, group] : out.results) num_results += group.size();

          std::cout << "iterations : " << out.stats.iterations << '\n'
                    << "hits       : " << out.stats.hits << '\n'
                    << "seconds    : " << std::fixed << std::setprecision(3) << out.stats.seconds
                    << '\n'
                    << "results    : " << num_results << '\n';
          if (!out.results.empty())
            std::cout << "best       : " << std::setprecision(6) << out.results.front().first
                      << '\n';
          if (stop.stop_requested()) std::cerr << "run was interrupted\n";
          return EXIT_SUCCESS;
        },
        config.value());
  }

}  // namespace

int main(int argc, char** argv) {
#ifdef SQSGEN_MPI
  // optimize() runs compute on std::jthreads while the main thread drives MPI, so the
  // head/workers need at least MPI_THREAD_FUNNELED (only the main thread calls MPI).
  int provided = 0;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
  if (provided < MPI_THREAD_FUNNELED && is_primary())
    std::cerr << "warning: MPI runtime does not provide MPI_THREAD_FUNNELED\n";
#endif

  argparse::ArgumentParser program("sqsgen");

  argparse::ArgumentParser preprocess_cmd("preprocess");
  preprocess_cmd.add_description("validate an input and write back its canonical form");
  preprocess_cmd.add_argument("-i", "--input").required().help("input configuration file");
  preprocess_cmd.add_argument("-o", "--output")
      .help("output file (default: <input>.proc.<suffix>)");

  argparse::ArgumentParser run_cmd("run");
  run_cmd.add_description("run the optimization described by an input file");
  run_cmd.add_argument("-i", "--input").required().help("input configuration file");
  run_cmd.add_argument("-q", "--quiet").flag().help("suppress the progress indicator");
  run_cmd.add_argument("-l", "--log")
      .default_value(std::string("warn"))
      .help("log level: trace, debug, info, warn, error or off");

  program.add_subparser(preprocess_cmd);
  program.add_subparser(run_cmd);

  int status = EXIT_SUCCESS;
  try {
    program.parse_args(argc, argv);

    if (program.is_subcommand_used(preprocess_cmd)) {
      std::optional<fs::path> output;
      if (auto o = preprocess_cmd.present("--output")) output = fs::path(*o);
      status = cmd_preprocess(fs::path(preprocess_cmd.get<std::string>("--input")), output);
    } else if (program.is_subcommand_used(run_cmd)) {
      const auto level = parse_log_level(run_cmd.get<std::string>("--log"));
      if (!level) {
        std::cerr << "unknown log level: " << run_cmd.get<std::string>("--log") << '\n';
        status = EXIT_FAILURE;
      } else {
        status = cmd_run(fs::path(run_cmd.get<std::string>("--input")),
                         run_cmd.get<bool>("--quiet"), level.value());
      }
    } else {
      if (is_primary()) std::cerr << program;
      status = EXIT_FAILURE;
    }
  } catch (const std::exception& e) {
    if (is_primary()) std::cerr << e.what() << '\n';
    status = EXIT_FAILURE;
  }

#ifdef SQSGEN_MPI
  MPI_Finalize();
#endif
  return status;
}
