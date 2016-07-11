from __future__ import print_function

import gtk
import gobject

from lxml import etree
from xdg import BaseDirectory

import ConfigParser
import os
import re
import shlex
from shutil import copyfile
import subprocess

from .item_types import elements_by_name
from .menu import MenuFile
from .util import TabButton, CommandText

try:
	 import dbus
except ImportError:
	 dbus = None

bus = None

element_list = [
	'Launcher',
	'Menu',
	'Separator',
	'Windows List',
	'Desktops List',
	'Viewports List',
	'Recent Documents', 
	'Reload',
	'Menu File',
]

allnotcache = True

class CBEditor(gtk.Window):
	def __init__(self, cache_file):
		gtk.Window.__init__(self)

		self.cache_file = cache_file
		try:
			self.parser=ConfigParser.SafeConfigParser()
			self.parser.read([cache_file])
			self.cache=self.parser.items("Files")
			self.allnotcache=False
		except IOError:
			self.allnotcache=True

		self.set_title("Compiz Boxmenu Editor")
		self.set_icon_name('cbmenu')
		self.set_border_width(5)
		self.set_size_request(600, 400)
		self.vbox=gtk.VBox(False, 2)
		self.connect('destroy', gtk.main_quit)
		hbox_main=gtk.HPaned()

		toolbar=gtk.Toolbar()

		new_button = gtk.MenuToolButton(gtk.STOCK_NEW)
		new_button.set_menu(self.make_new_menu())
		new_button.connect('clicked', self.new_item_dialog)
		new_button.set_tooltip_text("Create a new menu file or menu item")

		#edit_button = gtk.ToolButton(gtk.STOCK_EDIT) # necessary?

		save_button = gtk.MenuToolButton(gtk.STOCK_SAVE)
		save_button.set_menu(self.make_save_menu())
		save_button.connect('clicked', self.save_one)
		save_button.set_tooltip_text("Save the current menu")

		generate_button = gtk.ToolButton(gtk.STOCK_CONVERT)
		generate_button.connect('clicked', self.generated_item) #need to import other menus
		generate_button.set_tooltip_text('Generate menu entries from a pipemenu script')

		delete_button = gtk.MenuToolButton(gtk.STOCK_DELETE)
		delete_button.set_menu(self.make_delete_menu())
		delete_button.connect('clicked', self.delete_item)
		delete_button.set_tooltip_text('Delete the current menu item')

		reload_button = gtk.ToolButton(gtk.STOCK_REFRESH)
		reload_button.connect('clicked', self.reload_menu)
		reload_button.set_tooltip_text('Reload the daemon')

		#settings_button = gtk.ToolButton(gtk.STOCK_PREFERENCES)
		#settings_button.connect('clicked', self.show_settings)

		about_button = gtk.ToolButton(gtk.STOCK_ABOUT)
		about_button.connect('clicked', self.show_about_dialog)
		about_button.set_tooltip_text('Show some information about this program')

		sep=gtk.SeparatorToolItem()
		sep2=gtk.SeparatorToolItem()
		sep.set_draw(False)
		sep.set_expand(True)
		sep2.set_draw(False)
		sep2.set_expand(True)
		toolbar.insert(new_button, 0)
		#toolbar.insert(edit_button, 1)
		toolbar.insert(save_button, 1)
		toolbar.insert(generate_button, 2)
		toolbar.insert(delete_button, 3)
		toolbar.insert(sep, 4)
		#toolbar.insert(undo_button, 6)
		#toolbar.insert(redo_button, 7)
		toolbar.insert(sep2, 5)
		toolbar.insert(reload_button, 6)
		#toolbar.insert(settings_button, 8)
		toolbar.insert(about_button, 7)

		self.vbox.pack_start(toolbar, expand=False)

		listview=gtk.TreeView()
		self.list_menus()
		listview.set_model(self.menu_list)
		listview.connect('row-activated',self.open_menu_file)
		scrolled=gtk.ScrolledWindow()
		scrolled.add(listview)

		scrolled.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		scrolled.set_size_request(150,-1)

		hbox_main.add(scrolled)

		cached = gtk.TreeViewColumn('Cached?')
		listview.append_column(cached)
		cell = gtk.CellRendererToggle()
		cell.set_activatable(True)
		cell.connect('toggled', self.update_cache)
		cached.pack_start(cell)
		cached.add_attribute(cell, "active", 0)

		name = gtk.TreeViewColumn('Menu')
		listview.append_column(name)
		cell = gtk.CellRendererText()
		name.pack_start(cell)
		name.add_attribute(cell,"text", 1)

		self.tabs=gtk.Notebook()
		self.tabs.set_size_request(340,240)
		self.tabs.set_scrollable(True)
		self.tabs.connect('switch-page', self.get_current_menu)
		hbox_main.add(self.tabs)
		self.tabs.set_show_tabs(True)
		self.tabs.set_tab_pos(gtk.POS_TOP)

		#self.hbox_item=gtk.HBox(False, 2)

		self.vbox.pack_start(hbox_main)
		#self.vbox.pack_end(self.hbox_item, expand=False, fill=False)
		self.add(self.vbox)
		self.show_all()

	def make_new_menu(self):
		new_menu=gtk.Menu()
		for i in element_list:
			new_item = gtk.ImageMenuItem(i, gtk.STOCK_NEW)
			new_item.connect('activate', self.make_new_item, i)
			new_menu.append(new_item)
		new_menu.show_all()
		return new_menu

	def make_delete_menu(self):
		delete_menu=gtk.Menu()

		delete_item=gtk.ImageMenuItem("Current Item")
		delete_item.connect('activate', self.delete_item)
		delete_menu.append(delete_item)

		delete_item=gtk.ImageMenuItem("Current Menu")
		delete_item.connect('activate', self.delete_menu)
		delete_menu.append(delete_item)

		delete_menu.show_all()
		return delete_menu

	def make_save_menu(self):
		save_menu=gtk.Menu()

		save_item=gtk.ImageMenuItem("Current Menu")
		save_item.connect('activate', self.save_one)
		save_menu.append(save_item)

		save_item=gtk.ImageMenuItem("All")
		save_item.connect('activate', self.save_all)
		save_menu.append(save_item)

		save_menu.show_all()
		return save_menu

	def delete_menu(self, widget):
		dialog = gtk.MessageDialog(self,  gtk.DIALOG_MODAL, gtk.MESSAGE_WARNING, \
				gtk.BUTTONS_NONE, \
				"Are you sure you want to delete the current menu? You cannot recover it after you do this!")
		dialog.add_buttons(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_DELETE, gtk.RESPONSE_ACCEPT)
		response = dialog.run()
		dialog.destroy()
		if response == gtk.RESPONSE_ACCEPT:
			idx=self.tabs.get_current_page()
			menu=self.tabs.get_nth_page(idx)
			for list_row in self.menu_list:
				if list_row[2]==menu.filename:
					break
			self.menu_list.remove(list_row.iter)
			os.remove(menu.filename)
			if hasattr(menu, 'currently_editing'):
				menu.currently_editing.destroy()
			self.tabs.remove_page(idx)

	def delete_item(self, widget):
		idx=self.tabs.get_current_page()
		menu=self.tabs.get_nth_page(idx)
		menu.on_delete_clicked(self)

	def recursive_generate(self, widget, tree, pt=None):
		for element in tree.getchildren():
			if element.tag=='menu':
				new_row=self.make_new_item(widget, "Menu", nd=element, parent=pt)
				self.recursive_generate(widget, element, pt=new_row)
			elif element.tag=='separator':
				self.make_new_item(widget, "Separator", nd=element, parent=pt)
			else:
				if element.attrib['type']=='windowlist':
					self.make_new_item(widget, "Window List", nd=element, parent=pt)
				elif element.attrib['type']=='viewportlist':
					self.make_new_item(widget, "Viewports List", nd=element, parent=pt)
				elif element.attrib['type']=='desktoplist':
					self.make_new_item(widget, "Desktops List", nd=element, parent=pt)
				elif element.attrib['type']=='documents':
					self.make_new_item(widget, "Recent Documents", nd=element, parent=pt)
				else:
					self.make_new_item(widget, element.attrib['type'].title(), nd=element, parent=pt)

	def new_item_dialog(self, widget):
		dialog=gtk.Dialog('New Item', self,gtk.DIALOG_MODAL, \
		buttons=(gtk.STOCK_OK, gtk.RESPONSE_ACCEPT, gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT))
		dialog.set_size_request(250, 250)

		dialog.props.border_width = 6
		dialog.vbox.props.spacing = 6
		dialog.set_has_separator(False)

		model = gtk.ListStore(str)
		for el in element_list:
			model.append([el])

		treeview = gtk.TreeView(model)
		column = gtk.TreeViewColumn(None, gtk.CellRendererText(), text=0)
		treeview.set_headers_visible(False)
		treeview.append_column(column)

		treeview.connect('row-activated', self.on_row_activated)

		scroll = gtk.ScrolledWindow()
		scroll.add(treeview)
		scroll.props.hscrollbar_policy = gtk.POLICY_NEVER
		scroll.props.vscrollbar_policy = gtk.POLICY_AUTOMATIC
		dialog.vbox.pack_start(scroll, True, True)

		dialog.action_area.props.border_width = 0

		dialog.show_all()

		if dialog.run() == gtk.RESPONSE_ACCEPT:
			m, r = treeview.get_selection().get_selected()
			if r:
				elementname = m[r][0]
				self.make_new_item(widget, elementname)
			else:
				dialog.destroy()
				return
		dialog.destroy()

	def on_row_activated(self, treeview, path, view_column):
		self.response(gtk.RESPONSE_ACCEPT)

	def generated_item(self, widget):
		dialog=gtk.Dialog("Add generated items from pipemenu", self, gtk.DIALOG_MODAL, \
		buttons=(gtk.STOCK_OK, gtk.RESPONSE_ACCEPT, gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT))
		cmd_input=CommandText(label_text="Command", mode="Pipe Menu" , alternate_mode="Pipe Menu")
		dialog.vbox.add(cmd_input)
		dialog.vbox.show_all()
		cmd_input.combobox.hide()
		response=dialog.run()
		if response==gtk.RESPONSE_ACCEPT:
			process=subprocess.Popen(shlex.split(os.path.expanduser(cmd_input.entry.props.text)), \
				stdout=subprocess.PIPE)
			try:
				stdout = process.communicate()[0]
				dummy_txt = "<menu name='Dummy'>{}</menu>".format(stdout)
				dummy_menu=etree.fromstring("<menu name='Dummy'>{}</menu>".format(stdout))
				self.recursive_generate(widget, dummy_menu)
				dialog.destroy()
			except etree.XMLSyntaxError as e:
				message = gtk.MessageDialog(parent=self, flags=0, 
				    type=gtk.MESSAGE_ERROR, 
				    buttons=gtk.BUTTONS_OK, message_format=None)
				msg = ("{}\n\nClick the gear icon in the dialog to check"
				" pipe output.").format(e.msg)
				message.set_markup(msg)
				message.run()
				message.destroy()
			finally:
				dialog.destroy()
		else:
			dialog.destroy()

	def make_new_item(self, widget, element_type, nd=None, parent=None):
		if element_type != 'Menu File':
			idx=self.tabs.get_current_page()
			menu=self.tabs.get_nth_page(idx)
			model=menu.treeview.get_model()
			m, r = menu.treeview.get_selection().get_selected()
			sibling = None
			if parent is not None:
				current = model[parent][0]
				if current.node.tag != 'menu':
					sibling = parent
			elif r:
				current = model[r][0]
				if current.node.tag == 'menu':
					parent = r
				else:
					parent = model[r].parent
					if parent is not None:
						parent = parent.iter
					sibling = r
			if parent:
				parentelement = model[parent][0]
			else:
				parentelement = menu.menu

			element = elements_by_name[element_type](node=nd)
			if sibling:
				position = parentelement.node.index(current.node) + 1
				parentelement.node.insert(position, element.node)
				new_row=model.insert_after(parent, sibling, row=(element,))
			else:
				new_row=model.append(parent, row=(element,))
				parentelement.node.append(element.node)
			print("New %s created" % element_type)
			return new_row
		else:
			menu_path=BaseDirectory.xdg_config_home + "/compiz/boxmenu"
			dialog=gtk.Dialog("Add a new file", self, gtk.DIALOG_MODAL, \
			buttons=(gtk.STOCK_OK, gtk.RESPONSE_ACCEPT, gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT))
			fpath_input=gtk.Entry()
			dialog.vbox.add(gtk.Label("Name of the new menu to be created, with .xml at the end:"))
			dialog.vbox.add(fpath_input)
			dialog.vbox.show_all()
			response=dialog.run()
			if response != gtk.RESPONSE_ACCEPT or \
				fpath_input.props.text == "" or \
				not fpath_input.props.text.endswith(".xml"):
				dialog.destroy()
				return
			path="%s/%s" %(menu_path, fpath_input.props.text)
			f = open(path, "w")
			f.write("<menu></menu>")
			f.close()
			self.menu_list.append([False, fpath_input.props.text, path])
			dialog.destroy()
			print("New %s created" % element_type)

	def save_one(self, widget):
		idx=self.tabs.get_current_page()
		#ignore if tab isn't flagged
		#or if undo stack and redo stoack is empty
		widget=self.tabs.get_nth_page(idx)
		widget.write_menu()
		label=self.tabs.get_tab_label(widget)
		current_text=label.label.get_text()
		label.label.set_text(current_text.replace("*",""))

	def save_all(self, widget):
		for i in xrange(self.tabs.get_n_pages()):
			widget=self.tabs.get_nth_page(i)
			widget.write_menu()
			label=self.tabs.get_tab_label(widget)
			current_text=label.label.get_text()
			label.label.set_text(current_text.replace("*",""))

	def get_current_menu(self, notebook, page, page_num):
		for i in xrange(self.tabs.get_n_pages()):
			widget=self.tabs.get_nth_page(i)
			if hasattr(widget, 'currently_editing'):
				if i != page_num:
					widget.currently_editing.hide()
				else:
					widget.currently_editing.show()

	def change_tab_title_del(self, path, a, label):
		if not re.match("\*",label.get_text()):
			label.set_text("*"+label.get_text())

	def change_tab_title(self, path, item_iter, a, label):
		if not re.match("\*",label.get_text()):
			label.set_text("*"+label.get_text())

	def get_edit_panel(self, treeview, path, view_column, menufile):
		self.vbox.pack_end(menufile.currently_editing, expand=False)
		page_num=self.tabs.get_current_page()
		for i in xrange(self.tabs.get_n_pages()):
			widget=self.tabs.get_nth_page(i)
			if hasattr(widget, 'currently_editing'):
				if i != page_num:
					widget.currently_editing.hide()
				else:
					widget.currently_editing.show()

	def get_edit_panel_click(self, widget, menufile):
		self.get_edit_panel(None, None, None, menufile)

	def open_menu_file(self,treeview, path, view_column):
		for i in xrange(self.tabs.get_n_pages()):
			widget=self.tabs.get_nth_page(i)
			if self.menu_list[path[0]][2] == widget.filename:
				return

		menufile=MenuFile(self.menu_list[path[0]][2])
		hbox=TabButton(self.menu_list[path[0]][1])
		hbox.btn.connect('clicked', self.on_closetab_button_clicked, hbox.label, menufile)

		self.tabs.append_page(menufile, hbox)
		menufile.model.connect('row-changed',self.change_tab_title, hbox.label)
		menufile.model.connect('row-deleted',self.change_tab_title_del, hbox.label)
		menufile.model.connect('row-inserted',self.change_tab_title, hbox.label)
		menufile.treeview.connect('row-activated',self.get_edit_panel, menufile)
		menufile.edit_menu.connect('activate', self.get_edit_panel_click, menufile)

	def on_closetab_button_clicked(self, sender, widget, menufile):
		pagenum = self.tabs.page_num(menufile)
		menu=self.tabs.get_nth_page(pagenum)
		if re.match("\*",widget.get_text()):
			warning = gtk.MessageDialog(self, gtk.DIALOG_MODAL, gtk.MESSAGE_INFO, gtk.BUTTONS_NONE, 'Close %s with unsaved changes?' % os.path.basename(menu.filename))
			warning.add_buttons(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_OK, gtk.RESPONSE_ACCEPT)
			if warning.run() != gtk.RESPONSE_ACCEPT:
				warning.destroy()
				return
			warning.destroy()
		if hasattr(menu, 'currently_editing'):
			menu.currently_editing.destroy()
		self.tabs.remove_page(pagenum)

	def update_cache(self, widget, path):
		self.menu_list[path][0] = not self.menu_list[path][0]
		self.parser.remove_section("Files")
		self.parser.add_section("Files")
		j=0
		for i in self.menu_list:
			if i[1] == "menu.xml":
				print("Compiz Boxmenu caches menu.xml by default")
				continue
			if i[0]:
				self.parser.set("Files", "file_%s" % j, i[2])
				j=j+1
		with open(self.cache_file, 'w') as f:
			self.parser.write(f)

	def reload_menu(self, widget):
		global bus
		if bus is not None:
			try:
				bus.get_object('org.compiz_fusion.boxmenu', \
				'/org/compiz_fusion/boxmenu/menu').reload()
			except dbus.DBusException:
				return

	def show_about_dialog(self, widget):
		dialog=gtk.AboutDialog()
		dialog.set_name("Compiz Boxmenu Editor")
		dialog.set_version("1.1.6")
		dialog.set_authors(["ShadowKyogre","crdlb"])
		dialog.set_comments("A specialized editor for Compiz Boxmenu's menu files")
		dialog.run()
		dialog.destroy()

	def check_cache(self, path):
		for i in self.cache:
			if os.path.expanduser(i[1]) == path:
				return True
		return False

	def list_menus(self):
		self.menu_list=gtk.ListStore(gobject.TYPE_BOOLEAN, \
				gobject.TYPE_STRING, \
				gobject.TYPE_STRING)
		menu_path=BaseDirectory.xdg_config_home + "/compiz/boxmenu"
		for checking in os.listdir(menu_path):
			path="%s/%s" %(menu_path, checking)
			if checking.endswith(".xml"):
				if allnotcache:
					self.menu_list.append([False,checking, path])
				else:
					self.menu_list.append([self.check_cache(path),checking, path])

def main():
	global bus
	if dbus:
		try:
			 bus = dbus.SessionBus()
		except dbus.DBusException:
			 bus = None
	else:
		bus = None

	menu_path = BaseDirectory.xdg_config_home + "/compiz/boxmenu"

	if not os.path.exists(menu_path):
		os.makedirs(menu_path)
	elif not os.path.exists(menu_path+"/menu.xml"):
		starting_file = BaseDirectory.load_first_config('compiz/boxmenu/menu.xml')
		copyfile(starting_file, menu_path+"/menu.xml")

	cache_file=menu_path+"/precache.ini"
	CBEditor(cache_file)
	gtk.main()

if __name__ == '__main__':
	main()
