import gtk
import os
import glib
from xdg import BaseDirectory
import re #This is to autoset file mode for *.desktop icons
import ConfigParser
from lxml import etree
from cb_itemtypes import *

#test lines:
#import cbmenu,cb_itemtypes
#import gtk
#blah=cbmenu.MenuFile("/home/shadowkyogre/.config/compiz/boxmenu/menu.xml")
#blurg=gtk.Window()
#blurg.add(blah)
#blurg.show_all()

class MenuFile(gtk.ScrolledWindow):

	def __init__(self,filename):
		gtk.ScrolledWindow.__init__(self)

		self.model = gtk.TreeStore(object)
		self.add_menu_file(filename)
		self.filename=filename

		self.props.hscrollbar_policy = gtk.POLICY_AUTOMATIC #because you really might want to read some of the stuff hanging off
		self.props.vscrollbar_policy = gtk.POLICY_AUTOMATIC
		self.treeview = gtk.TreeView(self.model)
		self.treeview.set_reorderable(True)
		cell = gtk.CellRendererText()
		elements = gtk.TreeViewColumn('Item', cell)
		elements.set_cell_data_func(cell, self.get_type)
		self.treeview.append_column(elements)

		name = gtk.TreeViewColumn('Name')

		cell = gtk.CellRendererPixbuf()
		name.pack_start(cell, False)
		name.set_cell_data_func(cell, self.get_icon)

		cell = gtk.CellRendererText()
		name.pack_start(cell)
		name.set_cell_data_func(cell, self.get_name)

		self.treeview.append_column(name)
		self.add(self.treeview)
		targets = [
			('deskmenu-element', gtk.TARGET_SAME_WIDGET, 0),
			#('deskmenu-element', gtk.TARGET_SAME_APP, 0),
			#('deskmenu-element', gtk.TREE_VIEW_ITEM, 0),
			('text/uri-list', 0, 1),
		]
		self.treeview.enable_model_drag_source(gtk.gdk.BUTTON1_MASK, targets, gtk.gdk.ACTION_DEFAULT|gtk.gdk.ACTION_MOVE)
		self.treeview.enable_model_drag_dest(targets, gtk.gdk.ACTION_MOVE)

		self.treeview.connect('drag-data-get', self.on_drag_data_get)
		self.treeview.connect('drag-data-received', self.on_drag_data_received)

		self.treeview.connect('row-activated', self.on_row_activated)

		self.treeview.connect('button-press-event', self.on_treeview_button_press_event)
		self.treeview.expand_all()

		self.selection = self.treeview.get_selection()
		self.selection.connect('changed', self.on_selection_changed)

		self.popup = gtk.Menu()
		self.edit_menu = gtk.ImageMenuItem(stock_id=gtk.STOCK_EDIT)
		self.edit_menu.connect('activate', self.on_edit_clicked)
		self.popup.append(self.edit_menu)
		self.delete_menu = gtk.ImageMenuItem(stock_id=gtk.STOCK_DELETE)
		self.delete_menu.connect('activate', self.on_delete_clicked)
		self.popup.append(self.delete_menu)
		self.popup.show_all()

		self.show_all()

	def add_menu_file(self, filename):
		self.menufile = etree.parse(filename)
		self.menu = Menu(self.menufile.getroot())
		self.add_menu(self.menu)

	def add_menu(self, m, parent=None):
		for item in m.children:
			iter = self.model.append(parent, [item])
			if item.node.tag == 'menu':
				self.add_menu(item, iter)

	def get_name(self, column, cell, model, iter):
		name = model.get_value(iter, 0).get_name()
		if name is None:
			name = ''
		cell.set_property('text', name)

	def get_type(self, column, cell, model, iter):
		typ = model.get_value(iter, 0).get_type()
		if typ is None:
			typ = ''
		cell.set_property('text', typ)

	def get_icon(self, column, cell, model, iter):
		icon = model.get_value(iter, 0).get_icon()
		icon_mode = model.get_value(iter, 0).get_icon_mode()
		#somehow does not set icon until selection change with things that don't have icons originally!
		if icon is not None:
			if icon_mode is not None:
				w = gtk.icon_size_lookup(gtk.ICON_SIZE_MENU)
				try:
					cell.set_property('pixbuf', gtk.gdk.pixbuf_new_from_file_at_size(os.path.expanduser(icon), w[0], w[0]))
					cell.set_property('icon-name', None) #possibly reduntant safety measure
				except glib.GError:
					cell.set_property('icon-name', None)
					cell.set_property('pixbuf', None)
			else:
				cell.set_property('icon-name', icon)
				cell.set_property('pixbuf', None) #possibly reduntant safety measure
		else:
			cell.set_property('icon-name', None)
			cell.set_property('pixbuf', None)

	def on_edit_clicked(self, widget):
		if hasattr(self, 'currently_editing'):
			self.currently_editing.destroy()
		model=self.selection.get_selected()[0]
		iter=self.selection.get_selected()[1]
		self.currently_editing=EditItemPanel(model,iter)

	def on_delete_clicked(self, widget):

		model, row = self.selection.get_selected()

		parent = None
		if row:
			current = model[row][0].node

			if current.tag == 'menu' and len(current):
				warning = gtk.MessageDialog(self.get_toplevel(), \
				gtk.DIALOG_MODAL, gtk.MESSAGE_INFO, gtk.BUTTONS_NONE, \
				'Delete menu element with %s children?' %len(current))
				warning.add_buttons(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_DELETE, gtk.RESPONSE_ACCEPT)
				if warning.run() != gtk.RESPONSE_ACCEPT:
					warning.destroy()
					return
				warning.destroy()

			parent = model[row].parent
			if parent is not None:
				parent = parent[0].node
			else:
				parent = self.menu.node
			parent.remove(current)
			model.remove(row)

		#write_menu()

	def on_close_clicked(self, widget):

		self.write_menu()

	def on_drag_data_get(self, treeview, context, selection, target_id,
						   etime):
		treeselection = treeview.get_selection()
		model, iter = treeselection.get_selected()
		data = model.get_string_from_iter(iter)

		selection.set(selection.target, 8, data)

	def on_drag_data_received(self, treeview, context, x, y, selection,
								info, etime):
		model = treeview.get_model()
		data = selection.data

		drop_info = treeview.get_dest_row_at_pos(x, y)
		if selection.type == 'deskmenu-element':
			source = model[data][0]
			if drop_info:
				path, position = drop_info
				siter = model.get_iter(data)
				diter = model.get_iter(path)

				if model.get_path(model.get_iter_from_string(data)) == path:
					return

				dest = model[path][0]
				if context.action == gtk.gdk.ACTION_MOVE:
					source.node.getparent().remove(source.node)

				if dest.node.tag == 'menu' and position in (gtk.TREE_VIEW_DROP_INTO_OR_BEFORE,
					gtk.TREE_VIEW_DROP_INTO_OR_AFTER):
					dest.node.append(source.node)
					fiter = model.append(diter, row=(source,))
				else:
					i = dest.node.getparent().index(dest.node)
					if position in (gtk.TREE_VIEW_DROP_INTO_OR_BEFORE,
						gtk.TREE_VIEW_DROP_BEFORE):
						dest.node.getparent().insert(i, source.node)
						fiter = model.insert_before(None, diter, row=(source,))
					else:
						dest.node.getparent().insert(i+1, source.node)
						fiter = model.insert_after(None, diter, row=(source,))

				if model.iter_has_child(siter):
					citer = model.iter_children(siter)
					while citer is not None:
						model.append(fiter, row=(model[citer][0],))
						citer = model.iter_next(citer)
				if context.action == gtk.gdk.ACTION_MOVE:
					context.finish(True, True, etime)

		elif selection.type == 'text/uri-list':
			print selection.data
			if drop_info:
				path, position = drop_info
				uri = selection.data.replace('file:///', '/').replace("%20"," ").strip()
				entry = ConfigParser.ConfigParser()
				entry.read(uri)
				launcher = Launcher()
				launcher.name = etree.SubElement(launcher.node, 'name')
				launcher.icon = etree.SubElement(launcher.node, 'icon')
				launcher.command = etree.SubElement(launcher.node, 'command')
				try:
					launcher.name.text = entry.get('Desktop Entry', 'Name')
					if re.search("/", entry.get('Desktop Entry', 'Icon')):
						launcher.icon.attrib['mode1'] = 'file'
					launcher.icon.text = entry.get('Desktop Entry', 'Icon')
					launcher.command.text = entry.get('Desktop Entry', 'Exec').split('%')[0]
				except ConfigParser.Error:
					return
				dest = model[path][0]
				diter = model.get_iter(path)
				if dest.node.tag == 'menu' and position in (gtk.TREE_VIEW_DROP_INTO_OR_BEFORE,
					gtk.TREE_VIEW_DROP_INTO_OR_AFTER):
					dest.node.append(launcher.node)
					fiter = model.append(diter, row=(launcher,))
				else:
					i = dest.node.getparent().index(dest.node)
					if position in (gtk.TREE_VIEW_DROP_INTO_OR_BEFORE,
						gtk.TREE_VIEW_DROP_BEFORE):
						dest.node.getparent().insert(i, launcher.node)
						fiter = model.insert_before(None, diter, row=(launcher,))
					else:
						dest.node.getparent().insert(i+1, launcher.node)
						fiter = model.insert_after(None, diter, row=(launcher,))
				if context.action == gtk.gdk.ACTION_MOVE:
					context.finish(True, True, etime)

		return

	def on_selection_changed(self, selection):

		model, row = selection.get_selected()

		sensitive = row and model.get_value(row, 0).editable

		self.edit_menu.props.sensitive = sensitive
		self.delete_menu.props.sensitive = row

	def on_row_activated(self, treeview, path, view_column):

		model = treeview.get_model()
		if hasattr(self, 'currently_editing'):
			self.currently_editing.destroy()
		self.currently_editing=EditItemPanel(model, model.get_iter(path))
		#change this to actually fill out item panel

	def on_treeview_button_press_event(self, treeview, event):
		if event.button == 3:
			pthinfo = self.treeview.get_path_at_pos(int(event.x), int(event.y))
			if pthinfo is not None:
				path, col, cellx, celly = pthinfo
				treeview.grab_focus()
				treeview.set_cursor(path, col, 0)
				self.popup.popup(None, None, None, event.button, event.time)
			return 1

	def get_icon_mode(self):
		iconnode = self.node.find('icon')
		if iconnode is not None:
			if iconnode.attrib.get('mode1') == 'file':
				return iconnode.attrib.get('mode1')
		else:
			return None

	def indent(self,elem, level=0):
		i = "\n" + level*"\t" #used to be "  ", use actual tabs
		if len(elem):
			if not elem.text or not elem.text.strip():
				elem.text = i + "\t"
			for e in elem:
				self.indent(e, level+1)
				if not e.tail or not e.tail.strip():
					e.tail = i + "\t"
			if not e.tail or not e.tail.strip():
				e.tail = i
		else:
			if level and (not elem.tail or not elem.tail.strip()):
				elem.tail = i

	def write_menu(self):
		self.indent(self.menu.node)
		self.menufile.write(open(self.filename, 'w'))

class EditItemPanel(gtk.HBox):

	def __init__(self, model=None, row=None, element=None):
		gtk.HBox.__init__(self)
		self.vbox_image_grid=gtk.VBox(True, 2)
		self.vbox_other_options=gtk.VBox(True, 2)

		self.add(self.vbox_image_grid)
		self.add(self.vbox_other_options)

		self.props.spacing = 6

		if element is None:
			if not row:
				return
			element = model.get_value(row, 0)

		if not element.editable:
			return

		icons,widgets=element.get_options()

		for icon in icons:
			self.vbox_image_grid.add(icon)

		for widget in widgets:
			self.vbox_other_options.add(widget)

		self.show_all()