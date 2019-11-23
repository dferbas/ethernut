/*
** $Id: lstring.h 4116 2012-04-12 22:35:23Z olereinhardt $
** String table (keep all strings handled by Lua)
** See Copyright Notice in lua.h
*/

#ifndef lstring_h
#define lstring_h


#include <lua/lgc.h>
#include <lua/lobject.h>
#include <lua/lstate.h>


#define sizestring(s)   (sizeof(union TString)+((s)->len+1)*sizeof(char))

#define sizeudata(u)    (sizeof(union Udata)+(u)->len)

#define luaS_new(L, s)  (luaS_newlstr(L, s, strlen(s)))
#define luaS_newliteral(L, s)   (luaS_newlstr(L, "" s, \
                                 (sizeof(s)/sizeof(char))-1))

#define luaS_fix(s) l_setbit((s)->tsv.marked, FIXEDBIT)

LUAI_FUNC void luaS_resize (lua_State *L, int newsize);
LUAI_FUNC Udata *luaS_newudata (lua_State *L, size_t s, Table *e);
LUAI_FUNC TString *luaS_newlstr (lua_State *L, const char *str, size_t l);


#endif
