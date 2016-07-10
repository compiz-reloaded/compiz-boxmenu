from distutils.core import setup
from distutils.command.install import install as _install
from distutils.command.install_data import install_data as _install_data

import os

setup (
	name             = "cbmenu_editor",
	version          = os.environ['VERSION'],
	description      = "Compiz-Boxmenu's menu editor",
	author           = "ShadowKyogre",
	author_email     = "shadowkyogre.public@gmail.com",
	url              = "http://github.com/ShadowKyogre/Compiz-Boxmenu",
	license          = "GPLv2+",
	packages         = ["cbmenu_editor"],
	scripts          = ['compiz-boxmenu-editor', 'compiz-boxmenu-iconbrowser'],
)
