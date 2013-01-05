ifdef ($(LOCALBASE))
	PREFIX=$(LOCALBASE)
else
	PREFIX?=/usr
endif

CPPFLAGS := `pkg-config --cflags dbus-glib-1 gdk-3.0 gtk+-3.0 libwnck-3.0`
CPPFLAGS_CLIENT := `pkg-config --cflags dbus-glib-1`
WARNINGS := -Wall -Wextra -Wno-unused-parameter
CFLAGS := -O2 -g $(WARNINGS)
LDFLAGS := `pkg-config --libs dbus-glib-1 gdk-3.0 gtk+-3.0 libwnck-3.0`
LDFLAGS_CLIENT := `pkg-config --libs dbus-glib-1`

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
	m4 -DLOOK_HERE=$(PREFIX)/share/cb-editor $@.in >$@

deskmenu-glue.h:
	dbus-binding-tool --mode=glib-server --prefix=deskmenu --output=$@ deskmenu-service.xml

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin/
	mkdir -p $(DESTDIR)$(PREFIX)/share/icons
	mkdir -p $(DESTDIR)$(PREFIX)/share/cb-editor
	install compiz-boxmenu $(DESTDIR)$(PREFIX)/bin/
	install compiz-boxmenu-dlist $(DESTDIR)$(PREFIX)/bin/
	install compiz-boxmenu-vplist $(DESTDIR)$(PREFIX)/bin/
	install compiz-boxmenu-wlist $(DESTDIR)$(PREFIX)/bin/
	install compiz-boxmenu-daemon $(DESTDIR)$(PREFIX)/bin/
	install -Dm755 compiz-boxmenu-editor $(DESTDIR)$(PREFIX)/bin/
	install new-editor/* $(DESTDIR)$(PREFIX)/share/cb-editor
	cp -r hicolor $(DESTDIR)$(PREFIX)/share/icons
	mkdir -p $(DESTDIR)$(LOCALBASE)/etc/xdg/compiz/boxmenu/
	install menu.xml $(DESTDIR)$(LOCALBASE)/etc/xdg/compiz/boxmenu/
	install precache.ini $(DESTDIR)$(LOCALBASE)/etc/xdg/compiz/boxmenu/
	mkdir -p $(DESTDIR)$(PREFIX)/share/dbus-1/services/
	install org.compiz_fusion.boxmenu.service $(DESTDIR)$(PREFIX)/share/dbus-1/services/

clean:
	rm -f compiz-boxmenu compiz-boxmenu-dlist compiz-boxmenu-vplist compiz-boxmenu-wlist compiz-boxmenu-daemon deskmenu-glue.h


