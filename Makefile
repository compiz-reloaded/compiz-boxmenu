DIRS = art data man menu_editor misc src

define make-descend
	for d in $(DIRS); do $(MAKE) -C $$d $@; done
endef

.PHONY: all install clean
all:
	$(call make-descend all)

install:
	$(call make-descend install)

clean:
	$(call make-descend clean)
