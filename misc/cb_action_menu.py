#!/usr/bin/env python2

import gtk, gobject
import wnck

from argparse import ArgumentParser
import ConfigParser
import os
import re
import sys

WID_AT_POINT=re.compile(r"(?:window:)(\d+)")
DEFAULT_ICONS={
	'move_icon':        'transform-move',
	'resize_icon':      'calibrate',
	'minimize_icon':    'gtk-go-down',
	'shade_icon':       'gtk-remove',
	'maximize_icon':    'gtk-fullscreen',
	'restore_icon':     'gtk-leave-fullscreen',
	'sticky_icon':      'sticky-notes',
	'pin_icon':         'sticky-notes',
	'above_icon':       'gtk-goto-top',
	'close_icon':       'gtk-close',
	'vp_holder_icon':   'workspace-switcher',
	'dp_holder_icon':   'workspace-switcher',
	'dbg_holder_icon':  'gtk-info',
}

class WindowActionMenu(gtk.Menu):
	def __init__(self, wid, icons, debug_menu=False):
		gtk.Menu.__init__(self)

		#Do this so that way we can ACTUALLY get a window
		self.scr =  wnck.screen_get_default()
		self.scr.force_update()

		self.windowfor = wnck.window_get(long(wid))

		wdecoheader = gtk.SeparatorMenuItem()
		hbox = gtk.HBox()
		wdecoheader.add(hbox)
		wicon = self.windowfor.get_mini_icon()
		hbox.pack_start(gtk.image_new_from_pixbuf(wicon), False, False)
		hbox.pack_end(gtk.Label(self.windowfor.get_name()), True, False)

		wdecoheader.set_state(gtk.STATE_PRELIGHT)

		minimize = gtk.ImageMenuItem()
		img = gtk.image_new_from_icon_name(icons.get('minimize_icon', ''), gtk.ICON_SIZE_MENU)
		minimize.set_image(img)
		if self.windowfor.is_minimized():
			minimize.set_label("Unminimize")
		else:
			minimize.set_label("Minimize")
		minimize.set_use_underline(True)
		minimize.connect('activate', self.toggle_minimize_window)

		shade = gtk.ImageMenuItem()
		img = gtk.image_new_from_icon_name(icons.get('shade_icon', ''), gtk.ICON_SIZE_MENU)
		shade.set_image(img)
		if self.windowfor.is_shaded():
			shade.set_label("Unshade")
		else:
			shade.set_label("Shade")
		shade.set_use_underline(True)
		shade.connect('activate', self.toggle_shade_window)

		maxrestore = gtk.ImageMenuItem()
		#maximized: gtk-fullscreen, gtk-leave-fullscreen
		if self.windowfor.is_maximized():
			img = gtk.image_new_from_icon_name(icons.get('restore_icon', ''), gtk.ICON_SIZE_MENU)
			maxrestore.set_image(img)
			maxrestore.set_label("Restore")
		else:
			img = gtk.image_new_from_icon_name(icons.get('maximize_icon', ''), gtk.ICON_SIZE_MENU)
			maxrestore.set_image(img)
			maxrestore.set_label("Maximize")
		maxrestore.set_use_underline(True)
		maxrestore.connect('activate', self.maxrestore_window)

		move = gtk.ImageMenuItem()
		img = gtk.image_new_from_icon_name(icons.get('move_icon', ''), gtk.ICON_SIZE_MENU)
		move.set_image(img)
		move.set_label("Move")
		move.set_use_underline(True)
		move.connect('activate', self.move_window)

		resize = gtk.ImageMenuItem()
		img = gtk.image_new_from_icon_name(icons.get('resize_icon', ''), gtk.ICON_SIZE_MENU)
		resize.set_image(img)
		resize.set_label("Resize")
		resize.set_use_underline(True)
		resize.connect('activate', self.resize_window)

		vp_on = self.scr.get_workspace(0).is_virtual()
		if vp_on:
			sticky = gtk.ImageMenuItem()
			img = gtk.image_new_from_icon_name(icons.get('sticky_icon', ''), gtk.ICON_SIZE_MENU)
			sticky.set_image(img)
			if self.windowfor.is_sticky():
				sticky.set_label("Unsticky")
			else:
				sticky.set_label("Sticky")
			sticky.set_use_underline(True)
			sticky.connect('activate', self.toggle_sticky_window)

		dp_on = self.scr.get_workspace_count() > 1
		if dp_on:
			pin = gtk.ImageMenuItem()
			img = gtk.image_new_from_icon_name(icons.get('pin_icon', ''), gtk.ICON_SIZE_MENU)
			pin.set_image(img)
			if self.windowfor.is_pinned():
				pin.set_label("Unpin")
			else:
				pin.set_label("Pin")
			pin.set_use_underline(True)
			pin.connect('activate', self.toggle_pin_window)

		above = gtk.ImageMenuItem()
		img = gtk.image_new_from_icon_name(icons.get('above_icon', ''), gtk.ICON_SIZE_MENU)
		above.set_image(img)
		if self.windowfor.is_above():
			above.set_label("Unput above")
		else:
			above.set_label("Put above")
		above.set_use_underline(True)
		above.connect('activate', self.toggle_above_window)

		wclose = gtk.ImageMenuItem()
		img = gtk.image_new_from_icon_name(icons.get('close_icon', ''), gtk.ICON_SIZE_MENU)
		wclose.set_image(img)
		wclose.set_label("Close")
		wclose.set_use_underline(True)
		wclose.connect('activate', self.close_window)

		self.append(wdecoheader)
		self.append(minimize)
		self.append(shade)
		self.append(maxrestore)
		self.append(resize)
		self.append(move)
		if dp_on:
			self.append(pin)
		if vp_on:
			self.append(sticky)
		self.append(above)
		self.make_for_desktop(icons)
		self.make_for_viewport(icons)
		self.append(wclose)
		if debug_menu:
			self.make_debug_menu(icons)

		self.show_all()

	def make_debug_menu(self, icons):
		self.dbgmenu = gtk.Menu()
		print dir(self.windowfor)

		self.clipboard = gtk.Clipboard()

		wname = self.windowfor.get_name()
		copy_name = gtk.MenuItem()
		copy_name.set_label('Name: {}'.format(wname))
		copy_name.connect('activate', self.copy_to_clipboard, wname)
		self.dbgmenu.append(copy_name)

		wcname = self.windowfor.get_class_group().get_name()
		copy_class_group_name = gtk.MenuItem()
		copy_class_group_name.set_label('Class: {}'.format(wcname))
		copy_class_group_name.connect('activate', self.copy_to_clipboard, wcname)
		self.dbgmenu.append(copy_class_group_name)

		wtype = self.windowfor.get_window_type()
		copy_wtype = gtk.MenuItem()
		copy_wtype.set_label('Window Type: {}'.format(wtype.value_name))
		copy_wtype.connect('activate', self.copy_to_clipboard, wtype.value_name)
		self.dbgmenu.append(copy_wtype)

		wid = self.windowfor.get_xid()
		copy_wid = gtk.MenuItem()
		copy_wid.set_label('Window ID: {}'.format(wid))
		copy_wid.connect('activate', self.copy_to_clipboard, str(wid))
		self.dbgmenu.append(copy_wid)
	
		self.dbgmenu_holder = gtk.ImageMenuItem()
		self.dbgmenu_holder.set_label("Debug")
		img = gtk.image_new_from_icon_name(icons.get('dbg_holder_icon', ''), gtk.ICON_SIZE_MENU)
		self.dbgmenu_holder.set_image(img)
		self.dbgmenu_holder.set_submenu(self.dbgmenu)
		self.append(self.dbgmenu_holder)

	def make_for_desktop(self, icons):
		"""Makes move to ${desktop} menu. Doesn't add anything if there's only one."""
		dp_on = self.scr.get_workspace_count() > 1
		if dp_on and not self.windowfor.is_pinned():
			self.dpmenu = gtk.Menu()

			ws = self.windowfor.get_workspace()
			if ws.get_neighbor(wnck.MOTION_LEFT):
				dpleft = gtk.ImageMenuItem()
				dpleft.set_label("Move left")
				dpleft.connect('activate', self.move_to_desktop, wnck.MOTION_LEFT)
				self.dpmenu.append(dpleft)

			if ws.get_neighbor(wnck.MOTION_RIGHT):
				dpright = gtk.ImageMenuItem()
				dpright.set_label("Move right")
				dpright.connect('activate', self.move_to_desktop, wnck.MOTION_RIGHT)
				self.dpmenu.append(dpright)

			if ws.get_neighbor(wnck.MOTION_UP):
				dpup = gtk.ImageMenuItem()
				dpup.set_label("Move up")
				dpup.connect('activate', self.move_to_desktop, wnck.MOTION_UP)
				self.dpmenu.append(dpup)

			if ws.get_neighbor(wnck.MOTION_DOWN):
				dpdown = gtk.ImageMenuItem()
				dpdown.set_label("Move down")
				dpdown.connect('activate', self.move_to_desktop, wnck.MOTION_DOWN)
				self.dpmenu.append(dpdown)

			self.dpmenu.append(gtk.SeparatorMenuItem())

			ws = self.windowfor.get_workspace()
			if ws:
				wsnum = ws.get_number()
			else:
				wsnum = -1

			for i in range(self.scr.get_workspace_count()):
				item = gtk.ImageMenuItem()
				iws = self.scr.get_workspace(i)
				item.set_label(iws.get_name())
				if i == wsnum:
					item.set_sensitive(False)
				item.connect('activate', self.warp_to_desktop, i)
				dpmenu.append(item)

			self.dpmenu_holder = gtk.ImageMenuItem()
			self.dpmenu_holder.set_label("Desktops")
			img = gtk.image_new_from_icon_name(icons.get('dp_holder_icon', ''), gtk.ICON_SIZE_MENU)
			self.dpmenu_holder.set_image(img)
			self.dpmenu_holder.set_submenu(self.dpmenu)
			self.append(self.dpmenu_holder)

	def make_for_viewport(self, icons):
		"""Makes move to ${viewport} menu. Doesn't add anything if there's only one."""

		vp_on = self.scr.get_workspace(0).is_virtual()
		if vp_on and not self.windowfor.is_sticky():
			sw = self.scr.get_width()
			sh = self.scr.get_height()

			#normally would use window workspace, but
			#this is null for stickies
			ws = self.scr.get_active_workspace()
			vpx = ws.get_viewport_x()
			vpy = ws.get_viewport_y()
			vpw = ws.get_width()
			vph = ws.get_height()
			wx, wy, _, _ = self.windowfor.get_geometry()
			wx += vpx
			wy += vpy

			self.vpmenu = gtk.Menu()

			if wx >= sw:
				vpleft = gtk.ImageMenuItem()
				vpleft.set_label("Move left")
				vpleft.connect('activate', self.move_to_viewport, 0)
				self.vpmenu.append(vpleft)

			if wx < (vpw - sw):
				vpright = gtk.ImageMenuItem()
				vpright.set_label("Move right")
				vpright.connect('activate', self.move_to_viewport, 1)
				self.vpmenu.append(vpright)

			if wy >= sh:
				vpup = gtk.ImageMenuItem()
				vpup.set_label("Move up")
				vpup.connect('activate', self.move_to_viewport, 2)
				self.vpmenu.append(vpup)

			if wy < (vph - sh):
				vpdown = gtk.ImageMenuItem()
				vpdown.set_label("Move down")
				vpdown.connect('activate', self.move_to_viewport, 3)
				self.vpmenu.append(vpdown)

			# create vp move to if the viewport is at least
			# two screens worth
			num = 1
			if vpw >= (sw << 1) or vph >= (sh << 1):
				self.vpmenu.append(gtk.SeparatorMenuItem())
				for y in range(0, vph, sh):
					for x in range(0, vpw, sw):
						item = gtk.ImageMenuItem()
						item.set_use_underline(True)
						if num == 10:
							label = "Viewport 1_0"
						else:
							label = "Viewport {}{}".format("_" if num > 10 else "", num)
						item.set_label(label)
						num+=1

						if wx >= x and wx < (x + sw) and wy >= y and wy < (y + sh):
							item.set_sensitive(False)

						item.connect('activate', self.warp_to_viewport, x, y)
						self.vpmenu.append(item)


			self.vpmenu_holder = gtk.ImageMenuItem()
			self.vpmenu_holder.set_label("Viewports")
			img = gtk.image_new_from_icon_name(icons.get('vp_holder_icon', ''), gtk.ICON_SIZE_MENU)
			self.vpmenu_holder.set_image(img)
			self.vpmenu_holder.set_submenu(self.vpmenu)
			self.append(self.vpmenu_holder)

	def copy_to_clipboard(self, widget, text):
		self.clipboard.set_can_store([("UTF8_STRING", 0, 0)])
		self.clipboard.set_text(text)
		self.clipboard.store()
		gobject.timeout_add(100, gtk.main_quit)
		gtk.main()

	def warp_to_desktop(self, widget, dp):
		ws = self.scr.get_workspace(dp)
		self.windowfor.move_to_workspace(ws)

	def move_to_desktop(self, widget, card_dir):
		ws = self.windowfor.get_workspace().get_neighbor(card_dir)
		if ws:
			self.windowfor.move_to_workspace(ws)

	def warp_to_viewport(self, widget, vpx, vpy):
		self.windowfor.unstick()
		wx, wy, ww, wh = self.windowfor.get_geometry()

		ws = self.scr.get_active_workspace()

		ovpx, ovpy = ws.get_viewport_x(), ws.get_viewport_y()

		self.windowfor.set_geometry(wnck.WINDOW_GRAVITY_STATIC,
			wnck.WINDOW_CHANGE_X|wnck.WINDOW_CHANGE_Y,
			wx + vpx-ovpx, wy + vpy-ovpy, ww, wh)

	def move_to_viewport(self, widget, card_dir):
		modx = 0
		mody = 0

		ws = self.scr.get_active_workspace()
		dvpx, dvpy = self.scr.get_width(), self.scr.get_height()

		if card_dir == 0:
			modx = -dvpx
		elif card_dir == 1:
			modx = dvpx
		elif card_dir == 2:
			mody = -dvpy
		elif card_dir == 3:
			mody = dvpy

		if modx != 0 or mody != 0:
			self.windowfor.unstick()
			wx, wy, ww, wh = self.windowfor.get_geometry()
			self.windowfor.set_geometry(wnck.WINDOW_GRAVITY_STATIC,
				wnck.WINDOW_CHANGE_X|wnck.WINDOW_CHANGE_Y,
				wx + modx, wy + mody, ww, wh)

	def resize_window(self, widget):
		self.windowfor.keyboard_size()

	def move_window(self, widget):
		self.windowfor.keyboard_move()

	def close_window(self, widget):
		self.windowfor.close()

	def toggle_above_window(self, widget):
		if self.windowfor.is_above():
			self.above()
		else:
			self.unmake_above()

	def toggle_pin_window(self, widget):
		if self.windowfor.is_pinned():
			self.windowfor.pin()
		else:
			self.windowfor.unpin()
	
	def toggle_sticky_window(self, widget):
		if self.windowfor.is_sticky():
			self.windowfor.unstick()
		else:
			self.windowfor.stick()

	def toggle_shade_window(self, widget):
		if self.windowfor.is_shaded():
			self.windowfor.unshade()
		else:
			self.windowfor.shade()

	def toggle_minimize_window(self, widget):
		if self.windowfor.is_minimized():
			self.windowfor.unminimize()
		else:
			self.windowfor.minimize()
	
	def maxrestore_window(self, widget):
		if self.windowfor.is_maximized():
			self.windowfor.unmaximize()
		else:
			self.windowfor.maximize()

if __name__ == '__main__':
	aparser = ArgumentParser()
	aparser.add_argument('windowid', help="The window to display this for")
	aparser.add_argument('--debug-menu', '-d', help="Show a debug menu for the window", 
	                     action='store_true', default=False)
	args = aparser.parse_args()

	cparser = ConfigParser.SafeConfigParser(DEFAULT_ICONS)
	conffile = os.path.expanduser('~/.config/compiz/boxmenu/actions_menu.ini')

	try:
		with open(conffile, 'r') as f:
			cparser.readfp(f)
	except IOError as e:
		pass
	finally:
		icon_conf = {}
		for k in DEFAULT_ICONS:
			icon_conf[k] = cparser.get('DEFAULT', k)

	wam = WindowActionMenu(args.windowid, icon_conf, debug_menu=args.debug_menu)
	wam.connect('deactivate', gtk.main_quit)
	wam.popup(None, None, None, 0, 0)
	gtk.main()
