# tests.mk
# ========
# ?

# sstest - Start a server for package testing 
sstest: PORT=2222
sstest: ADDR=http://localhost:$(PORT)
sstest: CONFIG=$(srcdir)/example/config.lua
sstest: dev
	@$(srcdir)/bin/$(BINNAME)-server --start -l /dev/null -a /dev/null -p $(PORT) -u $$USER -g $$USER -c $(CONFIG) 2>/dev/null & echo $$! > /tmp/pid
	sleep 2


# kill
kill:
	-@kill -9 `cat /tmp/pid`



# config - Generate configuration file for a server (...)
config:
	sed 's//' share/server.config.lua config.lua	


# test - Create a new server and instance for package testing 
test: PORT=2222
test: ADDR=http://localhost:$(PORT)
test: CONFIG=$(srcdir)/example/config.lua
test: OPTIONS=--start -l /dev/null -a /dev/null -p $(PORT) -u $$USER -g $$USER -c $(CONFIG)
test: TESTDIR=workspace
test: dev config
	# Make a new directory for the testing server
	@mkdir $(TESTDIR)/
	# Create a test application (consider using TEST_ for localvars)
	@$(srcdir)/bin/lxpkg --create -t application -d $(TESTDIR)/example/myapp
	# Start a test instance 
	@$(srcdir)/bin/$(BINNAME)-server --start -l /dev/null -a /dev/null -p $(PORT) -u $$USER -g $$USER -c $(CONFIG) 2>/dev/null & echo $$! > /tmp/pid
	# Kill the process
	-@kill -9 `cat /tmp/pid`
	# Probably need to delete the new server too
	-@rm -f $(srcdir)/test



# server-test - Start a server and see if packets can be received (this is an odd test)
server-test: PORT=2222
server-test: ADDR=http://localhost:$(PORT)
server-test: CONFIG=$(srcdir)/example/config.lua
server-test: OPTIONS=--start -l /dev/null -a /dev/null -p $(PORT) -u $$USER -g $$USER -c $(CONFIG)
server-test:
	@$(srcdir)/bin/$(BINNAME)-server $(OPTIONS) & echo $$! > ./server-test
	@wget -qSO- $(ADDR) 2>&1 | grep '200 OK' >/dev/null && echo "Test OK" || echo "Test failed!"
	-@kill -9 `cat $(srcdir)/test`
	-@rm -f $(srcdir)/test


# tests - Build some common tests
tests:
	cd src/lua/tests && $(MAKE) -f Makefile


# hello_world_package - Deploy a package 'hello_world' to the instance 'example/api.test'
# TODO: Start the server before hand and shut it down regardless of success or failure?
hello_world_package: dev
	bin/lxpkg --instance example/api.test --package file://extensions/hello_world.tgz --verbose


# create_instance - Creates an instance and checks that all files were deployed the way they should have been
create_instance: DIR=workspace/markdown
create_instance:
	bin/lxpkg --create --directory $(DIR) --type application --verbose


# create_package - Creates an instance and checks that all files were deployed the way they should have been
create_package: DIR=workspace/markdown
create_package:
	bin/lxpkg --create --directory $(DIR) --type package --verbose

