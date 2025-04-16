
#include <argparse/argparse.hpp>
#include <fstream>
#include <iostream>
#include <ranges>

#include "helpers.h"
#include "progress.h"
#include "sqsgen/core/helpers.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/config/combined.h"
#include "sqsgen/io/mpi.h"
#include "sqsgen/io/structure.h"
#include "sqsgen/sqs.h"
#include "termcolor.h"

#define stringify_helper(x) #x
#define stringify(x) stringify_helper(x)

using json = nlohmann::json;

namespace ranges = std::ranges;
using namespace sqsgen;

std::string read_file(std::string const& filename) {
  std::ifstream ifs(filename);
  if (!ifs) throw std::runtime_error(std::format("Could not open file: {}", filename));
  std::ostringstream oss;
  oss << ifs.rdbuf();
  return oss.str();
}

namespace views = ranges::views;

void display_version_info() {
  using namespace termcolor;
  const auto print_row = [](std::string_view label, std::string_view value,
                            std::optional<std::string_view> link = std::nullopt) {
    std::cout << green << bold << std::format("{}: ", label) << reset
              << (link.has_value() ? cli::format_hyperlink(value, link.value()) : value)
              << std::endl;
  };

  std::cout << bold << italic << "sqsgen" << reset
            << " - A simple, fast and efficient tool to find optimized " << italic << "S" << reset
            << "pecial-" << italic << "Q" << reset << "uasirandom-" << italic << "S" << reset
            << "tructures (" << italic << "SQS" << reset << ")" << std::endl;

  std::cout << std::endl;
  print_row("Version", std::format("{}.{}.{}", SQSGEN_MAJOR_VERSION, SQSGEN_MINOR_VERSION,
                                   SQSGEN_BUILD_NUMBER));
  print_row("Build Info",
            std::format("{}@{}", stringify(SQSGEN_BUILD_BRANCH), stringify(SQSGEN_BUILD_COMMIT)),
            std::format("https://github.com/dgehringer/sqsgenerator/commit/{}",
                        stringify(SQSGEN_BUILD_COMMIT)));
  print_row("Build Date", std::format("{} {}", __DATE__, __TIME__));
  print_row("Build Ver.", std::format("{}", __VERSION__));
  print_row("Repository", "dgehringer/sqsgenerator", "https://github.com/dgehringer/sqsgenerator");
  print_row("Author", "Dominik Gehringer", "mailto:dgehringer@gmail.com");
  print_row("Docs", "https://sqsgenerator.readthedocs.io/en/latest",
            "https://sqsgenerator.readthedocs.io/en/latest");
  print_row("MPI", sqsgen::io::mpi::HAVE_MPI ? "yes" : "no");
}

nlohmann::json read_msgpack(std::string const& filename) {
  if (!std::filesystem::exists(filename))
    cli::render_error(std::format("File '{}' does not exist", filename), true);
  std::ifstream ifs(filename, std::ios::in | std::ios::binary);
  nlohmann::json config_json;
  try {
    config_json = nlohmann::json::from_msgpack(ifs);
  } catch (nlohmann::json::parse_error& e) {
    cli::render_error(std::format("'{}' is not a valid msgpack file", filename), true, std::nullopt,
                      e.what());
  }
  return config_json;
}

nlohmann::json read_json(std::string const& filename) {
  if (!std::filesystem::exists(filename)) cli::render_error("File '{}' does not exist", true);
  std::string data;
  try {
    data = read_file(filename);
  } catch (const std::exception& e) {
    cli::render_error(std::format("Error reading file '{}'", filename), true, std::nullopt,
                      e.what());
  }

  nlohmann::json config_json;
  try {
    config_json = nlohmann::json::parse(data);
  } catch (nlohmann::json::parse_error& e) {
    cli::render_error(std::format("'{}' is not a valid JSON file", filename), true, std::nullopt,
                      e.what());
  }
  return config_json;
}

using result_packt_t = std::variant<core::sqs_result_pack<float, SUBLATTICE_MODE_SPLIT>,
                                    core::sqs_result_pack<double, SUBLATTICE_MODE_SPLIT>,
                                    core::sqs_result_pack<float, SUBLATTICE_MODE_INTERACT>,
                                    core::sqs_result_pack<double, SUBLATTICE_MODE_INTERACT>>;
result_packt_t load_result_pack(std::string const& path, Prec prec = PREC_SINGLE) {
  using namespace sqsgen;
  auto pack_json = read_msgpack(path);
  if (!pack_json.contains("config"))
    cli::render_error("Invalid result pack - cannot find config", true);
  if (!pack_json["config"].contains("sublattice_mode"))
    cli::render_error("Invalid result pack - cannot find sublattice_mode", true);
  SublatticeMode mode = pack_json["config"]["sublattice_mode"].get<SublatticeMode>();
  if (prec == PREC_SINGLE && mode == SUBLATTICE_MODE_SPLIT)
    return pack_json.get<core::sqs_result_pack<float, SUBLATTICE_MODE_SPLIT>>();
  if (prec == PREC_SINGLE && mode == SUBLATTICE_MODE_INTERACT)
    return pack_json.get<core::sqs_result_pack<float, SUBLATTICE_MODE_INTERACT>>();
  if (prec == PREC_DOUBLE && mode == SUBLATTICE_MODE_SPLIT)
    return pack_json.get<core::sqs_result_pack<double, SUBLATTICE_MODE_SPLIT>>();
  if (prec == PREC_DOUBLE && mode == SUBLATTICE_MODE_INTERACT)
    return pack_json.get<core::sqs_result_pack<double, SUBLATTICE_MODE_INTERACT>>();
  throw std::invalid_argument("Invalid result pack - invalid sublattice_mode");
}

template <class T, SublatticeMode Mode>
void show_result_pack(core::sqs_result_pack<T, Mode> const& pack) {
  using namespace termcolor;

  std::cout << bold << "Mode: " << reset << italic
            << (Mode == SUBLATTICE_MODE_INTERACT ? "interact" : "split") << reset << std::endl;
  std::cout << bold << "min(O(σ)): " << reset
            << std::format("{:.5f}", pack.statistics.best_objective) << std::endl;
  std::cout << bold << "Num. objectives: " << reset << pack.results.size() << std::endl;
  std::cout << std::endl;
  std::cout << bold << "index" << " O(σ)        " << "N" << std::endl;
  auto index = 0;
  for (auto&& [objective, results] : pack.results) {
    std::cout << bold << std::format("{:<5} ", index++) << reset;
    std::cout << sqsgen::cli::pad_right(std::format("{:.5f} ", objective), 12);
    std::cout << results.size() << std::endl;
  }
}

void run_main(std::string const& input, std::string const& output, std::string const& log_level,
              bool quiet) {
  using namespace sqsgen;
  using namespace sqsgen::core;
  using namespace sqsgen::core::helpers;

  auto log_levels
      = std::map<std::string_view, spdlog::level::level_enum>{{"error", spdlog::level::critical},
                                                              {"warn", spdlog::level::warn},
                                                              {"info", spdlog::level::info},
                                                              {"debug", spdlog::level::debug},
                                                              {"trace", spdlog::level::trace}};

  if (!log_levels.contains(log_level))
    cli::render_error(std::format("Invalid log level '{}'", log_level));

  if (!std::filesystem::exists(input))
    cli::render_error(std::format("File '{}' does not exist", input));

  auto conf = io::config::parse_config(read_json(input));
  if (conf.ok()) {
    auto total = std::visit([](auto&& config) { return config.iterations; }, conf.result());

    auto progress = cli::Progress("Progress", total.value(), 50);

    std::optional<sqs_callback_t> callback = std::nullopt;
    if (!quiet)
      callback = [total = total.value(), &progress](auto&& ctx) {
        progress.set_progress(std::forward<decltype(ctx)>(ctx));
        auto finished = std::visit([](auto&& c) { return c.statistics.finished; }, ctx);
        progress.render(std::cout, finished >= total);
      };

    auto result = run_optimization(conf.result(), log_levels[log_level], callback);

#ifdef WITH_MPI
    bool should_dump{mpl::environment::comm_world().rank() == io::mpi::RANK_HEAD};
#else
    bool should_dump{true};
#endif

    if (should_dump) {
      std::vector<uint8_t> dump;
      std::visit(
          [&dump](auto&& r) {
            nlohmann::json j = r;
            dump = nlohmann::json::to_msgpack(j);
          },
          result);
      std::ofstream out(output, std::ios::out | std::ios::binary);
      out.write(reinterpret_cast<const std::ostream::char_type*>(dump.data()), dump.size());
    }
  } else {
    auto err = conf.error();
    cli::render_error(err.msg, true, err.key);
  }
}

int main(int argc, char** argv) {
  using namespace sqsgen;

  using namespace sqsgen::core;
  using namespace sqsgen::core::helpers;
  auto version_string
      = std::format("{}.{}.{}", stringify(SQSGEN_MAJOR_VERSION), stringify(SQSGEN_MINOR_VERSION),
                    stringify(SQSGEN_BUILD_NUMBER));
  argparse::ArgumentParser program("sqsgen", version_string, argparse::default_arguments::help);

  program.add_argument("-v", "--version")
      .help("Display version info")
      .default_value(false)
      .implicit_value(true);

  program.add_argument("-q", "--quiet")
      .help("Suppress all output")
      .default_value(false)
      .implicit_value(true);

  program.add_argument("-i", "--input")
      .help("The configuration file to use")
      .default_value("sqs.json")
      .implicit_value("sqs.json")
      .nargs(1);

  program.add_argument("-o", "--output")
      .help("The output file to write the results to in brinary format")
      .default_value("sqs.mpack")
      .nargs(1);

  program.add_argument("-l", "--log")
      .help("sets the log value")
      .default_value(std::string{"warn"})
      .choices("error", "warn", "info", "debug", "trace")
      .nargs(1);

  argparse::ArgumentParser template_command("template");
  template_command.add_description("use templates to create a new configuration file quickly");

  program.add_subparser(template_command);

  argparse::ArgumentParser output_command("output", version_string,
                                          argparse::default_arguments::help);
  output_command.add_description("export the results of a SQS optimization run");

  output_command.add_argument("-o", "--output")
      .help("The output file to write the results to in binary format")
      .default_value("sqs.mpack")
      .nargs(1);

  argparse::ArgumentParser output_config_command("config");
  output_config_command.add_description("export the config as a JSON file. E.g. sqs.config.json");

  argparse::ArgumentParser output_structure_command("structure", version_string,
                                                    argparse::default_arguments::help);
  output_structure_command.add_description("export the structure of a result file");

  output_structure_command.add_argument("-f", "--format")
      .help("The output format to use")
      .default_value(std::string{"sqsgen"})
      .choices("pymatgen", "ase", "vasp", "poscar", "sqsgen")
      .nargs(1);

  output_structure_command.add_argument("--objective")
      .help("select the n-th best objective")
      .default_value(std::vector<std::string>{"0"})
      .append();

  output_structure_command.add_argument("-i", "--index")
      .help("the index of the structure to export,  specified by the --objective option")
      .default_value(std::vector<std::string>{"0"})
      .append();
  output_structure_command.add_argument("--all").default_value(false).implicit_value(true).help(
      "Export all structures of a certain objective value, specified by the --objective option");

  output_command.add_subparser(output_config_command);
  output_command.add_subparser(output_structure_command);

  program.add_subparser(output_command);
  // "A simple tool to create special-quasirandom-structures (SQS)"

  try {
    program.parse_args(argc, argv);
  } catch (const std::exception& err) {
    std::cerr << err.what() << std::endl << std::endl << program;
    return EXIT_FAILURE;
  }

  if (program["--version"] == true) {
    display_version_info();
    return EXIT_SUCCESS;
  }

  if (program.is_subcommand_used("template")) {
    std::cout << template_command;
    return EXIT_SUCCESS;
  }

  if (program.is_subcommand_used("output")) {
    auto pack = load_result_pack(output_command.get<std::string>("--output"));
    if (output_command.is_subcommand_used("config")) {
      std::string output = std::format(
          "{}.config.json",
          std::filesystem::path(output_command.get<std::string>("--output")).stem().string());
      std::ofstream out(output, std::ios::out);
      if (!out.good()) cli::render_error(std::format("Failed to open output file '{}'", output));
      out << std::visit([](auto&& p) { return cli::fixup_config_json(p.config).dump(); }, pack);
      return EXIT_SUCCESS;
    } else if (output_command.is_subcommand_used("structure")) {
      auto num_objectives = std::visit([](auto&& p) { return p.results.size(); }, pack);
      auto objective_indices = helpers::as<std::set>{}(
          output_structure_command.get<std::vector<std::string>>("--objective")
          | views::transform(
              [num_objectives](auto&& raw) { return cli::validate_index(raw, num_objectives); }));
      bool export_all = output_structure_command["--all"] == true;
      auto structure_indices = std::vector<std::pair<int, int>>{};
      ranges::for_each(objective_indices, [&, export_all](auto&& index) {
        int num_structures = std::visit(
            [index](auto& p) { return std::get<1>(p.results.at(index)).size(); }, pack);
        const auto append = [&, index](int i) { structure_indices.push_back({index, i}); };
        if (export_all)
          ranges::for_each(range<int>(num_structures), append);
        else
          ranges::for_each(output_structure_command.get<std::vector<std::string>>("--index"),
                           [&](auto&& raw) { append(cli::validate_index(raw, num_structures)); });
      });

      return EXIT_SUCCESS;
    } else {
      std::visit([](auto const& p) { show_result_pack(p); }, pack);
      return EXIT_FAILURE;
    }
  }

  std::string output = program.get<std::string>("--output");
  if (program.is_used("--input") && !program.is_used("--output")) {
    // The user has specified a custom input file we try to split the extension
    output = std::format(
        "{}.mpack", std::filesystem::path(program.get<std::string>("--input")).stem().string());
  }

  run_main(program.get<std::string>("--input"), output, program.get<std::string>("--log"),
           program["--quiet"] == true);

  return 0;
}
