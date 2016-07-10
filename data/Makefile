all: menu.xml precache.ini Compiz-Boxmenu-Editor.desktop Compiz-Boxmenu-IconBrowser.desktop

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/share/icons
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	mkdir -p $(DESTDIR)$(LOCALBASE)/etc/xdg/compiz/boxmenu/
	install -m644 menu.xml                           $(DESTDIR)$(LOCALBASE)/etc/xdg/compiz/boxmenu/
	install -m644 precache.ini                       $(DESTDIR)$(LOCALBASE)/etc/xdg/compiz/boxmenu/
	install -m644 Compiz-Boxmenu-Editor.desktop      $(DESTDIR)$(PREFIX)/share/applications
	install -m644 Compiz-Boxmenu-IconBrowser.desktop $(DESTDIR)$(PREFIX)/share/applications
