include $(CURDIR)/../inc/version.make
include $(CURDIR)/../inc/prefix_fix.make

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

# Targets

all: deskmenu-glue.h       compiz-boxmenu-daemon compiz-boxmenu \
     compiz-boxmenu-pipe   compiz-boxmenu-dlist \
     compiz-boxmenu-dplist compiz-boxmenu-vplist compiz-boxmenu-wlist

#has manpage
compiz-boxmenu:
	$(CC) -o $@ deskmenu.c deskmenu-common.h $(LDFLAGS_CLIENT) $(CPPFLAGS_CLIENT) $(CFLAGS)

compiz-boxmenu-pipe:
	$(CC) -o $@ deskmenu-pipe.c deskmenu-common.h $(LDFLAGS_CLIENT) $(CPPFLAGS_CLIENT) $(CFLAGS)

#has manpage
compiz-boxmenu-dlist:
	$(CC) -o $@ deskmenu-documentlist-client.c deskmenu-common.h $(LDFLAGS_CLIENT) $(CPPFLAGS_CLIENT) $(CFLAGS) 

#has manpage
compiz-boxmenu-vplist:
	$(CC) -o $@ deskmenu-vplist-client.c deskmenu-common.h $(LDFLAGS_CLIENT) $(CPPFLAGS_CLIENT) $(CFLAGS)

#has manpage
compiz-boxmenu-dplist:
	$(CC) -o $@ deskmenu-dplist-client.c deskmenu-common.h $(LDFLAGS_CLIENT) $(CPPFLAGS_CLIENT) $(CFLAGS)

#has manpage
compiz-boxmenu-wlist:
	$(CC) -o $@ deskmenu-windowlist-client.c deskmenu-common.h $(LDFLAGS_CLIENT) $(CPPFLAGS_CLIENT) $(CFLAGS)

#has manpage
compiz-boxmenu-daemon:
	$(CC) -o $@ deskmenu-menu.c deskmenu-wnck.c deskmenu-utils.c $(LDFLAGS) $(CPPFLAGS) $(CFLAGS)

deskmenu-glue.h:
	dbus-binding-tool --mode=glib-server --prefix=deskmenu --output=$@ deskmenu-service.xml

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin/
	mkdir -p $(DESTDIR)$(PREFIX)/share/dbus-1/services/
	install -m755 compiz-boxmenu        $(DESTDIR)$(PREFIX)/bin/
	install -m755 compiz-boxmenu-dlist  $(DESTDIR)$(PREFIX)/bin/
	install -m755 compiz-boxmenu-dplist $(DESTDIR)$(PREFIX)/bin/
	install -m755 compiz-boxmenu-vplist $(DESTDIR)$(PREFIX)/bin/
	install -m755 compiz-boxmenu-wlist  $(DESTDIR)$(PREFIX)/bin/
	install -m755 compiz-boxmenu-daemon $(DESTDIR)$(PREFIX)/bin/
	install -m644 org.compiz_fusion.boxmenu.service $(DESTDIR)$(PREFIX)/share/dbus-1/services/

clean:
	rm -f compiz-boxmenu compiz-boxmenu-pipe compiz-boxmenu-dlist compiz-boxmenu-vplist compiz-boxmenu-dplist compiz-boxmenu-wlist compiz-boxmenu-daemon deskmenu-glue.h
