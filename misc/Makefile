include $(CURDIR)/../inc/version.make
include $(CURDIR)/../inc/prefix_fix.make
include $(CURDIR)/../inc/pyver.make

install:
	$(PYTHONBIN) ./setup.py install --root=$(DESTDIR) --prefix=$(PREFIX)

clean:
	$(PYTHONBIN) ./setup.py clean
