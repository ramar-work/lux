# local.mk - Tests features locally without installing system wide
_DIR=$(shell realpath .build)

# .build - Run a test build of Lux and install it locally.
.build: .build-configure .build-make .build-install .build-env
	echo "Installed Lux to $(_DIR), use \`source .build/env\` to utilize it."	

# .build-configure - Configure for local, "hidden" build
.build-configure:
	# Run a configure with most options set 
	./configure \
		--prefix=$(_DIR)/ \
		--exec-prefix=$(_DIR)/ \
		--localstatedir=$(_DIR)/var \
		--datarootdir=$(_DIR)/share \
		--sysconfdir=$(_DIR)/etc \
		--enable-local-lua \
		--enable-local-sqlite3 \
		--enable-debugging \
		--with-www-root=$(_DIR)/www \
		--with-www-user=$(USER) \
		--with-www-group=users \
		--disable-tls-support \
		--disable-systemd \
		--disable-examples

# .build-make - Make it
.build-make:
	make -j2

# .build-install- Make it
.build-install:
	make install
	make -j2

# .build-env - Create an environment file (for bash) for testing.
.build-env:
	printf "export PATH=\"\$$(realpath $(_DIR)):\$${PATH}\"\n" > $(_DIR)/env
	printf "export PS1='[\[\033[01;32m\]\u@\h (*lux*) | \d, \@ | \j | \! | \w\[\033[00m\] ]\n\$$ '" >> $(_DIR)/env

.PHONY: .build
