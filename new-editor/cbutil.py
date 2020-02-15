import os, gtk, glib
from pyicon_browser import *
from gi.repository import GObject
import subprocess
import shlex
import os

class TabButton(Gtk.HBox):
	def __init__(self, text):
		GObject.GObject.__init__(self)
		#http://www.eurion.net/python-snippets/snippet/Notebook%20close%20button.html
		self.label = Gtk.Label(label=text)
		self.pack_start(self.label, expand=True, fill=True, padding=0)

		#get a stock close button image
		close_image = Gtk.Image.new_from_stock(Gtk.STOCK_CLOSE, Gtk.IconSize.MENU)
		b_val, image_w, image_h = Gtk.icon_size_lookup(Gtk.IconSize.MENU)

		#make the close button
		self.btn = Gtk.Button()
		self.btn.set_relief(Gtk.ReliefStyle.NONE)
		self.btn.set_focus_on_click(False)
		self.btn.add(close_image)
		self.pack_start(self.btn, expand=False, fill=False, padding=0)

		#this reduces the size of the button
		#style = Gtk.RcStyle()
		#style.xthickness = 0
		#style.ythickness = 0
		#self.btn.modify_style(style)

		self.show_all()

#test code
#from cbutil import *
#from gi.repository import Gtk
#d=Gtk.Dialog()
#d.vbox.add(CommandText())
#d.run()

class CommandText(Gtk.HBox):
	def __init__(self, label_text="Name", mode="Normal", text="", alternate_mode="Execute"):
			GObject.GObject.__init__(self)

			label=Gtk.Label(label=label_text)

			self.entry=Gtk.Entry()
			self.entry.props.text=text

			self.button=Gtk.Button()
			image=Gtk.Image.new_from_icon_name("gtk-execute",Gtk.IconSize.LARGE_TOOLBAR)
			self.button.set_image(image)
			#known bug
			self.button.set_tooltip_markup("See the output this command generates")

			self.combobox=Gtk.ComboBoxText()
			self.combobox.append_text("Normal")
			self.combobox.append_text(alternate_mode)
			self.combobox.props.active = mode != "Normal"

			self.pack_start(label,expand=False,fill=True,padding=0)
			self.pack_start(self.entry, True, True, 0)
			self.pack_end(self.button,expand=False,fill=True,padding=0)
			self.pack_end(self.combobox, True, True, 0)

			self.combobox.connect('changed', self._emit_mode_signal)
			self.entry.connect('changed', self._emit_text_signal)
			self.button.connect('pressed', self._preview_text)

			self.show_all()

			get_mode=self.combobox.get_active_text()
			if get_mode == "Normal":
				self.button.props.sensitive=0
			else:
				self.button.props.sensitive=1

			if alternate_mode != "Execute" or get_mode != "Normal":
				completion = Gtk.EntryCompletion()
				self.entry.set_completion(completion)
				completion.set_model(POSSIBILITY_STORE)
				completion.set_text_column(0)

	def _emit_mode_signal(self, widget):
		text=self.combobox.get_active_text()
		if text == "Normal":
			self.entry.set_completion(None)
			self.button.props.sensitive=0
		else:
			completion = Gtk.EntryCompletion()
			self.entry.set_completion(completion)
			completion.set_model(POSSIBILITY_STORE)
			completion.set_text_column(0)
			self.button.props.sensitive=1
		self.emit('mode-changed', text)

	def _emit_text_signal(self, widget):
		self.emit('text-changed', widget.props.text)

	def _preview_text(self, widget):
		print("Generating preview, please wait...")
		buffer=Gtk.TextBuffer()
		buffer_errors=Gtk.TextBuffer()
		full_text=' '.join(['/usr/bin/env',os.path.expanduser(self.entry.props.text)])
		cmd=subprocess.Popen(shlex.split(full_text),stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		text,errors=cmd.communicate()
		if text != None:
			buffer.set_text(text)
		else:
			buffer.set_text("")

		if errors != None:
			buffer_errors.set_text(errors)
		else:
			buffer_errors.set_text("")

		dialog=Gtk.Dialog(title="Preview of %s" %(self.entry.props.text), \
			buttons=(Gtk.STOCK_OK, Gtk.ResponseType.ACCEPT))

		tabs=Gtk.Notebook()
		tabs.set_scrollable(True)

		scrolled=Gtk.ScrolledWindow()
		scrolled.add(Gtk.TextView(buffer))

		scrolled_errors=Gtk.ScrolledWindow()
		scrolled_errors.add(Gtk.TextView(buffer_errors))

		tabs.append_page(scrolled, Gtk.Label(label="Output"))
		tabs.append_page(scrolled_errors, Gtk.Label(label="Errors"))

		dialog.vbox.add(tabs)
		dialog.show_all()
		dialog.run()
		dialog.destroy()

#test code
#from cbutil import *
#from gi.repository import Gtk
#d=Gtk.Dialog()
#d.vbox.add(IconSelector())
#d.run()

class IconSelector(Gtk.HBox):
	def __init__(self, label_text="Icon", mode="Normal", text=""):
			GObject.GObject.__init__(self)

			label=Gtk.Label(label=label_text)
			self.text=text

			self.combobox=Gtk.ComboBoxText()
			self.combobox.append_text("Normal")
			self.combobox.append_text("File path")
			self.combobox.props.active = mode != "Normal"
			self.button=Gtk.Button()
			self.image=Gtk.Image()

			self.pack_start(label, False, True, 0)
			self.pack_start(self.button,expand=False,fill=True,padding=0)
			self.pack_end(self.combobox, True, True, 0)
			self.button.set_image(self.image)

			self.combobox.connect('changed', self._emit_mode_signal)
			self.button.connect('pressed', self._emit_text_signal)

			if mode == "File path":
				size=Gtk.icon_size_lookup(Gtk.IconSize.LARGE_TOOLBAR)[0]
				try:
					pixbuf=GdkPixbuf.Pixbuf.new_from_file_at_size(os.path.expanduser(self.text), size, size)
					self.image.set_from_pixbuf(pixbuf)
				except GLib.GError:
					self.image.set_from_pixbuf(None)
					print("Couldn't set icon from file: %s" %(self.text))
			else:
				self.image.set_from_icon_name(self.text,Gtk.IconSize.LARGE_TOOLBAR)
			self.button.set_tooltip_text(self.text)

			self.show_all()

	def _change_image(self, mode):
		if mode == "File path":
			size=Gtk.icon_size_lookup(Gtk.IconSize.LARGE_TOOLBAR)[0]
			try:
				pixbuf=GdkPixbuf.Pixbuf.new_from_file_at_size(os.path.expanduser(self.text), size, size)
				self.image.set_from_pixbuf(pixbuf)
			except GLib.GError:
				self.image.set_from_pixbuf(None)
				print("Couldn't set icon from file: %s" %(self.text))
		else:
			self.image.set_from_icon_name(self.text,Gtk.IconSize.LARGE_TOOLBAR)
		self.emit('image-changed', mode)

	def _emit_mode_signal(self, widget):
		text=self.combobox.get_active_text()
		self._change_image(text)
		self.emit('mode-changed', text)

	def _emit_text_signal(self, widget):
		text=""
		if self.combobox.get_active_text() == "Normal":
			dialog=IcoBrowse()
			dialog.set_defaults(self.text)
			response = dialog.run()
			if response == Gtk.ResponseType.ACCEPT:
				text=dialog.get_icon_name(None)
			dialog.destroy()
		else:
			btns=(Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL, Gtk.STOCK_OPEN, Gtk.ResponseType.ACCEPT)
			dialog=Gtk.FileChooserDialog(title="Select Icon", buttons=btns)
			dialog.set_filename(text)
			filter = Gtk.FileFilter()
			filter.set_name("Images")
			filter.add_mime_type("image/png")
			filter.add_mime_type("image/jpeg")
			filter.add_mime_type("image/gif")
			filter.add_pattern("*.png")
			filter.add_pattern("*.jpg")
			filter.add_pattern("*.gif")
			filter.add_pattern("*.tif")
			filter.add_pattern("*.xpm")
			dialog.add_filter(filter)
			response = dialog.run()
			if response == Gtk.ResponseType.ACCEPT:
				text=dialog.get_filename()
			dialog.destroy()
		if text != self.text and (text != "" and text != None):
			self.text = text
			self._change_image(self.combobox.get_active_text())
			self.button.set_tooltip_text(self.text)
			self.emit('text-changed', text)

def set_up():
	GObject.type_register(CommandText)
	GObject.type_register(IconSelector)

	GObject.signal_new("text-changed", CommandText, GObject.SignalFlags.RUN_FIRST,  None, (GObject.TYPE_STRING,))
	GObject.signal_new("mode-changed", CommandText, GObject.SignalFlags.RUN_FIRST,  None, (GObject.TYPE_STRING,))

	GObject.signal_new("image-changed", IconSelector, GObject.SignalFlags.RUN_FIRST,  None, (GObject.TYPE_STRING,))
	GObject.signal_new("text-changed", IconSelector, GObject.SignalFlags.RUN_FIRST,  None, (GObject.TYPE_STRING,))
	GObject.signal_new("mode-changed", IconSelector, GObject.SignalFlags.RUN_FIRST,  None, (GObject.TYPE_STRING,))

def completion_setup():
	print("Setting up command auto completion for best experience")
	for i in os.path.expandvars("$PATH").split(":"):
		if os.path.exists(i):
			print("Looking in %s" %i)
			for j in os.listdir(i):
				path="%s/%s" %(i,j)
				if not os.path.isdir(path) and \
				os.access(path, os.X_OK):
					POSSIBILITY_STORE.append([j])

POSSIBILITY_STORE = Gtk.ListStore(str)
set_up()
completion_setup()
