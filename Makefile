# PREFIX fixing
ifdef ($(LOCALBASE))
	PREFIX=$(LOCALBASE)
else
	PREFIX?=/usr
endif

# checking for python
PYTHONBIN?=$(shell which python$(shell pkg-config --modversion python-2.7 2> /dev/null))
PYTHONBIN?=$(shell which python2.6)
PYTHONBIN?=$(shell which python2)

ifeq ("$(PYTHONBIN)", "")
$(error Python not found. Version >= 2.7 or 2.6 is required.)
endif

# Set up compile flags
CPPFLAGS := `pkg-config --cflags dbus-glib-1 gdk-2.0 gtk+-2.0 libwnck-1.0`
CPPFLAGS_CLIENT := `pkg-config --cflags dbus-glib-1`
WARNINGS := -Wall -Wextra -Wno-unused-parameter
ifdef ($(DEBUG))
	CFLAGS := -O2 -g $(WARNINGS)
else
	CFLAGS := $(WARNINGS)
endif
LDFLAGS := `pkg-config --libs dbus-glib-1 gdk-2.0 gtk+-2.0 libwnck-1.0`
LDFLAGS_CLIENT := `pkg-config --libs dbus-glib-1`

# Targets

all: deskmenu-glue.h compiz-boxmenu-daemon compiz-boxmenu compiz-boxmenu-dlist compiz-boxmenu-vplist compiz-boxmenu-wlist compiz-boxmenu-editor

compiz-boxmenu: 
	$(CC) $(CPPFLAGS_CLIENT) $(CFLAGS) -o $@ deskmenu.c deskmenu-common.h $(LDFLAGS_CLIENT)  

compiz-boxmenu-dlist:
	$(CC) $(CPPFLAGS_CLIENT) $(CFLAGS) -o $@ deskmenu-documentlist-client.c deskmenu-common.h $(LDFLAGS_CLIENT)  

compiz-boxmenu-vplist:
	$(CC) $(CPPFLAGS_CLIENT) $(CFLAGS) -o $@ deskmenu-vplist-client.c deskmenu-common.h $(LDFLAGS_CLIENT)  

compiz-boxmenu-wlist:
	$(CC) $(CPPFLAGS_CLIENT) $(CFLAGS) -o $@ deskmenu-windowlist-client.c deskmenu-common.h $(LDFLAGS_CLIENT)  

compiz-boxmenu-daemon:
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ deskmenu-menu.c deskmenu-wnck.c deskmenu-utils.c  $(LDFLAGS) 

compiz-boxmenu-editor:
	m4 -DLOOK_HERE=$(PREFIX)/share/cb-editor -DPYTHONBIN=$(PYTHONBIN) $@.in >$@

deskmenu-glue.h:
	dbus-binding-tool --mode=glib-server --prefix=deskmenu --output=$@ deskmenu-service.xml

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin/
	mkdir -p $(DESTDIR)$(PREFIX)/share/icons
	mkdir -p $(DESTDIR)$(PREFIX)/share/cb-editor
	install -m755 compiz-boxmenu $(DESTDIR)$(PREFIX)/bin/
	install -m755 compiz-boxmenu-dlist $(DESTDIR)$(PREFIX)/bin/
	install -m755 compiz-boxmenu-vplist $(DESTDIR)$(PREFIX)/bin/
	install -m755 compiz-boxmenu-wlist $(DESTDIR)$(PREFIX)/bin/
	install -m755 compiz-boxmenu-daemon $(DESTDIR)$(PREFIX)/bin/
	install -m755 compiz-boxmenu-editor $(DESTDIR)$(PREFIX)/bin/
	install -m644 new-editor/* $(DESTDIR)$(PREFIX)/share/cb-editor
	cp -r hicolor $(DESTDIR)$(PREFIX)/share/icons
	mkdir -p $(DESTDIR)$(LOCALBASE)/etc/xdg/compiz/boxmenu/
	install -m644 menu.xml $(DESTDIR)$(LOCALBASE)/etc/xdg/compiz/boxmenu/
	install -m644 precache.ini $(DESTDIR)$(LOCALBASE)/etc/xdg/compiz/boxmenu/
	mkdir -p $(DESTDIR)$(PREFIX)/share/dbus-1/services/
	install -m644 org.compiz_fusion.boxmenu.service $(DESTDIR)$(PREFIX)/share/dbus-1/services/

clean:
	rm -f compiz-boxmenu compiz-boxmenu-dlist compiz-boxmenu-vplist compiz-boxmenu-wlist compiz-boxmenu-daemon deskmenu-glue.h compiz-boxmenu-editor
