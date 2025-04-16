import glob
import json
import os
import sys

NAMESPACE = "sqsgen::templates"

FILENAME = "templates.h"

INCLUDES = [
    "<nlohmann/json.hpp>",
]


def format_author(author: dict) -> str:
    name = f'"{author["name"]}"' if "name" in author else "std::nullopt"
    surname = f'"{author["surname"]}"' if "surname" in author else "std::nullopt"
    email = f'"{author["email"]}"' if "email" in author else "std::nullopt"
    return f"""
config_template_author{{
    {name},
    {surname},
    {email},
    std::vector<std::string>{{{", ".join(f'"{aff}"' for aff in author.get("affiliations", []))}}}
}}"""


def load_template(filename: str):
    with open(filename) as f:
        t = json.load(f)

    return (
        t["name"],
        f"""
config_template {{
"{t["name"]}",
"{t["description"]}",
std::vector<std::string>{{{", ".join(f'"{tag}"' for tag in t.get("tags", []))}}},
std::vector<config_template_author>{{{", ".join(format_author(a) for a in t.get("authors", []))}}},
nlohmann::json::parse(R"({json.dumps(t["config"]).replace('"', '\\"')})")
}}
""",
    )


def file_template(templates):
    macro_name = f"{NAMESPACE}::{FILENAME}".replace("::", "_").replace(".", "_").upper()
    return f"""
#ifndef {macro_name}
#define {macro_name}

{"\n".join(f"#include {inc}" for inc in INCLUDES)}

namespace {NAMESPACE} {{

    struct config_template_author {{
        std::optional<std::string> name;
        std::optional<std::string> surname;
        std::optional<std::string> email;
        std::vector<std::string> affiliations;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(config_template_author, name, surname, email, affiliations);
    }};

    struct config_template {{
        std::string name;
        std::string description;
        std::vector<std::string> tags;
        std::vector<config_template_author> authors;
        nlohmann::json config;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(config_template_author, name, description, tags, authors, config);
    }};

    namespace detail {{
        static const std::map<std::string, config_template> templates = {{
         {",\n".join(f'{{"{name}", {template}}}' for name, template in templates.items())}
        }};
    }} // namespace detail

}}

#endif // {macro_name}
"""


if __name__ == "__main__":
    _, template_dir, *args = sys.argv
    template_dir = os.path.abspath(template_dir)
    if args:
        output_dir, *_ = args
        output_dir = os.path.abspath(output_dir)
    else:
        output_dir = os.path.abspath(os.path.join(template_dir, "..", "cli"))
    print(template_dir, output_dir)

    templates = dict(
        load_template(f) for f in glob.glob(os.path.join(template_dir, "*.json"))
    )

    with open(os.path.join(output_dir, FILENAME), "w") as f:
        f.write(file_template(templates))
