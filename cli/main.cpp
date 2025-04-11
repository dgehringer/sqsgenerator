
#include <argparse/argparse.hpp>
#include <fstream>
#include <termcolor/termcolor.hpp>

#include "progress.h"
#include "sqsgen/core/helpers.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/config/combined.h"
#include "sqsgen/io/structure.h"
#include "sqsgen/sqs.h"

#define stringify_helper(x) #x
#define stringify(x) stringify_helper(x)

using json = nlohmann::json;

namespace ranges = std::ranges;

std::string read_file(const std::string_view filename) {
  std::ifstream ifs(filename);
  if (!ifs) throw std::runtime_error(std::format("Could not open file: {}", filename));
  std::ostringstream oss;
  oss << ifs.rdbuf();
  return oss.str();
}

namespace views = ranges::views;

template <class CharT> using formatter_t
    = std::function<std::basic_ostream<CharT>(std::basic_ostream<CharT>&)>;

std::string format_hyperlink(std::string_view text, std::string_view link) {
  return std::format("\033]8;;{}\007{}\033]8;;\007", link, text);
}

void display_version_info() {
  using namespace termcolor;
  const auto print_row = [](std::string_view label, std::string_view value,
                            std::optional<std::string_view> link = std::nullopt) {
    std::cout << green << bold << std::format("{}: ", label) << reset
              << (link.has_value() ? format_hyperlink(value, link.value()) : value) << std::endl;
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

void render_error(std::string_view message, bool exit = true,
                  std::optional<std::string> parameter = std::nullopt,
                  std::optional<std::string> info = std::nullopt) {
  using namespace termcolor;
  std::cout << red << bold << underline << "Error:" << reset << " " << message << std::endl;
  if (info.has_value())
    std::cout << "      (" << blue << italic << "info: " << reset << italic << info.value() << reset
              << ")" << std::endl;
  if (parameter.has_value())
    std::cout
        << bold << blue << "Help: " << reset
        << std::format(
               "the documentation for parameter \"{}\" is available at: {}", parameter.value(),
               format_hyperlink(
                   std::format(
                       "https://sqsgenerator.readthedocs.io/en/latest/input_parameters.html#{}",
                       parameter.value()),
                   std::format(
                       "https://sqsgenerator.readthedocs.io/en/latest/input_parameters.html#{}",
                       parameter.value())))
        << std::endl;
  if (exit) std::exit(1);
}

void run_main(std::string_view input, std::string_view output, std::string_view log_level,
              bool quiet) {
  using namespace sqsgen;
  using namespace sqsgen::core;
  using namespace sqsgen::core::helpers;

  if (!std::filesystem::exists(input)) render_error(std::format("File '{}' does not exist", input));

  std::string data;
  try {
    data = read_file(input);
  } catch (const std::exception& e) {
    render_error(std::format("Error reading file '{}'", input), true, std::nullopt, e.what());
  }

  nlohmann::json config_json;
  try {
    config_json = json::parse(data);
  } catch (nlohmann::json::parse_error& e) {
    render_error(std::format("'{}' is not a valid JSON file", input), true, std::nullopt, e.what());
  }

  auto conf = io::config::parse_config(config_json);
  if (conf.ok()) {
    auto total = std::visit([](auto&& config) { return config.iterations; }, conf.result());

    auto progress = cli::Progress("Progress", total.value(), 50);
    sqs_callback_t callback = [total = total.value(), &progress](auto ctx) {
      auto finished = std::visit([](auto&& c) { return c.statistics.finished; }, ctx);
      progress.set_progress(finished);
    };

    auto result = run_optimization(conf.result(), spdlog::level::info, callback);
    progress.set_progress(total.value());
    progress.rendered(std::cout, true);
    std::vector<uint8_t> dump;
    std::visit(
        [&dump](auto&& r) {
          nlohmann::json j = r;
          dump = nlohmann::json::to_msgpack(j);
        },
        result);
    std::ofstream out(output, std::ios::out | std::ios::binary);
    out.write(reinterpret_cast<const std::ostream::char_type*>(dump.data()), dump.size());
  } else {
    auto err = conf.error();
    render_error(err.msg, true, err.key);
  }
}

int main(int argc, char** argv) {
  using namespace sqsgen;

  using namespace sqsgen::core;
  using namespace sqsgen::core::helpers;

  argparse::ArgumentParser program(
      "sqsgen",
      std::format("{}.{}.{}", stringify(SQSGEN_MAJOR_VERSION), stringify(SQSGEN_MINOR_VERSION),
                  stringify(SQSGEN_BUILD_NUMBER)),
      argparse::default_arguments::help);

  program.add_argument("-v", "--version")
      .help("Display version info")
      .default_value(false)
      .implicit_value(true);

  program.add_argument("-q", "--quiet")
      .help("Suppress all output")
      .default_value(false)
      .implicit_value(true);

  program.add_argument("config")
      .help("The configuration file to use")
      .default_value("sqs.json")
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
  // "A simple tool to create special-quasirandom-structures (SQS)"

  try {
    program.parse_args(argc, argv);
  } catch (const std::exception& err) {
    std::cerr << err.what() << std::endl << std::endl << program;
    return EXIT_FAILURE;
  }

  if (program["--version"] == true) {
    display_version_info();
    return 0;
  }

  run_main(program.get<std::string>("config"), program.get<std::string>("output"),
           program.get<std::string>("log"), program["--quiet"] == true);

  return 0;
}
