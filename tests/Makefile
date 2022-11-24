SUBDIRS := regression/

all: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@

clean:
	$(MAKE) clean -C regression/

.PHONY: all $(SUBDIRS) clean clean-$(SUBDIRS)
