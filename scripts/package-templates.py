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
    name = author["name"] if "name" in author else "std::nullopt"
    surname = author["surname"] if "surname" in author else "std::nullopt"
    email = author["email"] if "email" in author else "std::nullopt"
    return """
config_template_author{{
    \"{name}\",
    \"{surname}\",
    \"{email}\",
   {affiliation}
}}""".format(
        name=name,
        surname=surname,
        email=email,
        affiliation=(
            'std::make_optional("{doi}")'.format(doi=author["affiliation"])
            if "doi" in author
            else "std::nullopt"
        ),
    )


def load_template(filename: str):
    with open(filename) as f:
        t = json.load(f)
    template_code = """
config_template {{
\"{name}\",
\"{description}\",
std::vector<std::string>{{{tags}}},
{doi},
std::vector{{{authors}}},
nlohmann::json::parse(R"({json})")
}}
""".format(
        name=t["name"],
        description=t["description"],
        json=json.dumps(t["config"]),
        authors=(", ".join(format_author(a) for a in t.get("authors", []))),
        tags=(", ".join('"{tag}"'.format(tag=tag) for tag in t.get("tags", []))),
        doi=(
            'std::make_optional("{doi}")'.format(doi=t["doi"])
            if "doi" in t
            else "std::nullopt"
        ),
    )
    return t["name"], template_code


def file_template(templates):
    macro_name = (
        "{ns}::{fname}".format(ns=NAMESPACE, fname=FILENAME)
        .upper()
        .replace("::", "_")
        .replace(".", "_")
    )

    return """
#ifndef {macro_name}
#define {macro_name}

{includes}

namespace {namespace} {{

    struct config_template_author {{
        std::optional<std::string> name;
        std::optional<std::string> surname;
        std::optional<std::string> email;
        std::optional<std::string> affiliation;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(config_template_author, name, surname, email, affiliation);
    }};

    struct config_template {{
        std::string name;
        std::string description;
        std::vector<std::string> tags;
        std::optional<std::string> doi;
        std::vector<config_template_author> authors;
        nlohmann::json config;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(config_template, name, description, tags, authors, config);
    }};

    namespace detail {{
        static const std::map<std::string, config_template> templates = {{
         {templates}
        }};
    }} // namespace detail

  inline const std::map<std::string, config_template>& templates() {{
    return detail::templates;
  }}

}} // namespace {namespace}

#endif // {macro_name}
""".format(
        macro_name=macro_name,
        includes=("\n".join("#include {}".format(inc) for inc in INCLUDES)),
        templates=(
            ",\n".join(
                '{{"{}", {}}}'.format(*template) for template in templates.items()
            )
        ),
        namespace=NAMESPACE,
    )


if __name__ == "__main__":
    _, template_dir, *args = sys.argv
    template_dir = os.path.abspath(template_dir)
    if args:
        output_dir, *_ = args
        output_dir = os.path.abspath(output_dir)
    else:
        output_dir = os.path.abspath(os.path.join(template_dir, "..", "cli"))

    templates = dict(
        load_template(f) for f in glob.glob(os.path.join(template_dir, "*.json"))
    )

    with open(os.path.join(output_dir, FILENAME), "w") as f:
        f.write(file_template(templates))
