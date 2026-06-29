# Build

# Run a couple of invocations with different arguments and see how they turn out

srcdir=../../
realdir=$(shell realpath $(srcdir))
port=8888
dir=$(realdir)/test
LXCLI=$(realdir)/test/bin/lxcli


# all - Run all tests
all: test-configure test-install test-instance clean

# custom - Run all tests with as many custom rules as possible 
test-custom-configure:
	# Run a configure with most options set 
	@cd $(realdir) && ./configure \
		--prefix=$(dir)/ \
		--exec-prefix=$(dir)/ \
		--localstatedir=$(dir)/var \
		--datarootdir=$(dir)/share \
		--sysconfdir=$(dir)/etc \
		--enable-local-lua \
		--enable-local-sqlite3 \
		--enable-debugging \
		--with-www-root=$(dir)/www \
		--with-www-user=$(USER) \
		--with-www-group=users \
		--disable-systemd \
		--disable-examples
	# Check for config.h and any other files that should have been generated from .in files
	test -f $(realdir)/src/config.h 
	test -f $(realdir)/Makefile


# test-configure - Tests a local build by making sure that installed files are indeed in the right place
test-configure:
	# Run a configure with most options set 
	@cd $(realdir) && ./configure \
		--prefix=$(dir)/ \
		--enable-local-lua \
		--enable-local-sqlite3 \
		--enable-debugging \
		--with-www-root=$(dir)/www \
		--with-www-user=$(USER) \
		--with-www-group=users \
		--disable-systemd \
		--disable-examples
	# Check for config.h and any other files that should have been generated from .in files
	test -f $(realdir)/src/config.h 
	test -f $(realdir)/Makefile


# test-install
test-install:
	# Make
	@cd $(realdir) && make -j4
	# Install
	@cd $(realdir) && make install
	# Test that the directories were created
	test -d $(realdir)/test/bin/
	test -d $(realdir)/test/etc/
	test -d $(realdir)/test/log/lux/
	test -d $(realdir)/test/share/lux/
	# Test that the files were copied 
	test -f $(realdir)/test/share/lux/app.hello.lua
	test -f $(realdir)/test/share/lux/app.README.md
	test -f $(realdir)/test/share/lux/app.stub.lua
	test -f $(realdir)/test/share/lux/assets.README.md
	test -f $(realdir)/test/share/lux/assets.standard.css
	test -f $(realdir)/test/share/lux/config.example.lua
	test -f $(realdir)/test/share/lux/config.instance.lua
	test -f $(realdir)/test/share/lux/config.lua
	test -f $(realdir)/test/share/lux/config.server.lua
	test -f $(realdir)/test/share/lux/db.README.md
	test -f $(realdir)/test/share/lux/etc.lux.config.lua
	test -f $(realdir)/test/share/lux/favicon.ico
	test -f $(realdir)/test/share/lux/favicon.png
	test -f $(realdir)/test/share/lux/index.html
	test -f $(realdir)/test/share/lux/intermediateCA-openssl.cnf
	test -f $(realdir)/test/share/lux/lib.README.md
	test -f $(realdir)/test/share/lux/Makefile
	test -f $(realdir)/test/share/lux/Makefile.in
	test -f $(realdir)/test/share/lux/misc.README.md
	test -f $(realdir)/test/share/lux/openssl-intermediateCA.cnf
	test -f $(realdir)/test/share/lux/openssl-rootCA.cnf
	test -f $(realdir)/test/share/lux/package.exclude
	test -f $(realdir)/test/share/lux/package.gitignore
	test -f $(realdir)/test/share/lux/package.Makefile
	test -f $(realdir)/test/share/lux/package.Makefile.in
	test -f $(realdir)/test/share/lux/package.README.md
	test -f $(realdir)/test/share/lux/private.README.md
	test -f $(realdir)/test/share/lux/robots.txt
	test -f $(realdir)/test/share/lux/rootCA-openssl.conf
	test -f $(realdir)/test/share/lux/routes.README.md
	test -f $(realdir)/test/share/lux/sql.README.md
	test -f $(realdir)/test/share/lux/src.README.md
	test -f $(realdir)/test/share/lux/tests.README.md
	test -f $(realdir)/test/share/lux/views.hello.tpl
	test -f $(realdir)/test/share/lux/views.README.md
	test -f $(realdir)/test/share/lux/views.stub.tpl


# test-instance - Tests that the suite is doing what its supposed to do
test-instance:
	# Check for a binary in the right place
	# Check that this command even works
	$(LXCLI) -d $(realdir)/test/www/wwwx -c
	# Test that the directory looks right
	test -d $(realdir)/test/www/wwwx
	test -f $(realdir)/test/www/wwwx/config.example.lua
	test -f $(realdir)/test/www/wwwx/config.lua
	test -f $(realdir)/test/www/wwwx/favicon.ico
	test -f $(realdir)/test/www/wwwx/robots.txt
	#test -f $(realdir)/test/www/wwwx/.gitignore
	test -d $(realdir)/test/www/wwwx/app
	test -f $(realdir)/test/www/wwwx/app/hello.lua
	test -d $(realdir)/test/www/wwwx/assets
	test -d $(realdir)/test/www/wwwx/db
	test -d $(realdir)/test/www/wwwx/lib
	test -d $(realdir)/test/www/wwwx/lib/dependencies
	test -d $(realdir)/test/www/wwwx/misc
	test -d $(realdir)/test/www/wwwx/misc/certs
	test -d $(realdir)/test/www/wwwx/private
	test -d $(realdir)/test/www/wwwx/private/setup
	test -d $(realdir)/test/www/wwwx/routes
	test -d $(realdir)/test/www/wwwx/sql
	test -d $(realdir)/test/www/wwwx/src
	test -d $(realdir)/test/www/wwwx/tests
	test -d $(realdir)/test/www/wwwx/views
	test -f $(realdir)/test/www/wwwx/views/hello.tpl

# test-uninstall - Tests that uninstall works
test-uninstall:
	# Make
	@cd $(realdir) && make uninstall

# clean - Clean previous results in test up
clean:
	rm -rf $(realdir)/test/
