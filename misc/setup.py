from distutils.core import setup
from distutils.command.install import install as _install
from distutils.command.install_data import install_data as _install_data

import os

setup (
	name             = "cbmenu_scripts",
	version          = os.environ['VERSION'],
	description      = "Compiz-Boxmenu helper scripts",
	author           = "ShadowKyogre",
	author_email     = "shadowkyogre.public@gmail.com",
	url              = "http://github.com/ShadowKyogre/Compiz-Boxmenu",
	license          = "GPLv2+",
	packages         = [],
	scripts          = [
		'autoconfig-compiz.py',
		'cb_action_menu.py',
	],
)
