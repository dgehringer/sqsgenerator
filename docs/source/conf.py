# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os
import sys
import shutil


core_stubs = '../core_stubs.py'
package_dir = '../../sqsgenerator'

sys.path.insert(0, '../..')

shutil.copy(core_stubs, os.path.join(package_dir, 'core', 'core.py'))

import sqsgenerator
import sqsgenerator.io
import sqsgenerator.adapters
import sqsgenerator.settings
print(sqsgenerator)
print(sqsgenerator.io)
print(sqsgenerator.adapters)
print(sqsgenerator.settings)
# -- Project information -----------------------------------------------------

project = 'sqsgenerator'
copyright = '2021, Dominik Gehringer'
author = 'Dominik Gehringer'

# The full version, including alpha/beta/rc tags
release = '0.1'


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
myst_enable_extensions = [
    'amsmath',
    'colon_fence',
    'deflist',
    'dollarmath',
    'html_admonition',
    'html_image',
    'replacements',
    'smartquotes',
    'substitution',
    'tasklist',
]

extensions = [
    'sphinx.ext.autodoc',
    'myst_parser',
    'sphinx_click'
]


# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = []

source_suffix = {
    '.rst': 'restructuredtext',
    '.md': 'markdown',
}

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'pydata_sphinx_theme'

html_logo = 'logo_large.svg'

html_short_title = 'sqsgenerator'

html_favicon = 'favicon.ico'
# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']