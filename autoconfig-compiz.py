#!/usr/bin/env python3

from __future__ import print_function
import compizconfig

context = compizconfig.Context()

if 'run_command0_key' in context.Plugins['commands'].Display:

    print("setting up for compiz 0.8.x+")

    context.Plugins['commands'].Display['command0'].Value = 'compiz-boxmenu'
    context.Plugins['commands'].Display['run_command0_key'].Value = '<Alt>F1'

    if 'vpswitch' in context.Plugins:
        vpswitch = context.Plugins['vpswitch']
        if not vpswitch.Enabled:
            vpswitch.Enabled = True
        vpswitch.Display['init_plugin'].Value = 'commands'
        vpswitch.Display['init_action'].Value = 'run_command0_key'

    context.Write()

else:
    
    print("autoconfig-compiz.py requires compiz 0.8.x+ for full functionality")
