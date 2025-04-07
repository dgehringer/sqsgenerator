#include <cxxopts.hpp>
#include <fstream>
#include <ftxui/dom/elements.hpp>  // for text, operator|, bold, Fit, hbox, Element
#include <memory>                  // for allocator

#include "ftxui/dom/node.hpp"       // for Render
#include "ftxui/screen/color.hpp"   // for ftxui
#include "ftxui/screen/screen.hpp"  // for Full, Screen
#include "sqsgen/core/helpers.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/config/combined.h"
#include "sqsgen/io/structure.h"
#include "sqsgen/sqs.h"

#define stringify_helper(x) #x
#define stringify(x) stringify_helper(x)

using json = nlohmann::json;

namespace ranges = std::ranges;

std::string read_file(const std::string& filename) {
  std::ifstream ifs(filename);
  if (!ifs) throw std::runtime_error(std::format("Could not open file: {}", filename));
  std::ostringstream oss;
  oss << ifs.rdbuf();
  return oss.str();
}

namespace views = ranges::views;

void display_version_info() {
  using namespace ftxui;

  const auto make_row = [](std::string_view label, std::string_view value,
                           std::optional<std::string_view> link = std::nullopt) {
    auto val = text(std::format("{}", value));
    return hbox({
        text(std::format("{}: ", label)) | bold | color(Color::Green),
        link.has_value() ? val | hyperlink(std::format("{}", link.value())) : val,
    });
  };

  auto document =  //
      vbox({hbox({text("sqsgen") | bold | italic,
                  text(" - A simple, fast and efficient tool to find optimized "),
                  text("S") | italic, text("pecial-"), text("Q") | italic, text("uasirandom-"),
                  text("S") | italic, text("tructures ("), text("SQS") | italic, text(")")}),
            hbox({text("")}),
            make_row("Version", std::format("{}.{}.{}", SQSGEN_MAJOR_VERSION, SQSGEN_MINOR_VERSION,
                                            SQSGEN_BUILD_NUMBER)),
            make_row("Build Info",
                     std::format("{}@{}", stringify(SQSGEN_BUILD_BRANCH),
                                 stringify(SQSGEN_BUILD_COMMIT)),
                     std::format("https://github.com/dgehringer/sqsgenerator/commit/{}",
                                 stringify(SQSGEN_BUILD_COMMIT))),
            make_row("Repository", "dgehringer/sqsgenerator",
                     "https://github.com/dgehringer/sqsgenerator"),
            make_row("Author", "Dominik Gehringer", "mailto:dgehringer@gmail.com"),
            make_row("Docs", "https://sqsgenerator.readthedocs.io/en/latest",
                     "https://sqsgenerator.readthedocs.io/en/latest"),
            make_row("MPI", sqsgen::io::mpi::HAVE_MPI ? "yes" : "no")});

  auto screen = Screen::Create(Dimension::Full(), Dimension::Fit(document));
  Render(screen, document);
  screen.Print();
}

int main(int argc, char** argv) {
  using namespace sqsgen;

  using namespace sqsgen::core;
  using namespace sqsgen::core::helpers;

  cxxopts::Options options("sqsgen",
                           "A simple tool to create special-quasirandom-structures (SQS)");

  options.add_options()("config", "The path to the JSON config",
                        cxxopts::value<std::string>()->default_value("sqs.json"))(
      "h,help", "Print usage and help info")("version", "Display version info");

  options.parse_positional({"config"});

  auto result = options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    exit(0);
  }
  if (result.count("version")) {
    display_version_info();
    exit(0);
  }

  return 0;

  std::string data = read_file("sqs.json");
  json j = json::parse(data);
  auto conf = io::config::parse_config(j);
  if (conf.ok()) {
    auto result = run_optimization(conf.result());

    /*std::vector<uint8_t> dump;
    std::visit(
        [&dump](auto &&r) {
          nlohmann::json j = r;
          dump = nlohmann::json::to_msgpack(j);
        },
        result);
    std::ofstream out("sqs.bin", std::ios::out | std::ios::binary);
    out.write(reinterpret_cast<const std::ostream::char_type *>(dump.data()), dump.size());
    */
  }
}
