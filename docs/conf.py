# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import os
import json
import datetime


YEAR = datetime.datetime.now().year
THIS_DIR = os.path.abspath(os.path.dirname(__file__))
VCPKG_JSON = os.path.abspath(os.path.join(THIS_DIR, "..", "vcpkg.json"))

with open(VCPKG_JSON, "r", encoding="utf-8") as f:
    vcpkg_data = json.load(f)
    release = vcpkg_data.get("version-string", "0.0.0")


project = "sqsgenerator"
copyright = f"{YEAR}, Dominik Gehringer"
author = "Dominik Gehringer"


# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = ["sphinx_inline_tabs", "myst_parser"]


templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]


# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "furo"
html_static_path = ["_static"]
html_logo = "logo_large.svg"

html_short_title = "sqsgenerator"

html_favicon = "favicon.ico"
