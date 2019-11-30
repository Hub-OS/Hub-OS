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
import re
import sys
# sys.path.insert(0, os.path.abspath('.'))


# -- Project information -----------------------------------------------------

project = 'C++ MMBN Engine'
copyright = '2019, TheMaverickProgrammer'
author = 'TheMaverickProgrammer'

# The full version, including alpha/beta/rc tags
release = '1'


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = ['breathe', 'exhale']

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
# html_static_path   = ['_static']
html_theme         = 'bootstrap'
html_favicon       = 'favicon.gif'
html_logo          = 'favicon.gif'
html_theme         = 'bootstrap'
html_theme_options = {
    'navbar_site_name': "Open MMBN",
    'navbar_links': [
        ("Contributing", "https://github.com/TheMaverickProgrammer/battlenetwork/wiki/Contributing", True)
    ],
    'source_link_position': "exclude",
    'navbar_class': "navbar",
    'navbar_fixed_top': "true",
    'navbar_sidebarrel': False,  # Exhale does not support next / previous well
    'bootswatch_theme': "sandstone",
    'bootstrap_version': "3",
}

# -- Breathe setup -----------------------------------------------------------
# NOTE: see also, OUTPUT_DIRECTORY and XML_OUTPUT_DIRECTORY in Doxyfile.
# If those change, this must update.
this_file_dir = os.path.dirname(os.path.abspath(__file__))
breathe_projects = {project: os.path.join(this_file_dir, '_doxygen', 'xml')}
breathe_default_project = project

# Tell sphinx not to process the doxygen output directory when searching for
# files to render into html.
exclude_patterns = ['_doxygen/**']

# -- Exhale setup ------------------------------------------------------------
exhale_args = {
    "containmentFolder": "./api",
    "rootFileName": "library_root.rst",
    "rootFileTitle": "Code",
    # NOTE: see STRIP_FROM_PATH in Doxyfile.  If that updates, this must too.
    "doxygenStripFromPath": "../../BattleNetwork",
    "fullToctreeMaxDepth": 1,
    # See edits in source_read.  This tricks Exhale into not including a
    # top-level "Full API" heading on the unabridged document.
    "fullApiSubSectionTitle": "",
    "createTreeView": False,  # See source_read, they get pruned.
    "exhaleExecutesDoxygen": True,
    "exhaleUseDoxyfile": True,
    "unabridgedOrphanKinds": {"dir", "file"}
}

primary_domain = 'cpp'
highlight_language = 'cpp'


def source_read(app, docname, source):
    # NOTE: these patches should work until exhale 1.x is released.

    # source is a length one list with it's single element being the source
    # of the file being processed.  Modify source[0] to change what Sphinx
    # will render.

    # Delete both view hierarchies since this project has a mostly flat
    # structure.  Rely on the unabridged API instead.
    if docname == "api/library_root":
        source[0] = re.sub(
            r"\.\. include:: .*_view_hierarchy.rst", "", source[0]
        )

    # Orphan the view hierarchy documents since they aren't used anymore.
    if docname in {"api/class_view_hierarchy", "api/file_view_hierarchy"}:
        source[0] = ":orphan:\n\n" + source[0]


# Called by sphinx automatically.
def setup(app):
    app.connect("source-read", source_read)
