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

extensions = [
    "sphinx_inline_tabs",
    "myst_parser",
    "sphinx.ext.intersphinx",
    "sphinx_design",
    "sphinx_click",
]


myst_enable_extensions = [
    "amsmath",
    "colon_fence",
    "deflist",
    "dollarmath",
    "html_admonition",
    "html_image",
    "replacements",
    "smartquotes",
    "substitution",
    "tasklist",
]

intersphinx_mapping = {
    "python": ("https://docs.python.org/3/", None),
    "numpy": ("https://numpy.org/doc/stable/", None),
    "scipy": ("https://docs.scipy.org/doc/scipy/", None),
    "matplotlib": ("https://matplotlib.org/stable/", None),
}

myst_dmath_allow_labels = True
html_show_sourcelink = True

templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]


# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "furo"
html_static_path = ["_static"]
html_logo = "images/logo_large.svg"
html_short_title = "sqsgenerator"
html_favicon = "favicon.ico"
html_css_files = [
    "custom.css",
]


def setup(app):
    # Inject our JavaScript
    app.add_js_file("run-button.js")
