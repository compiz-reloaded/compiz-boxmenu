import gtk
from lxml import etree
from cbutil import *

class Item(object):

	def __init__(self, node=None, parent=None, type=None):

		self.editable = False

		if node is None:
			self.node = etree.Element('item')
			if type is not None:
				self.node.attrib['type'] = type

		else:
			self.node = node

	def get_name(self):
		return None

	def get_type(self):
		return 'Item'

	def get_icon(self):
		return None

	def get_icon_mode(self):
		return None

class Launcher(Item):

	def __init__(self, node=None):

		Item.__init__(self, node)

		if node is None:
			self.node = etree.Element('item', type='launcher')

		self.editable = True

	def get_name(self):

		subnode = self.node.find('name')
		if subnode is not None:
			name = subnode.text
			if subnode.attrib.get('mode') == 'exec':
				name = 'exec: %s' %name
			return name
		else:
			return None

	def get_type(self):
		return 'Launcher'

	def get_icon(self):
		iconnode = self.node.find('icon')
		if iconnode is not None:
			return iconnode.text
		else:
			return None

	def get_icon_mode(self):
		iconnode = self.node.find('icon')
		if iconnode is not None:
			if iconnode.attrib.get('mode1') == 'file':
				return iconnode.attrib.get('mode1')
		else:
			return None

	def get_options(self):

		retlist = []
		icons = []

		namenode = self.node.find('name')
		if namenode is not None:
			name = namenode.text
			if namenode.attrib.get('mode') == 'exec':
				name_mode="Execute"
			else:
				name_mode="Normal"
		else:
			name = ''
			name_mode="Normal"

		widget = CommandText(mode=name_mode,text=name)
		widget.connect('text-changed', self.on_subnode_changed, 'name')
		widget.connect('mode-changed', self.on_name_mode_changed)

		retlist.append(widget)

		iconnode = self.node.find('icon')
		if iconnode is not None:
			icon = iconnode.text
			if iconnode.attrib.get('mode1') == 'file':
				icon_mode = "File path"
			else:
				icon_mode="Normal"
		else:
			icon = ''
			icon_mode="Normal"
		widget = IconSelector(mode=icon_mode,text=icon)

		widget.connect('text-changed', self.on_subnode_changed, 'icon')
		widget.connect('mode-changed', self.on_icon_mode_changed)

		icons.append(widget)

		commandnode = self.node.find('command')
		if commandnode is not None:
			command = commandnode.text
			if commandnode.attrib.get('mode2') == 'pipe':
				command_mode="Pipe"
			else:
				command_mode="Normal"
		else:
			command = ''
			command_mode = "Normal"
		widget = CommandText(label_text="Command",mode=command_mode,text=command, alternate_mode="Pipe")

		widget.connect('text-changed', self.on_subnode_changed, 'command')
		widget.connect('mode-changed', self.on_command_mode_changed)

		retlist.append(widget)

		widget = gtk.CheckButton("Cache output of pipe")
		widget.props.active = commandnode is not None and commandnode.attrib.get('mode2') == 'pipe' and commandnode.attrib.get('cache') == 'true'
		widget.connect('toggled', self.on_cached_changed)
		retlist.append(widget)

		return icons,retlist

	def on_subnode_changed(self, widget, text, tag):
		subnode = self.node.find(tag)
		if text:
			if subnode is None:
				subnode = etree.SubElement(self.node, tag)
			subnode.text = text
		else:
			if subnode is not None:
				self.node.remove(subnode)

	def on_name_mode_changed(self, widget, text):
		namenode = self.node.find('name')
		print text
		if text == "Execute":
			if namenode is None:
				namenode = etree.SubElement(self.node, 'name')
			namenode.attrib['mode'] = 'exec'
		elif 'mode' in namenode.attrib:
			del namenode.attrib['mode']

	def on_icon_mode_changed(self, widget, text):
		iconnode = self.node.find('icon')
		if text == "File path":
			if iconnode is None:
				iconnode = etree.SubElement(self.node, 'icon')
			iconnode.attrib['mode1'] = 'file'
		elif 'mode1' in iconnode.attrib:
			del iconnode.attrib['mode1']

	def on_command_mode_changed(self, widget, text):
		commandnode = self.node.find('command')
		if text == "Pipe":
			if commandnode is None:
				commandnode = etree.SubElement(self.node, 'command')
			commandnode.attrib['mode2'] = 'pipe'
		elif 'mode2' in commandnode.attrib:
			del commandnode.attrib['mode2']

	def on_cached_changed(self, widget):
		commandnode = self.node.find('command')
		if widget.props.active:
			if commandnode is None:
				commandnode = etree.SubElement(self.node, 'command')
			commandnode.attrib['cache'] = 'true'
		elif 'cache' in commandnode.attrib:
			del commandnode.attrib['cache']

class Windowlist(Item):

	def __init__(self, node=None):
		Item.__init__(self, node, type='windowlist')
		self.editable = True
		self.thisvp = self.node.find('thisvp')
		self.minionly = self.node.find('minionly')

	def get_icon(self):
		iconnode = self.node.find('icon')
		if iconnode is not None:
			return iconnode.text
		else:
			return None

	def get_icon_mode(self):
		iconnode = self.node.find('icon')
		if iconnode is not None:
			if iconnode.attrib.get('mode1') == 'file':
				return iconnode.attrib.get('mode1')
		else:
			return None

	def get_options(self):
		icons = []
		retlist = []

		widget = gtk.CheckButton("Show windows only on current viewport")
		widget.props.active = self.get_thisvp()
		widget.connect('toggled', self.on_thisvp_changed)
		retlist.append(widget)

		widget = gtk.CheckButton("Show only minimized windows")
		widget.props.active = self.get_minionly()
		widget.connect('toggled', self.on_minionly_changed)
		retlist.append(widget)

		iconnode = self.node.find('icon')
		if iconnode is not None:
			icon = iconnode.text
			if iconnode.attrib.get('mode1') == 'file':
				icon_mode = "File path"
			else:
				icon_mode="Normal"
		else:
			icon = ''
			icon_mode="Normal"
		widget = IconSelector(mode=icon_mode,text=icon)

		widget.connect('text-changed', self.on_subnode_changed, 'icon')
		widget.connect('mode-changed', self.on_icon_mode_changed)

		icons.append(widget)

		return icons,retlist

	def on_subnode_changed(self, widget, text, tag):
		subnode = self.node.find(tag)
		if text:
			if subnode is None:
				subnode = etree.SubElement(self.node, tag)
			subnode.text = text
		else:
			if subnode is not None:
				self.node.remove(subnode)

	def on_icon_mode_changed(self, widget, text):
		iconnode = self.node.find('icon')
		if text == "File path":
			if iconnode is None:
				iconnode = etree.SubElement(self.node, 'icon')
			iconnode.attrib['mode1'] = 'file'
		elif 'mode1' in iconnode.attrib:
			del iconnode.attrib['mode1']

	def get_thisvp(self):
		if self.thisvp is not None:
			return self.thisvp.text == 'true'
		return False

	def get_minionly(self):
		if self.minionly is not None:
			return self.minionly.text == 'true'
		return False

	def on_thisvp_changed(self, widget):
		if self.thisvp is None:
			self.thisvp = etree.SubElement(self.node, 'thisvp')
		if widget.props.active:
			text = 'true'
		else:
			text = 'false'
		self.thisvp.text = text

	def on_minionly_changed(self, widget):
		if self.minionly is None:
			self.minionly = etree.SubElement(self.node, 'minionly')
		if widget.props.active:
			text = 'true'
		else:
			text = 'false'
		self.minionly.text = text

	def get_type(self):
		return 'Windows list'

class Viewportlist(Item):

	def __init__(self, node=None): #change to be an attribute of viewportlist
		Item.__init__(self, node, type='viewportlist')
		self.editable = True
		self.wrap = self.node.find('wrap')

	def get_type(self):
		return 'Viewports list'

	def get_icon(self):
		iconnode = self.node.find('icon')
		if iconnode is not None:
			return iconnode.text
		else:
			return None

	def get_icon_mode(self):
		iconnode = self.node.find('icon')
		if iconnode is not None:
			if iconnode.attrib.get('mode1') == 'file':
				return iconnode.attrib.get('mode1')
		else:
			return None


	def get_options(self):
		retlist = []
		icons = []

		widget = gtk.CheckButton("Wrap Viewports")
		widget.props.active = self.get_wrap()
		widget.connect('toggled', self.on_wrap_changed)
		retlist.append(widget)

		iconnode = self.node.find('icon')
		if iconnode is not None:
			icon = iconnode.text
			if iconnode.attrib.get('mode1') == 'file':
				icon_mode = "File path"
			else:
				icon_mode="Normal"
		else:
			icon = ''
			icon_mode="Normal"
		widget = IconSelector(mode=icon_mode,text=icon)

		widget.connect('text-changed', self.on_subnode_changed, 'icon')
		widget.connect('mode-changed', self.on_icon_mode_changed)

		icons.append(widget)

		vpiconnode = self.node.find('vpicon')
		if vpiconnode is not None:
			vpicon = vpiconnode.text
			if vpiconnode.attrib.get('mode1') == 'file':
				vpicon_mode = "File path"
			else:
				vpicon_mode="Normal"
		else:
			vpicon = ''
			vpicon_mode="Normal"
		widget = IconSelector(label_text="Viewport Icon",mode=vpicon_mode,text=vpicon)

		widget.connect('text-changed', self.on_subnode_changed, 'vpicon')
		widget.connect('mode-changed', self.on_vpicon_mode_changed)

		icons.append(widget)

		return icons,retlist

	def get_wrap(self):
		if self.wrap is not None:
			return self.wrap.text == 'true'
		return False

	def on_icon_mode_changed(self, widget, text):
		iconnode = self.node.find('icon')
		if text == "File path":
			if iconnode is None:
				iconnode = etree.SubElement(self.node, 'icon')
			iconnode.attrib['mode1'] = 'file'
		elif 'mode1' in iconnode.attrib:
			del iconnode.attrib['mode1']

	def on_vpicon_mode_changed(self, widget, text):
		iconnode = self.node.find('vpicon')
		if text == "File path":
			if iconnode is None:
				iconnode = etree.SubElement(self.node, 'vpicon')
			iconnode.attrib['mode1'] = 'file'
		elif 'mode1' in iconnode.attrib:
			del iconnode.attrib['mode1']

	def on_subnode_changed(self, widget, text, tag):
		subnode = self.node.find(tag)
		if text:
			if subnode is None:
				subnode = etree.SubElement(self.node, tag)
			subnode.text = text
		else:
			if subnode is not None:
				self.node.remove(subnode)

	def on_wrap_changed(self, widget):
		if self.wrap is None:
			self.wrap = etree.SubElement(self.node, 'wrap')
		if widget.props.active:
			text = 'true'
		else:
			text = 'false'
		self.wrap.text = text

class Documents(Item):

	def __init__(self, node=None): #change to be an attribute of viewportlist
		Item.__init__(self, node, type='documents')
		self.editable = True

	def get_type(self):
		return 'Recent Documents list'

	def get_icon(self):
		iconnode = self.node.find('icon')
		if iconnode is not None:
			return iconnode.text
		else:
			return None

	def get_icon_mode(self):
		iconnode = self.node.find('icon')
		if iconnode is not None:
			if iconnode.attrib.get('mode1') == 'file':
				return iconnode.attrib.get('mode1')
		else:
			return None


	def get_options(self):
		retlist = []
		icons = []
		sgroup = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)

		label = gtk.Label()
		label.set_alignment(0, 0.5)
		sgroup.add_widget(label)
		label.set_markup('<b>Open method:</b>')
		widget = gtk.Entry()
		commandnode = self.node.find('command')
		if commandnode is not None:
			command = commandnode.text
		else:
			command = ''
		widget.props.text = command
		widget.set_tooltip_text('If you need a more complicated command, type in that command and %f, which will tell compiz-boxmenu where to place the file name.')
		widget.connect('changed', self.on_subnode_changed, 'command')

		hbox = gtk.HBox()
		hbox.pack_start(label)
		hbox.pack_start(widget, True, True)
		retlist.append(hbox)

		iconnode = self.node.find('icon')
		if iconnode is not None:
			icon = iconnode.text
			if iconnode.attrib.get('mode1') == 'file':
				icon_mode = "File path"
			else:
				icon_mode="Normal"
		else:
			icon = ''
			icon_mode="Normal"
		widget = IconSelector(mode=icon_mode,text=icon)

		widget.connect('text-changed', self.on_subnode_changed, 'icon')
		widget.connect('mode-changed', self.on_icon_mode_changed)

		icons.append(widget)

		label = gtk.Label()
		label.set_alignment(0, 0.5)
		sgroup.add_widget(label)
		label.set_markup('<b>Days from today:</b>')
		widget = gtk.Entry()

		agenode = self.node.find('age')
		if agenode is not None:
			age = agenode.text
		else:
			age = ''
		widget.props.text = age
		widget.connect('changed', self.on_subnode_changed, 'age')

		hbox = gtk.HBox()
		hbox.pack_start(label)
		hbox.pack_start(widget, True, True)
		retlist.append(hbox)

		label = gtk.Label()
		label.set_alignment(0, 0.5)
		sgroup.add_widget(label)
		label.set_markup('<b>Items to display:</b>')
		widget = gtk.Entry()

		quantitynode = self.node.find('quantity')
		if quantitynode is not None:
			quantity = quantitynode.text
		else:
			quantity = ''
		widget.props.text = quantity
		widget.connect('changed', self.on_subnode_changed, 'quantity')

		hbox = gtk.HBox()
		hbox.pack_start(label)
		hbox.pack_start(widget, True, True)
		retlist.append(hbox)

		sortnode = self.node.find('sort')

		label = gtk.Label()
		label.set_alignment(0, 0.5)
		sgroup.add_widget(label)
		label.set_markup('<b>Sort mode:</b>')
		widget = gtk.combo_box_new_text()
		widget.append_text('None')
		widget.append_text('Most Used')
		widget.append_text('Least Used')
		if sortnode is None:
			widget.set_active(0)
		elif sortnode.text == 'most used':
			widget.set_active(1)
		elif sortnode.text == 'least used':
			widget.set_active(2)
		else:
			widget.set_active(-1)
		widget.connect('changed', self.on_sort_mode_changed)

		hbox = gtk.HBox()
		hbox.pack_start(label)
		hbox.pack_start(widget, True, True)
		retlist.append(hbox)

		return icons,retlist

	def on_subnode_changed(self, widget, text, tag):
		subnode = self.node.find(tag)
		if text:
			if subnode is None and tag != 'age' and tag != 'quantity':
				subnode = etree.SubElement(self.node, tag)
			elif tag == 'age' or tag == 'quantity':
				if re.search("^\d+$", text): #don't make if it doesn't have text
					if subnode is None:
						subnode = etree.SubElement(self.node, tag)
					subnode.text = text #don't change the numbers if
			else:
				subnode.text = text
		else:
			if subnode is not None:
				self.node.remove(subnode)

	def on_icon_mode_changed(self, widget, text):
		iconnode = self.node.find('icon')
		if text == "File path":
			if iconnode is None:
				iconnode = etree.SubElement(self.node, 'icon')
			iconnode.attrib['mode1'] = 'file'
		elif 'mode1' in iconnode.attrib:
			del iconnode.attrib['mode1']

	def on_sort_mode_changed(self, widget):
		sortnode = self.node.find('sort')
		sorttype = widget.get_active_text()
		if sorttype != 'None':
			print sorttype
			if sortnode is None:
				sortnode = etree.SubElement(self.node, 'sort')
			if sorttype == 'Most Used':
				sortnode.text = 'most used'
			else:
				sortnode.text = 'least used'
		else:
			if sortnode is not None:
				self.node.remove(sortnode)

class Reload(Item):

	def __init__(self, node=None):
		Item.__init__(self, node, type='reload')
		self.editable = True

	def get_icon(self):
		iconnode = self.node.find('icon')
		if iconnode is not None:
			return iconnode.text
		else:
			return None

	def get_icon_mode(self):
		iconnode = self.node.find('icon')
		if iconnode is not None:
			if iconnode.attrib.get('mode1') == 'file':
				return iconnode.attrib.get('mode1')
		else:
			return None

	def get_options(self):
		icons = []
		retlist = []

		iconnode = self.node.find('icon')
		if iconnode is not None:
			icon = iconnode.text
			if iconnode.attrib.get('mode1') == 'file':
				icon_mode = "File path"
			else:
				icon_mode="Normal"
		else:
			icon = ''
			icon_mode="Normal"
		widget = IconSelector(mode=icon_mode,text=icon)

		widget.connect('text-changed', self.on_subnode_changed, 'icon')
		widget.connect('mode-changed', self.on_icon_mode_changed)

		icons.append(widget)

		return icons,retlist


	def on_subnode_changed(self, widget, text, tag):
		subnode = self.node.find(tag)
		if text:
			if subnode is None:
				subnode = etree.SubElement(self.node, tag)
			subnode.text = text
		else:
			if subnode is not None:
				self.node.remove(subnode)

	def on_icon_mode_changed(self, widget, text):
		iconnode = self.node.find('icon')
		if text == "File path":
			if iconnode is None:
				iconnode = etree.SubElement(self.node, 'icon')
			iconnode.attrib['mode1'] = 'file'
		elif 'mode1' in iconnode.attrib:
			del iconnode.attrib['mode1']

	def get_type(self):
		return 'Reload'

class Separator(object):

	def __init__(self, node=None, parent=None):
		self.node = node
		self.editable = True
		if self.node is None:
			self.node = etree.Element('separator')

	def get_name(self):
		name = self.node.attrib.get('name', '---')
		if self.node.attrib.get('mode') == 'exec':
			name = 'exec: %s' %name
		return name

	def get_type(self):
		return 'Separator'

	def get_icon(self):
		return self.node.attrib.get('icon', '')

	def get_icon_mode(self):
		if self.node.attrib.get('mode1') == 'file':
			return self.node.attrib.get('mode1')
		else:
			return None

	def get_options(self):
		icons = []
		retlist = []

		name = self.node.attrib.get('name')
		if name is not None:
			if self.node.attrib.get('mode') == 'exec':
				name_mode="Execute"
			else:
				name_mode="Normal"
		else:
			name = ''
			name_mode="Normal"

		widget = CommandText(mode=name_mode,text=name)
		widget.connect('text-changed', self.on_name_changed)
		widget.connect('mode-changed', self.on_name_mode_changed)

		retlist.append(widget)

		icon = self.node.attrib.get('icon')
		if icon is not None:
			if self.node.attrib.get('mode1') == 'file':
				icon_mode = "File path"
			else:
				icon_mode="Normal"
		else:
			icon = ''
			icon_mode="Normal"

		widget = IconSelector(mode=icon_mode,text=icon)
		widget.connect('text-changed', self.on_icon_changed)
		widget.connect('mode-changed', self.on_icon_mode_changed)

		icons.append(widget)

		return icons,retlist

	def on_name_changed(self, widget, text):
		if text != '':
			self.node.attrib['name'] = text
		else:
			del self.node.attrib['name']

	def on_name_mode_changed(self, widget, text):
		if text == "Execute":
			self.node.attrib['mode'] = 'exec'
		elif 'mode' in self.node.attrib:
			del self.node.attrib['mode']

	def on_icon_changed(self, widget, text):
		if text != '':
			self.node.attrib['icon'] = text
		else:
			del self.node.attrib['icon']

	def on_icon_mode_changed(self, widget, text):
		if text == "File path":
			self.node.attrib['mode1'] = 'file'
		elif 'mode1' in self.node.attrib:
			del self.node.attrib['mode1']

class Menu(object):

	def __init__(self, node=None):
		self.node = node
		self.children = []
		self.editable = True

		if node is None:
			self.node = etree.Element('menu', name='')

		for child in self.node.getchildren():
			try:
				self.children.append(self.make_child(child))
			except KeyError:
				pass


	def make_child(self, child):
		return elements[child.tag](child)

	def get_name(self):
		name = self.node.attrib.get('name', '')
		if self.node.attrib.get('mode') == 'exec':
			name = 'exec: %s' %name
		return name

	def get_icon(self):
		return self.node.attrib.get('icon', '')

	def get_icon_mode(self):
		if self.node.attrib.get('mode1') == 'file':
			return self.node.attrib.get('mode1')
		else:
			return None

	def get_type(self):
		return 'Menu'

	def get_options(self):

		icons = []
		retlist = []

		name = self.node.attrib.get('name')
		if name is not None:
			if self.node.attrib.get('mode') == 'exec':
				name_mode="Execute"
			else:
				name_mode="Normal"
		else:
			name = ''
			name_mode="Normal"

		widget = CommandText(mode=name_mode,text=name)
		widget.connect('text-changed', self.on_name_changed)
		widget.connect('mode-changed', self.on_name_mode_changed)

		retlist.append(widget)

		icon = self.node.attrib.get('icon')
		if icon is not None:
			if self.node.attrib.get('mode1') == 'file':
				icon_mode = "File path"
			else:
				icon_mode="Normal"
		else:
			icon = ''
			icon_mode="Normal"

		widget = IconSelector(mode=icon_mode,text=icon)
		widget.connect('text-changed', self.on_icon_changed)
		widget.connect('mode-changed', self.on_icon_mode_changed)

		icons.append(widget)

		return icons,retlist

	def on_name_changed(self, widget, text):
		self.node.attrib['name'] = text

	def on_name_mode_changed(self, widget, text): #EDIT THIS
		if text == "Execute":
			self.node.attrib['mode'] = 'exec'
		elif 'mode' in self.node.attrib:
			del self.node.attrib['mode']

	def on_icon_changed(self, widget, text):
		if text != '':
			self.node.attrib['icon'] = text
		else:
			del self.node.attrib['icon']

	def on_icon_mode_changed(self, widget, text): #EDIT THIS
		if text == "File path":
			self.node.attrib['mode1'] = 'file'
		elif 'mode1' in self.node.attrib:
			del self.node.attrib['mode1']

itemtypes = {
	'launcher': Launcher,
	'windowlist': Windowlist,
	'viewportlist': Viewportlist,
	'documents': Documents,
	'reload': Reload,
}


def make_item(node):

	itemtype = node.attrib.get('type')
	return itemtypes.get(itemtype, Item)(node)

elements = {'menu': Menu, 'item': make_item, 'separator': Separator}

elementsbyname = {
	'Launcher': Launcher,
	'Windows List': Windowlist,
	'Viewports List': Viewportlist,
	'Recent Documents': Documents,
	'Reload': Reload,
	'Separator': Separator,
	'Menu': Menu,
}