# Targets for generating packages

# Create a package (in a different way)
pkg: $(DISTDIR).tar.gz

# Create a package archive 
$(DISTDIR).tar.gz: clean $(DISTDIR)
	tar chof - $(DISTDIR) | gzip -9 -c > $@	
	rm -rf $(DISTDIR)
	test -d archives/ || mkdir archives/
	mv $@ archives/

# Create a package directory
$(DISTDIR):
	rm -f $(DISTDIR).tar.gz
	rm -rf $(DISTDIR)
	mkdir -p \
		$(DISTDIR)/bin \
		$(DISTDIR)/etc \
		$(DISTDIR)/example \
		$(DISTDIR)/include \
		$(DISTDIR)/lib \
		$(DISTDIR)/mk \
		$(DISTDIR)/share \
		$(DISTDIR)/src \
		$(DISTDIR)/vendor
	cp $(FILES) $(DISTDIR)/
	cp -r etc/* $(DISTDIR)/etc/
	cp -r example/* $(DISTDIR)/example/
	cp -r mk/docs.mk mk/tests.mk $(DISTDIR)/mk/
	cp -r src/* $(DISTDIR)/src/
	cp -r share/* $(DISTDIR)/share/
	cp -r vendor/*.[ch] vendor/lua-$(LUAVER)/ $(DISTDIR)/vendor/

# Check that packaging worked (super useful for other pkgributions...) 
pkgcheck:
	gzip -cd $(DISTDIR).tar.gz | tar xvf -
	cd $(DISTDIR) && ./configure
	cd $(DISTDIR) && $(MAKE)
	cd $(DISTDIR) && $(MAKE) clean
	rm -rf $(DISTDIR)
	@echo "*** package $(DISTDIR).tar.gz is ready for distribution."

# pkgclean - Run `clean` in prep for a clean package
pkgclean: clean
	-@rm -rf $(srcdir)/lib/ $(srcdir)/include/
	-@rm -rf autom4te.cache config.guess config.log config.status config.sub configure Makefile

# pkgboot - Run a bootstrapping procedure to initialize autoconf
pkgboot:
	autoupdate && autoreconf --install

# pkgtest - RUn an automated build against a different OS
#pkgtest:
#	test ! -z $(REMOTE)
#	test -f $(DISTDIR).tar.gz
#	scp $(DISTDIR).tar.gz $(REMOTE):~/
#	ssh $(REMOTE) ' \
#		rm -rf $(DISTDIR); \
#		tar xzf $(DISTDIR).tar.gz && \
#		cd $(DISTDIR)/ && \
#		./configure --enable-local-lua --enable-local-sqlite3 && \
#		make -j2'	

# check - Run common tests
check: main
	@echo "*** all tests passed"	

