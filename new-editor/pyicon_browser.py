#!/usr/bin/python

from gi.repository import Gtk as gtk
from gi.repository import GdkPixbuf as gdkpixbuf
from gi.repository import GObject as gobject
import re

#http://developer.gnome.org/pygtk/2.22/class-gtkicontheme.html#method-gtkicontheme--list-contexts

class IcoBrowse(gtk.Dialog):
	def __init__(self, message="", default_text='', modal=True):
		gtk.Dialog.__init__(self)
		self.add_buttons(gtk.STOCK_CANCEL, gtk.ResponseType.ACCEPT,
		      gtk.STOCK_OK, gtk.ResponseType.ACCEPT)
		#self.set_title("Icon search")
		if modal:
			self.set_modal(True)
		self.set_border_width(5)
		self.set_size_request(400, 300)
		self.combobox=gtk.ComboBoxText()
		self.combobox.set_size_request(200, 20)
		hbox=gtk.HBox(False,2)

		#format: actual icon, name, context
		self.model=gtk.ListStore(gdkpixbuf.Pixbuf, gobject.TYPE_STRING, gobject.TYPE_STRING)
		self.modelfilter=self.model.filter_new()
		self.modelfilter.set_visible_func(self.search_icons)
		self.iconview=gtk.IconView()
		self.iconview.set_model(self.modelfilter)
		self.iconview.set_pixbuf_column(0)
		self.iconview.set_text_column(1)
		self.iconview.set_selection_mode(gtk.SelectionMode.SINGLE)
		self.iconview.set_item_width(72)
		self.iconview.set_size_request(200, 220)
		defaulttheme=gtk.IconTheme.get_default()
		self.combobox.connect('changed', self.category_changed)
		self.refine=gtk.Entry()
		self.refine.connect('changed', self.category_changed)
		self.refine.set_icon_from_stock(gtk.EntryIconPosition.SECONDARY, gtk.STOCK_FIND)
		self.refine.set_size_request(200, 30)
		list2=[]
		for c in defaulttheme.list_contexts():
			current=defaulttheme.list_icons(c)
			list2+=set(current)
			self.combobox.append_text(c)
			for i in current:
				self.model.append([defaulttheme.load_icon(i, 32,
									  gtk.IconLookupFlags.USE_BUILTIN),
									  i,c])
		other=list(set(defaulttheme.list_icons(None))-(set(list2)))
		for i in other:
			self.model.append([defaulttheme.load_icon(i, 32,
									  gtk.IconLookupFlags.USE_BUILTIN),
									  i,"Other"])
		self.combobox.prepend_text("Other")
		scrolled = gtk.ScrolledWindow()
		scrolled.add(self.iconview)
		scrolled.props.hscrollbar_policy = gtk.PolicyType.NEVER
		scrolled.props.vscrollbar_policy = gtk.PolicyType.AUTOMATIC
		hbox.add(self.combobox)
		hbox.add(self.refine)
		self.vbox.add(hbox)
		self.vbox.add(scrolled)
		self.combobox.set_active(0)

		self.iconview.connect('selection-changed', self.get_icon_name)

		self.vbox.show_all()

	def set_defaults(self, icon_name):
		if icon_name != "" and icon_name != None:
			cmodel = self.combobox.get_model()
			for i in range(len(self.model)):
				if self.model[i][1] == icon_name:
					self.refine.props.text=icon_name
					for j in range(len(cmodel)):
						if self.model[i][2] == cmodel[j][0]:
							print(cmodel[j])
							self.combobox.set_active(j)
							break
					break

	def get_icon_name(self, widget):
		path=self.iconview.get_selected_items()
		if len(path) > 0:
			return self.modelfilter[path[0]][1]

	def category_changed(self, widget):
		self.modelfilter.refilter()

	def search_icons(self, tree, iter, blah):
		search_term = self.combobox.get_active_text()
		search_term2 = self.refine.props.text
		if self.model.get_value(iter, 2) == search_term:
			if search_term2 == None or search_term2 == "" or re.search(search_term2, self.model.get_value(iter,1)):
				return True
			return False
		else:
			return False
