SUBDIRS := src/ tests/

all: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@

clean:
	for dir in $(SUBDIRS); do $(MAKE) clean -C $$dir; done

.PHONY: all $(SUBDIRS) clean
