# PREFIX fixing
ifneq ("$(LOCALBASE)","")
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
ifneq ("$(DEBUG)","")
	CFLAGS := -O2 -g $(WARNINGS)
else
	CFLAGS := $(WARNINGS)
endif
LDFLAGS := -Wl,--as-needed `pkg-config --libs dbus-glib-1 gdk-2.0 gtk+-2.0 libwnck-1.0`
LDFLAGS_CLIENT := -Wl,--as-needed `pkg-config --libs dbus-glib-1`

VERSION=1.1.13

# Targets

all: deskmenu-glue.h compiz-boxmenu-daemon compiz-boxmenu \
     compiz-boxmenu-pipe compiz-boxmenu-dlist \
     compiz-boxmenu-dplist compiz-boxmenu-vplist compiz-boxmenu-wlist \
	 compiz-boxmenu-editor compiz-boxmenu-iconbrowser

#has manpage
compiz-boxmenu:
	$(CC) -o $@ deskmenu.c deskmenu-common.h $(LDFLAGS_CLIENT) $(CPPFLAGS_CLIENT) $(CFLAGS)
	m4 -DVERSION=$(VERSION) man/$@.1.in > man/$@.1

compiz-boxmenu-pipe:
	$(CC) -o $@ deskmenu-pipe.c deskmenu-common.h $(LDFLAGS_CLIENT) $(CPPFLAGS_CLIENT) $(CFLAGS)
	m4 -DVERSION=$(VERSION) man/$@.1.in > man/$@.1

#has manpage
compiz-boxmenu-dlist:
	$(CC) -o $@ deskmenu-documentlist-client.c deskmenu-common.h $(LDFLAGS_CLIENT) $(CPPFLAGS_CLIENT) $(CFLAGS) 
	m4 -DVERSION=$(VERSION) man/$@.1.in > man/$@.1

#has manpage
compiz-boxmenu-vplist:
	$(CC) -o $@ deskmenu-vplist-client.c deskmenu-common.h $(LDFLAGS_CLIENT) $(CPPFLAGS_CLIENT) $(CFLAGS)
	m4 -DVERSION=$(VERSION) man/$@.1.in > man/$@.1

#has manpage
compiz-boxmenu-dplist:
	$(CC) -o $@ deskmenu-dplist-client.c deskmenu-common.h $(LDFLAGS_CLIENT) $(CPPFLAGS_CLIENT) $(CFLAGS)
	m4 -DVERSION=$(VERSION) man/$@.1.in > man/$@.1

#has manpage
compiz-boxmenu-wlist:
	$(CC) -o $@ deskmenu-windowlist-client.c deskmenu-common.h $(LDFLAGS_CLIENT) $(CPPFLAGS_CLIENT) $(CFLAGS)
	m4 -DVERSION=$(VERSION) man/$@.1.in > man/$@.1

#has manpage
compiz-boxmenu-daemon:
	$(CC) -o $@ deskmenu-menu.c deskmenu-wnck.c deskmenu-utils.c $(LDFLAGS) $(CPPFLAGS) $(CFLAGS)
	m4 -DVERSION=$(VERSION) man/$@.1.in > man/$@.1

compiz-boxmenu-editor:
	m4 -d -DLOOK_HERE=$(PREFIX)/share/cb-editor -DPYTHONBIN=$(PYTHONBIN) $@.in >$@
	m4 -DVERSION=$(VERSION) man/$@.1.in > man/$@.1

compiz-boxmenu-iconbrowser:
	m4 -d -DLOOK_HERE=$(PREFIX)/share/cb-editor -DPYTHONBIN=$(PYTHONBIN) $@.in >$@

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
	install -m755 compiz-boxmenu-iconbrowser $(DESTDIR)$(PREFIX)/bin/
	install -m644 new-editor/* $(DESTDIR)$(PREFIX)/share/cb-editor
	cp -r hicolor $(DESTDIR)$(PREFIX)/share/icons
	mkdir -p $(DESTDIR)$(LOCALBASE)/etc/xdg/compiz/boxmenu/
	install -m644 menu.xml $(DESTDIR)$(LOCALBASE)/etc/xdg/compiz/boxmenu/
	install -m644 precache.ini $(DESTDIR)$(LOCALBASE)/etc/xdg/compiz/boxmenu/
	mkdir -p $(DESTDIR)$(PREFIX)/share/dbus-1/services/
	install -m644 org.compiz_fusion.boxmenu.service $(DESTDIR)$(PREFIX)/share/dbus-1/services/
	install -m644 man/*.1 $(DESTDIR)$(PREFIX)/share/man/man1
	install -m644 Compiz-Boxmenu-Editor.desktop $(DESTDIR)$(PREFIX)/share/applications
	install -m644 Compiz-Boxmenu-IconBrowser.desktop $(DESTDIR)$(PREFIX)/share/applications

clean:
	rm -f compiz-boxmenu compiz-boxmenu-pipe compiz-boxmenu-dlist compiz-boxmenu-vplist compiz-boxmenu-dplist compiz-boxmenu-wlist compiz-boxmenu-daemon deskmenu-glue.h compiz-boxmenu-editor man/*.1
