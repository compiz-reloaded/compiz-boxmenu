from gi.repository import Gtk
from gi.repository import GdkPixbuf
from gi.repository import GObject
import re
from gi.repository import GLib
#from GLib import GError
from sys import stderr

#http://developer.gnome.org/pygtk/2.22/class-gtkicontheme.html#method-gtkicontheme--list-contexts

class IcoBrowse(Gtk.Dialog):
	def __init__(self, message="", default_text='', modal=True):
		GObject.GObject.__init__(self)
		self.add_buttons(Gtk.STOCK_CANCEL, Gtk.ResponseType.CLOSE,
		      Gtk.STOCK_OK, Gtk.ResponseType.ACCEPT)
		#self.set_title("Icon search")
		if modal:
			self.set_modal(True)
		self.set_border_width(5)
		self.set_size_request(400, 300)
		self.combobox=Gtk.ComboBoxText()
		self.combobox.set_size_request(200, 20)
		hbox=Gtk.HBox(False,2)
		
		#format: actual icon, name, context
		#self.model=Gtk.ListStore(GdkPixbuf.Pixbuf, GObject.TYPE_STRING, GObject.TYPE_STRING)
		#self.modelfilter=self.model.filter_new()
		self.modelfilter=ICON_STORE.filter_new()
		self.iconview=Gtk.IconView()
		self.iconview.set_model(self.modelfilter)
		self.iconview.set_pixbuf_column(0)
		self.iconview.set_text_column(1)
		self.iconview.set_selection_mode(Gtk.SelectionMode.SINGLE)
		self.iconview.set_item_width(72)
		self.iconview.set_size_request(200, 220)
		defaulttheme=Gtk.IconTheme.get_default()
		self.combobox.connect('changed', self.category_changed)
		self.refine=Gtk.Entry()
		self.refine.connect('changed', self.category_changed)
		self.refine.set_icon_from_stock(Gtk.EntryIconPosition.SECONDARY, Gtk.STOCK_FIND)
		self.refine.set_size_request(200, 30)
		self.modelfilter.set_visible_func(self.search_icons)
		#catted_icons=[]
		for c in defaulttheme.list_contexts():
			#current=defaulttheme.list_icons(context=c)
			#catted_icons+=set(current)
			self.combobox.append_text(c)
			#for i in current:
			#	try:
			#		self.model.append([defaulttheme.load_icon(i, 32,
			#						  Gtk.IconLookupFlags.USE_BUILTIN),
			#						  i,c])
			#	except GLib.GError as err: stderr.write('Error loading "%s": %s\n' % (i, err.args[0]))
		#other=list(set(defaulttheme.list_icons())-(set(catted_icons)))
		#for i in other:
		#	self.model.append([defaulttheme.load_icon(i, 32,
		#							  Gtk.IconLookupFlags.USE_BUILTIN),
		#							  i,"Other"])
		self.combobox.prepend_text("Other")
		scrolled = Gtk.ScrolledWindow()
		scrolled.add(self.iconview)
		scrolled.props.hscrollbar_policy = Gtk.PolicyType.NEVER
		scrolled.props.vscrollbar_policy = Gtk.PolicyType.AUTOMATIC
		hbox.add(self.combobox)
		hbox.add(self.refine)
		self.vbox.pack_start(hbox, expand=False,fill=True, padding=1)
		self.vbox.pack_end(scrolled, expand=True, fill=True, padding=1)
		self.combobox.set_active(0)
		
		self.iconview.connect('selection-changed', self.get_icon_name)
		
		self.vbox.show_all()

	def set_defaults(self, icon_name):
		if icon_name != "" and icon_name != None:
			cmodel = self.combobox.get_model()
			for i in range(len(ICON_STORE)):
				if ICON_STORE[i][1] == icon_name:
					self.refine.props.text=icon_name
					for j in range(len(cmodel)):
						if ICON_STORE[i][2] == cmodel[j][0]:
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

	def search_icons(self, tree, iter, data):
		search_term = self.combobox.get_active_text()
		search_term2 = self.refine.props.text
		if ICON_STORE.get_value(iter, 2) == search_term:
			if search_term2 == None or search_term2 == "" or re.search(search_term2, ICON_STORE.get_value(iter,1)):
				return True
			return False
		else:
			return False

def set_up():
	print("Preloading icons")
	defaulttheme=Gtk.IconTheme.get_default()
	catted_icons=set()
	for c in defaulttheme.list_contexts():
		current=defaulttheme.list_icons(context=c)
		catted_icons=catted_icons.union(set(current))
		print("Found {} icons in {}".format(len(current),c))
		#self.combobox.append_text(c)
		for i in current:
			try:
				ICON_STORE.append([defaulttheme.load_icon(i, 32,
								  Gtk.IconLookupFlags.USE_BUILTIN),
								  i,c])
			except GLib.GError as err: stderr.write('Error loading "%s": %s\n' % (i, err.args[0]))
	other=list(set(defaulttheme.list_icons())-catted_icons)
	print("Placing misc. icons in Other")
	for i in other:
		ICON_STORE.append([defaulttheme.load_icon(i, 32,
								  Gtk.IconLookupFlags.USE_BUILTIN),
								  i,"Other"])


ICON_STORE=Gtk.ListStore(GdkPixbuf.Pixbuf, GObject.TYPE_STRING, GObject.TYPE_STRING)
set_up()

if __name__ == '__main__':
	icobrowse = IcoBrowse()
	icobrowse.connect('destroy', Gtk.main_quit)
	icobrowse.run()
	Gtk.main()
