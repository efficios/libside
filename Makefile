SUBDIRS := src/

all: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@

clean:
	$(MAKE) clean -C src/

.PHONY: all $(SUBDIRS) clean clean-$(SUBDIRS)
