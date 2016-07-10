include $(CURDIR)/../inc/version.make
include $(CURDIR)/../inc/prefix_fix.make

all: compiz-boxmenu-daemon.1 compiz-boxmenu.1 \
     compiz-boxmenu-pipe.1   compiz-boxmenu-dlist.1 \
     compiz-boxmenu-dplist.1 compiz-boxmenu-vplist.1 compiz-boxmenu-wlist.1 \
     compiz-boxmenu-editor.1

%.1: %.1.in
	m4 -DVERSION=$(VERSION) $< > $@

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/share/man/man1
	install -m644 *.1 $(DESTDIR)$(PREFIX)/

clean:
	rm -f *.1
