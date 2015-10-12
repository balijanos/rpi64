/*    Copyright (c) 2010 Andreas Krinke
 *
 *    Permission is hereby granted, free of charge, to any person obtaining a copy
 *    of this software and associated documentation files (the "Software"), to deal
 *    in the Software without restriction, including without limitation the rights
 *    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *    copies of the Software, and to permit persons to whom the Software is
 *    furnished to do so, subject to the following conditions:
 *    
 *    The above copyright notice and this permission notice shall be included in
 *    all copies or substantial portions of the Software.
 *    
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *    THE SOFTWARE.
 */ 

#include <stdlib.h>

#include "chipmunk.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "cpBody-lua.h"
#include "cpConstraint-lua.h"
#include "cpMisc-lua.h"
#include "cpVect-lua.h"

#define check_cpGrooveJoint(L, index) \
  *(cpGrooveJoint **)luaL_checkudata(L, (index), "cpGrooveJoint")

static cpGrooveJoint *push_cpGrooveJoint (lua_State *L) {
  cpGrooveJoint *sj = cpGrooveJointAlloc();
  cpGrooveJoint **psj = (cpGrooveJoint **)lua_newuserdata(L, sizeof(cpGrooveJoint*));
  *psj = sj;

  luaL_getmetatable(L, "cpGrooveJoint");
  lua_setmetatable(L, -2);

  return sj;
}

static int cpGrooveJoint_new (lua_State *L) {
  cpBody *b1 = check_cpBody(L, 1);
  cpBody *b2 = check_cpBody(L, 2);
  cpVect g1 = check_cpVect(L, 3);
  cpVect g2 = check_cpVect(L, 5);
  cpVect a2 = check_cpVect(L, 7);

  cpGrooveJoint *gj = push_cpGrooveJoint(L);
  cpGrooveJointInit(gj, b1, b2, g1, g2, a2);

  cpConstraint_store_refs(L);
  return 1;
}

WCP_DefineConstraintGetterSetterVect(GrooveJoint, Anchor2, anchr2);

static int cpGrooveJoint_tostring (lua_State *L) {
  lua_pushfstring(L, "cpGrooveJoint (%p)", lua_topointer(L, 1));
  return 1;
}

static const luaL_reg cpGrooveJoint_functions[] = {
  {"newGrooveJoint", cpGrooveJoint_new},
  {NULL, NULL}
};

static const luaL_reg cpGrooveJoint_methods[] = {
  WCP_METHOD(GrooveJoint, getAnchor2),
  WCP_METHOD(GrooveJoint, setAnchor2),
  DEFINE_CONSTRAINT_METHODS,
  {NULL, NULL}
};

static const luaL_reg cpGrooveJoint_meta[] = {
  {"__gc",       cpConstraint_gc},
  {"__tostring", cpGrooveJoint_tostring},
  {NULL, NULL}
};

int cpGrooveJoint_register (lua_State *L) {
  luaL_register(L, NULL, cpGrooveJoint_functions);

  luaL_newmetatable(L, "cpGrooveJoint");
  luaL_register(L, NULL, cpGrooveJoint_meta);
  /* metatable.__index = methods table */
  lua_pushliteral(L, "__index");
  lua_newtable(L);
  luaL_register(L, NULL, cpGrooveJoint_methods);
  lua_rawset(L, -3);

  /* cpConstraints.metatable = 1 */
  lua_pushliteral(L, "cpConstraints");
  lua_rawget(L, LUA_REGISTRYINDEX);
  lua_pushvalue(L, -2);
  lua_pushinteger(L, 1);
  lua_rawset(L, -3);

  /* drop metatable and cpConstraints table */
  lua_pop(L, 2);

  return 0;
}
