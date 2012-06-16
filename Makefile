PREFIX := /usr

CPPFLAGS := `pkg-config --cflags dbus-glib-1 gdk-3.0 gtk+-3.0 libwnck-3.0`
CPPFLAGS_CLIENT := `pkg-config --cflags dbus-glib-1`
WARNINGS := -Wall -Wextra -Wno-unused-parameter
CFLAGS := -O2 -g $(WARNINGS)
LDFLAGS := `pkg-config --libs dbus-glib-1 gdk-3.0 gtk+-3.0 libwnck-3.0`
LDFLAGS_CLIENT := `pkg-config --libs dbus-glib-1`

all: compiz-boxmenu-daemon compiz-boxmenu compiz-boxmenu-dlist compiz-boxmenu-vplist compiz-boxmenu-wlist

compiz-boxmenu: deskmenu.c deskmenu-common.h
	$(CC) $(CPPFLAGS_CLIENT) $(CFLAGS) $(LDFLAGS_CLIENT) -o $@ $<

compiz-boxmenu-dlist: deskmenu-documentlist-client.c deskmenu-common.h
	$(CC) $(CPPFLAGS_CLIENT) $(CFLAGS) $(LDFLAGS_CLIENT) -o $@ $<

compiz-boxmenu-vplist: deskmenu-vplist-client.c deskmenu-common.h
	$(CC) $(CPPFLAGS_CLIENT) $(CFLAGS) $(LDFLAGS_CLIENT) -o $@ $<

compiz-boxmenu-wlist: deskmenu-windowlist-client.c deskmenu-common.h
	$(CC) $(CPPFLAGS_CLIENT) $(CFLAGS) $(LDFLAGS_CLIENT) -o $@ $<

compiz-boxmenu-daemon: deskmenu-menu.c deskmenu-wnck.c deskmenu-utils.c deskmenu-utils.h deskmenu-wnck.h deskmenu-glue.h deskmenu-common.h deskmenu-menu.h

	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ deskmenu-menu.c deskmenu-wnck.c deskmenu-utils.c

deskmenu-glue.h: deskmenu-service.xml
	dbus-binding-tool --mode=glib-server --prefix=deskmenu --output=$@ $^

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin/
	mkdir -p $(DESTDIR)$(PREFIX)/share/icons
	mkdir -p $(DESTDIR)$(PREFIX)/share/cb-editor
	install compiz-boxmenu $(DESTDIR)$(PREFIX)/bin/
	install compiz-boxmenu-dlist $(DESTDIR)$(PREFIX)/bin/
	install compiz-boxmenu-vplist $(DESTDIR)$(PREFIX)/bin/
	install compiz-boxmenu-wlist $(DESTDIR)$(PREFIX)/bin/
	install compiz-boxmenu-daemon $(DESTDIR)$(PREFIX)/bin/
	install compiz-boxmenu-editor $(DESTDIR)$(PREFIX)/bin/
	install new-editor/* $(DESTDIR)$(PREFIX)/share/cb-editor
	cp -r hicolor $(DESTDIR)$(PREFIX)/share/icons
	mkdir -p $(DESTDIR)/etc/xdg/compiz/boxmenu/
	install menu.xml $(DESTDIR)/etc/xdg/compiz/boxmenu/
	install precache.ini $(DESTDIR)/etc/xdg/compiz/boxmenu/
	mkdir -p $(DESTDIR)$(PREFIX)/share/dbus-1/services/
	install org.compiz_fusion.boxmenu.service $(DESTDIR)$(PREFIX)/share/dbus-1/services/

clean:
	rm -f compiz-boxmenu compiz-boxmenu-dlist compiz-boxmenu-vplist compiz-boxmenu-wlist compiz-boxmenu-daemon deskmenu-glue.h

