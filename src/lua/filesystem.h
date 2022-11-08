/* ------------------------------------------- * 
 * filesystem.h 
 * ============
 * 
 * Summary 
 * -------
 * -
 *
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 *
 * See LICENSE in the top-level directory for more information.
 *
 * CHANGELOG 
 * ---------
 * -
 * ------------------------------------------- */
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <zdb.h>
#include <zhttp.h>
#include <ztable.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "../lua.h"
#include "../util.h"

#ifndef LFS_H
#define LFS_H

int fs_open ( lua_State * );
int fs_read ( lua_State * );
int fs_close ( lua_State * );
int fs_stat ( lua_State * );
int fs_list ( lua_State * );
int fs_write ( lua_State * );
extern struct luaL_Reg fs_set[];

#endif

