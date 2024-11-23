# PREFIX fixing
ifneq ("$(LOCALBASE)","")
	PREFIX=$(LOCALBASE)
else
	PREFIX?=/usr
endif

PKG_CONFIG ?= pkg-config

# checking for python
PYTHONBIN?=$(shell which python)

ifeq ("$(PYTHONBIN)", "")
$(error Python not found.)
endif

# Set up compile flags
CPPFLAGS ?=
CPPFLAGS_CLIENT := $(CPPFLAGS) `$(PKG_CONFIG) --cflags dbus-glib-1`
CPPFLAGS += `$(PKG_CONFIG) --cflags dbus-glib-1 gdk-3.0 gtk+-3.0 libwnck-3.0`
WARNINGS := -Wall -Wextra -Wno-unused-parameter
ifneq ("$(DEBUG)","")
	CFLAGS ?= -O2 -g
else
	CFLAGS ?=
endif
CFLAGS += $(WARNINGS)
LDFLAGS ?=
LDFLAGS_CLIENT := $(LDFLAGS) -Wl,--as-needed `$(PKG_CONFIG) --libs dbus-glib-1`
LDFLAGS += -Wl,--as-needed `$(PKG_CONFIG) --libs dbus-glib-1 gdk-3.0 gtk+-3.0 libwnck-3.0`

VERSION=1.1.12

# Targets

all: deskmenu-glue.h compiz-boxmenu-daemon compiz-boxmenu compiz-boxmenu-dlist compiz-boxmenu-dplist compiz-boxmenu-vplist compiz-boxmenu-wlist compiz-boxmenu-editor

#has manpage
compiz-boxmenu:
	$(CC) -o $@ deskmenu.c $(LDFLAGS_CLIENT) $(CPPFLAGS_CLIENT) $(CFLAGS)  
	m4 -DVERSION=$(VERSION) man/$@.1.in > man/$@.1

#has manpage
compiz-boxmenu-dlist:
	$(CC) -o $@ deskmenu-documentlist-client.c $(LDFLAGS_CLIENT) $(CPPFLAGS_CLIENT) $(CFLAGS) 
	m4 -DVERSION=$(VERSION) man/$@.1.in > man/$@.1

#has manpage
compiz-boxmenu-vplist:
	$(CC) -o $@ deskmenu-vplist-client.c $(LDFLAGS_CLIENT) $(CPPFLAGS_CLIENT) $(CFLAGS)
	m4 -DVERSION=$(VERSION) man/$@.1.in > man/$@.1

#has manpage
compiz-boxmenu-dplist:
	$(CC) -o $@ deskmenu-dplist-client.c $(LDFLAGS_CLIENT) $(CPPFLAGS_CLIENT) $(CFLAGS) 
	m4 -DVERSION=$(VERSION) man/$@.1.in > man/$@.1

#has manpage
compiz-boxmenu-wlist:
	$(CC) -o $@ deskmenu-windowlist-client.c $(LDFLAGS_CLIENT) $(CPPFLAGS_CLIENT) $(CFLAGS)
	m4 -DVERSION=$(VERSION) man/$@.1.in > man/$@.1

#has manpage
compiz-boxmenu-daemon: deskmenu-glue.h
	$(CC) -o $@ deskmenu-menu.c deskmenu-wnck.c deskmenu-utils.c $(LDFLAGS) $(CPPFLAGS) $(CFLAGS) 
	m4 -DVERSION=$(VERSION) man/$@.1.in > man/$@.1

compiz-boxmenu-editor:
	m4 -d -DLOOK_HERE=$(PREFIX)/share/cb-editor -DPYTHONBIN=$(PYTHONBIN) $@.in >$@
	m4 -DVERSION=$(VERSION) man/$@.1.in > man/$@.1

deskmenu-glue.h:
	dbus-binding-tool --mode=glib-server --prefix=deskmenu --output=$@ deskmenu-service.xml

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin/
	mkdir -p $(DESTDIR)$(PREFIX)/share/icons
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	mkdir -p $(DESTDIR)$(PREFIX)/share/cb-editor
	mkdir -p $(DESTDIR)$(PREFIX)/share/man/man1
	install -m755 compiz-boxmenu $(DESTDIR)$(PREFIX)/bin/
	install -m755 compiz-boxmenu-dlist $(DESTDIR)$(PREFIX)/bin/
	install -m755 compiz-boxmenu-dplist $(DESTDIR)$(PREFIX)/bin/
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
	install -m644 man/*.1 $(DESTDIR)$(PREFIX)/share/man/man1
	install -m644 Compiz-Boxmenu-Editor.desktop $(DESTDIR)$(PREFIX)/share/applications

clean:
	rm -f compiz-boxmenu compiz-boxmenu-dlist compiz-boxmenu-vplist compiz-boxmenu-dplist compiz-boxmenu-wlist compiz-boxmenu-daemon deskmenu-glue.h compiz-boxmenu-editor man/*.1
