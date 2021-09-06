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
import wheel.wheelfile
import requests

wheels = {
    '3.6': ('sqsgenerator-0.1-cp36-cp36m-linux_x86_64.whl', 'http://oc.unileoben.ac.at/index.php/s/qqDIydH02PkV32V/download'),
    '3.7': ('sqsgenerator-0.1-cp37-cp37m-linux_x86_64.whl', 'http://oc.unileoben.ac.at/index.php/s/34xlYhyZxkyb6xy/download'),
    '3.8': ('sqsgenerator-0.1-cp38-cp38-linux_x86_64.whl', 'http://oc.unileoben.ac.at/index.php/s/gTk345lGTwk3C0G/download'),
    '3.9': ('sqsgenerator-0.1-cp39-cp39-linux_x86_64.whl', 'http://oc.unileoben.ac.at/index.php/s/3x01KBKarx11BgQ/download'),
}

package_dir = '../deps'
package_name = 'sqsgenerator'
os.makedirs(package_dir, exist_ok=True)

py_version = f'{sys.version_info.major}.{sys.version_info.minor}'

wheel_name, wheel_url = wheels.get(py_version)
wheel_path = os.path.join(package_dir, wheel_name)
response = requests.get(wheel_url)

print('response_status_code', response.status_code)

with open(wheel_path, 'wb') as fh:
    fh.write(response.content)


with wheel.wheelfile.WheelFile(wheel_path) as whl:
    whl.extractall(package_dir)

sys.path.insert(0, package_dir)

os.remove(os.path.join(package_dir, package_name, 'core', 'core.so'))
shutil.copy('../core_stubs.py', os.path.join(package_dir, package_name, 'core', 'core.py'))

import sqsgenerator
print(sqsgenerator)
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