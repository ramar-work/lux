# system.mk - Builds & installs a new version of Lux system wide
.system: veryclean .system-configure .system-make
	echo "Installed Lux to /usr/local."	

# .system-configure - Configure for a regular system build
.system-configure:
	./configure \
		--with-www-root=$(HOME)/www \
		--with-www-user=$(USER) \
		--with-www-group=users \
		--enable-local-lua \
		--enable-local-sqlite3 \
		--enable-debugging \
		--disable-tls-support \
		--disable-examples

# .system-make - Run build 
.system-make:
	make clangdebug

# .system-uninstall- Uninstall the system build 
.system-uninstall:
	make uninstall
