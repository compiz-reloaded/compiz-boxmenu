#!/usr/bin/env python2

import compizconfig

context = compizconfig.Context()

if 'run_command0_key' in context.Plugins['core'].Display:

    print "setting up for compiz 0.7.x+"

    context.Plugins['core'].Display['command0'].Value = 'compiz-boxmenu'
    context.Plugins['core'].Display['run_command0_key'].Value = '<Alt>F1'

    if 'vpswitch' in context.Plugins:
        vpswitch = context.Plugins['vpswitch']
        if not vpswitch.Enabled:
            vpswitch.Enabled = True
        vpswitch.Display['init_plugin'].Value = 'core'
        vpswitch.Display['init_action'].Value = 'run_command0_key'

    context.Write()

else:
    
    print "compiz-boxmenu requires compiz 0.7.x+ for full functionality"
