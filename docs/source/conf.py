# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# http://www.sphinx-doc.org/en/master/config

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os 
import sys
#import subprocess
#subprocess.call('cd ../.. ; doxygen doxygenfile', shell=True)

# sys.path.insert(0, os.path.abspath('.'))


# -- Project information -----------------------------------------------------

project = 'C++ MMBN Engine'
copyright = '2019, TheMaverickProgrammer'
author = 'TheMaverickProgrammer'

# The full version, including alpha/beta/rc tags
release = '1'


# -- General configuration ---------------------------------------------------

sys.path.append("../breathe")
# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = ['breathe', 'exhale'
]

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = []


# -- Options for HTML output -------------------------------------------------

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

# -- Breathe setup -----------------------------------------------------------
breathe_projects = { "C++ MMBN Project": "xml" }
breathe_default_project = "C++ MMBN Project"

# -- Exhale setup ------------------------------------------------------------
exhale_args = {
        "containmentFolder": "./api",
        "rootFileName":      "library_root.rst",
        "rootFileTitle":     "Code",
        "doxygenStripFromPath": "..",
        "createTreeView":    True,
        "exhaleExecutesDoxygen": True,
        # "exhaleDoxygenStdin": "INPUT = ../../BattleNetwork",
        "exhaleUseDoxyfile": True,
        # "unabridgedOrphanKinds": {"dir", "file" }
        }

primary_domain = 'cpp'

highlight_language = 'cpp'

# on_rtd is whether we are on readthedocs.org, this line of code grabbed from docs.readthedocs.org
on_rtd = os.environ.get('READTHEDOCS', None) == 'True'

html_favicon = 'favicon.gif'
html_logo    = 'favicon.gif'
html_theme   = 'bootstrap'
html_theme_options = {
        'navbar_site_name': "Open MMBN",
        'navbar_links': [
            ("Contributing", "https://github.com/TheMaverickProgrammer/battlenetwork/wiki/Contributing", True)
            ],

        'source_link_position': "exclude",

        'navbar_class': "navbar",

        'navbar_fixed_top': "true",

        'bootswatch_theme': "sandstone",

        'bootstrap_version': "3",
        }

if not on_rtd:
    import sphinx_bootstrap_theme
    html_theme = 'bootstrap'
    html_theme_path = [sphinx_bootstrap_theme.get_html_theme_path()]
