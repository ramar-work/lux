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
		$(DISTDIR)/include \
		$(DISTDIR)/lib \
		$(DISTDIR)/mk \
		$(DISTDIR)/share \
		$(DISTDIR)/src \
		$(DISTDIR)/vendor \
		$(DISTDIR)/www
	cp $(FILES) $(DISTDIR)/
	cp -r etc/* $(DISTDIR)/etc/
	cp -r mk/docs.mk mk/tests.mk $(DISTDIR)/mk/
	cp -r src/* $(DISTDIR)/src/
	cp -r share/* $(DISTDIR)/share/
	cp -r vendor/*.[ch] vendor/lua-$(LUAVER)/ $(DISTDIR)/vendor/
	cp -r www/demos $(DISTDIR)/www/
	cp -r www/docs $(DISTDIR)/www/

# Check that packaging worked (super useful for other pkgributions...) 
pkgcheck:
	gzip -cd archives/$(DISTDIR).tar.gz | tar xvf -
	cd $(DISTDIR) && \
		./configure \
			--disable-tls-support \
			--disable-systemd \
			--enable-local-lua \
			--enable-local-sqlite3 \
			--prefix=`realpath ./$(DISTDIR)` \
			--localstatedir=`realpath ./$(DISTDIR)`/var \
			--datarootdir=`realpath ./$(DISTDIR)`/share \
			--sysconfdir=`realpath ./$(DISTDIR)`/etc \
			--with-www-root=`realpath ./$(DISTDIR)`/www \
			--with-www-user=$(USER) \
			--with-www-group=users
	cd $(DISTDIR) && \
		$(MAKE) -j2
	cd $(DISTDIR) && \
		$(MAKE) install 
	cd $(DISTDIR) && \
		$(MAKE) uninstall 
	cd $(DISTDIR) && \
		$(MAKE) clean
	rm -rf $(DISTDIR)
	@echo "*** package $(DISTDIR).tar.gz is ready for distribution."

# pkgclean - Run `clean` in prep for a clean package
pkgclean: clean
	-@rm -rf $(srcdir)/lib/ $(srcdir)/include/
	-@rm -rf autom4te.cache config.guess config.log config.status config.sub configure Makefile

# pkgboot - Run a bootstrapping procedure to initialize autoconf
pkgboot:
	autoupdate && autoreconf --install

# pkgtest - Run an automated build against a different OS
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

