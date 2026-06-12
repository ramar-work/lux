# Keep this to run a server specifically for these demos
LXSRV=../../test/bin/lxsrv
LXPORT=2987
LXFLAGS=--start --user $(USER):$(USER) --port $(LXPORT) -L ./lxlog.sqlite3

server:
	$(LXSRV) $(LXFLAGS) -c ./config.lua
