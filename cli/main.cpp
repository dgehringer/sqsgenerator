
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
#include "templates.h"
#include "termcolor.h"

#define stringify_helper(x) #x
#define stringify(x) stringify_helper(x)

using json = nlohmann::json;

namespace ranges = std::ranges;
using namespace sqsgen;

namespace views = ranges::views;

auto format_cyan(std::string_view str) {
  using namespace termcolor;
  std::stringstream ss;
  ss << colorize << cyan << bold << str << reset;
  return ss.str();
};

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

using result_packt_t = std::variant<core::sqs_result_pack<float, SUBLATTICE_MODE_SPLIT>,
                                    core::sqs_result_pack<double, SUBLATTICE_MODE_SPLIT>,
                                    core::sqs_result_pack<float, SUBLATTICE_MODE_INTERACT>,
                                    core::sqs_result_pack<double, SUBLATTICE_MODE_INTERACT>>;
result_packt_t load_result_pack(std::string const& path, Prec prec = PREC_SINGLE) {
  using namespace sqsgen;
  auto pack_json = cli::read_msgpack(path);
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

  auto index = 0;
  cli::table<"INDEX", "OBJ.", "N">::render(pack.results | views::transform([&](auto&& pair) {
                                             auto [objective, results] = pair;
                                             return std::array{format_cyan(std::to_string(index++)),
                                                               std::format("{:.5f} ", objective),
                                                               std::to_string(results.size())};
                                           }));
}

void render_template_overview() {
  cli::table<"NAME", "AUTHOR(S)", "TAGS">::render(
      templates::detail::templates
      | views::transform([](auto&& pair) -> std::array<std::string, 3> {
          auto [name, template_config] = pair;

          auto format_author = [](auto&& author) -> std::string {
            if (author.name.has_value() && author.surname.has_value())
              return std::format(
                  "{} {}{}", author.name.value(), author.surname.value(),
                  author.email.has_value() ? std::format(" ({})", author.email.value()) : "");
            return "";
          };

          return std::array{
              format_cyan(name),
              template_config.authors.empty() ? "" : format_author(template_config.authors.front()),
              cli::join(template_config.tags, ", ")};
        }));
}

void render_template(templates::config_template const& tpl) {
  using namespace termcolor;

  auto can_display_author = [](templates::config_template_author const& author) -> bool {
    return (author.name.has_value() && author.surname.has_value()) || author.email.has_value();
  };

  auto format_author = [&](templates::config_template_author const& author) {
    if (can_display_author(author)) {
      if (author.name.has_value() && author.surname.has_value())
        std::cout << italic << "  Name: " << reset
                  << std::format("{} {}", author.name.value(), author.surname.value()) << std::endl;
      if (author.email.has_value())
        std::cout << italic << "  Mail: " << reset
                  << cli::format_hyperlink(author.email.value(),
                                           std::format("mailto:{}", author.email.value()))
                  << std::endl;
      if (author.affiliation.has_value())
        std::cout << italic << "  Affilitation: " << reset << author.affiliation.value()
                  << std::endl;
      std::cout << std::endl;
    }
  };

  std::cout << bold << "Template: " << format_cyan(tpl.name) << std::endl;

  std::cout << std::endl;
  std::cout << bold << "Description: " << reset << tpl.description << std::endl;
  if (!tpl.authors.empty() && ranges::any_of(tpl.authors, can_display_author)) {
    std::cout << bold << "Authors: " << reset << std::endl;
    ranges::for_each(tpl.authors, format_author);
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

  auto conf = io::config::parse_config(cli::read_json(input));
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

  argparse::ArgumentParser template_command("template", version_string,
                                            argparse::default_arguments::help);
  template_command.add_argument("-v", "--version")
      .help("Display version info")
      .default_value(false)
      .implicit_value(true);
  template_command.add_argument("name")
      .help("name of the template to use")
      .nargs(argparse::nargs_pattern::optional);
  template_command.add_description("use templates to create a new configuration file quickly");

  program.add_subparser(template_command);

  argparse::ArgumentParser output_command("output", version_string,
                                          argparse::default_arguments::all);
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

  output_structure_command.add_argument("-p", "--print")
      .help("print to stdout instead of writing it into a file")
      .default_value(false)
      .implicit_value(true);

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
    if (template_command["--version"] == true) {
      display_version_info();
      return EXIT_SUCCESS;
    }
    if (template_command.is_used("name")) {
      auto name = template_command.get<std::string>("name");
      if (!templates::templates().contains(name))
        cli::render_error(
            std::format("Cannot find a template with name '{}'. Use \"sqsgen template\" to display "
                        "all packaged templates",
                        name));
      auto tpl = templates::templates().at(name);
      render_template(tpl);
      return EXIT_SUCCESS;
    }

    render_template_overview();
    return EXIT_SUCCESS;
  }

  if (program.is_subcommand_used("output")) {
    if (output_command["--version"] == true) {
      display_version_info();
      return EXIT_SUCCESS;
    }
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

      ranges::for_each(structure_indices, [&](auto&& indices) {
        auto [objective_index, structure_index] = indices;
        std::visit(
            [&, objective_index,
             structure_index]<class T, SublatticeMode Mode>(sqs_result_pack<T, Mode> const& p) {
              auto structure
                  = std::get<1>(p.results.at(objective_index)).at(structure_index).structure();
              auto format = output_structure_command.get<std::string>("--format");
              auto basename = std::filesystem::path(output_command.get<std::string>("--output"))
                                  .stem()
                                  .string();

              std::map<std::string, std::pair<StructureFormat, std::string>> formats{
                  {"vasp", {STRUCTURE_FORMAT_POSCAR, "poscar"}},
                  {"poscar", {STRUCTURE_FORMAT_POSCAR, "poscar"}},
                  {"pymatgen", {STRUCTURE_FORMAT_JSON_PYMATGEN, "pymatgen.json"}},
                  {"sqsgen", {STRUCTURE_FORMAT_JSON_SQSGEN, "sqsgen.json"}},
                  {"cif", {STRUCTURE_FORMAT_CIF, "cif"}},
                  {"ase", {STRUCTURE_FORMAT_JSON_ASE, "ase.json"}}};

              auto result = formats.find(format);
              if (result != formats.end()) {
                auto [format, ext] = result->second;
                auto text = io::format(structure, format);
                std::string filename
                    = std::format("{}-{}-{}.{}", basename, objective_index, structure_index, ext);
                if (output_structure_command["--print"] == true) {
                  if (structure_indices.size() > 1) std::cout << std::format("# {}", filename);
                  std::cout << text << std::endl;
                  if (structure_indices.size() > 1) std::cout << std::endl;
                } else {
                  std::ofstream out(filename, std::ios::out);
                  if (!out.good())
                    cli::render_error(std::format("Failed to open output file '{}'", filename));
                  out << text;
                }
              } else
                cli::render_error(std::format("Invalid format '{}'", format), true);
            },
            pack);
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
