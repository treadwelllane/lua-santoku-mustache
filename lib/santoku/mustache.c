#include <lua.h>
#include <lauxlib.h>
#include <santoku/lua/utils.h>
#include <string.h>
#include <stdio.h>

#include "cjson/cJSON.h"
#include "mustach-cjson.h"
#include "mustach-wrap.h"

#define MUSTACH_MAX_DEPTH 256

struct lua_mustache_context {
  lua_State *L;
  int root_idx;
  int partials_idx;
  int depth;
  struct {
    int obj_idx;
    int cont_idx;
    int iter_idx;
    int is_array;
    int array_len;
    int is_objiter;
    int key_idx;
  } stack[MUSTACH_MAX_DEPTH];
  int selection_idx;
  char numbuf[64];
  char keybuf[64];
};

static int table_array_len(lua_State *L, int idx) {
  int count = 0;
  int max = 0;
  idx = (idx > 0) ? idx : lua_gettop(L) + idx + 1;
  lua_pushnil(L);
  while (lua_next(L, idx)) {
    if (lua_type(L, -2) != LUA_TNUMBER) {
      lua_pop(L, 2);
      return 0;
    }
    double num = lua_tonumber(L, -2);
    int key = (int)num;
    if (num != (double)key || key < 1) {
      lua_pop(L, 2);
      return 0;
    }
    if (key > max)
      max = key;
    count++;
    lua_pop(L, 1);
  }
  return (count > 0 && max == count) ? count : 0;
}

static int is_truthy(lua_State *L, int idx) {
  switch (lua_type(L, idx)) {
    case LUA_TNIL: return 0;
    case LUA_TBOOLEAN: return lua_toboolean(L, idx);
    case LUA_TNUMBER: return lua_tonumber(L, idx) != 0;
    case LUA_TSTRING: {
      size_t len;
      lua_tolstring(L, idx, &len);
      return len > 0;
    }
    case LUA_TTABLE: {
      lua_pushnil(L);
      int has = lua_next(L, idx);
      if (has) lua_pop(L, 2);
      return has;
    }
    default: return 0;
  }
}

static int lua_mustache_start(void *closure) {
  struct lua_mustache_context *ctx = closure;
  ctx->depth = 0;
  ctx->stack[0].obj_idx = ctx->root_idx;
  ctx->stack[0].is_array = 0;
  ctx->stack[0].is_objiter = 0;
  ctx->selection_idx = ctx->root_idx;
  return MUSTACH_OK;
}

static int lua_mustache_compare(void *closure, const char *value) {
  struct lua_mustache_context *ctx = closure;
  lua_State *L = ctx->L;
  int idx = ctx->selection_idx;
  switch (lua_type(L, idx)) {
    case LUA_TNUMBER: {
      double d = lua_tonumber(L, idx) - atof(value);
      return d < 0 ? -1 : d > 0 ? 1 : 0;
    }
    case LUA_TSTRING:
      return strcmp(lua_tostring(L, idx), value);
    case LUA_TBOOLEAN:
      return strcmp(lua_toboolean(L, idx) ? "true" : "false", value);
    case LUA_TNIL:
      return strcmp("null", value);
    default:
      return 1;
  }
}

static int lua_mustache_sel(void *closure, const char *name) {
  struct lua_mustache_context *ctx = closure;
  lua_State *L = ctx->L;
  if (name == NULL) {
    ctx->selection_idx = ctx->stack[ctx->depth].obj_idx;
    return 1;
  }
  for (int i = ctx->depth; i >= 0; i--) {
    int obj_idx = ctx->stack[i].obj_idx;
    if (lua_type(L, obj_idx) == LUA_TTABLE) {
      lua_pushstring(L, name);
      lua_rawget(L, obj_idx);
      if (!lua_isnil(L, -1)) {
        ctx->selection_idx = lua_gettop(L);
        return 1;
      }
      lua_pop(L, 1);
    }
  }
  lua_pushnil(L);
  ctx->selection_idx = lua_gettop(L);
  return 0;
}

static int lua_mustache_subsel(void *closure, const char *name) {
  struct lua_mustache_context *ctx = closure;
  lua_State *L = ctx->L;
  int idx = ctx->selection_idx;
  if (lua_type(L, idx) == LUA_TTABLE) {
    lua_pushstring(L, name);
    lua_rawget(L, idx);
    if (!lua_isnil(L, -1)) {
      ctx->selection_idx = lua_gettop(L);
      return 1;
    }
    lua_pop(L, 1);
  }
  return 0;
}

static int lua_mustache_enter(void *closure, int objiter) {
  struct lua_mustache_context *ctx = closure;
  lua_State *L = ctx->L;
  if (++ctx->depth >= MUSTACH_MAX_DEPTH)
    return MUSTACH_ERROR_TOO_DEEP;
  int sel_idx = ctx->selection_idx;
  int type = lua_type(L, sel_idx);
  ctx->stack[ctx->depth].is_objiter = 0;
  ctx->stack[ctx->depth].is_array = 0;
  ctx->stack[ctx->depth].cont_idx = 0;
  if (objiter) {
    if (type != LUA_TTABLE) goto not_entering;
    lua_pushnil(L);
    if (!lua_next(L, sel_idx)) {
      lua_pop(L, 1);
      goto not_entering;
    }
    ctx->stack[ctx->depth].cont_idx = sel_idx;
    ctx->stack[ctx->depth].is_objiter = 1;
    ctx->stack[ctx->depth].key_idx = lua_gettop(L) - 1;
    ctx->stack[ctx->depth].obj_idx = lua_gettop(L);
    return 1;
  }
  if (type == LUA_TTABLE) {
    int len = table_array_len(L, sel_idx);
    if (len > 0) {
      lua_rawgeti(L, sel_idx, 1);
      ctx->stack[ctx->depth].cont_idx = sel_idx;
      ctx->stack[ctx->depth].obj_idx = lua_gettop(L);
      ctx->stack[ctx->depth].is_array = 1;
      ctx->stack[ctx->depth].array_len = len;
      ctx->stack[ctx->depth].iter_idx = 1;
      return 1;
    }
    lua_pushnil(L);
    int has = lua_next(L, sel_idx);
    if (has) lua_pop(L, 2);
    if (!has) goto not_entering;
    ctx->stack[ctx->depth].obj_idx = sel_idx;
    return 1;
  }
  if (is_truthy(L, sel_idx)) {
    ctx->stack[ctx->depth].obj_idx = sel_idx;
    return 1;
  }
not_entering:
  ctx->depth--;
  return 0;
}

static int lua_mustache_next(void *closure) {
  struct lua_mustache_context *ctx = closure;
  lua_State *L = ctx->L;
  if (ctx->depth <= 0)
    return MUSTACH_ERROR_CLOSING;
  if (ctx->stack[ctx->depth].is_array) {
    int iter = ++ctx->stack[ctx->depth].iter_idx;
    if (iter > ctx->stack[ctx->depth].array_len)
      return 0;
    lua_rawgeti(L, ctx->stack[ctx->depth].cont_idx, iter);
    ctx->stack[ctx->depth].obj_idx = lua_gettop(L);
    return 1;
  }
  if (ctx->stack[ctx->depth].is_objiter) {
    if (!lua_next(L, ctx->stack[ctx->depth].cont_idx))
      return 0;
    ctx->stack[ctx->depth].key_idx = lua_gettop(L) - 1;
    ctx->stack[ctx->depth].obj_idx = lua_gettop(L);
    return 1;
  }
  return 0;
}

static int lua_mustache_leave(void *closure) {
  struct lua_mustache_context *ctx = closure;
  if (ctx->depth <= 0)
    return MUSTACH_ERROR_CLOSING;
  ctx->depth--;
  return MUSTACH_OK;
}

static int lua_mustache_get(void *closure, struct mustach_sbuf *sbuf, int key) {
  struct lua_mustache_context *ctx = closure;
  lua_State *L = ctx->L;
  if (key) {
    sbuf->value = "";
    for (int d = ctx->depth; d >= 0; d--) {
      if (ctx->stack[d].is_objiter) {
        int key_idx = ctx->stack[d].key_idx;
        if (lua_type(L, key_idx) == LUA_TSTRING) {
          sbuf->value = lua_tostring(L, key_idx);
        } else if (lua_type(L, key_idx) == LUA_TNUMBER) {
          snprintf(ctx->keybuf, sizeof(ctx->keybuf), "%.14g", lua_tonumber(L, key_idx));
          sbuf->value = ctx->keybuf;
        }
        break;
      }
    }
    sbuf->length = 0;
    return 1;
  }
  int idx = ctx->selection_idx;
  switch (lua_type(L, idx)) {
    case LUA_TSTRING:
      sbuf->value = lua_tostring(L, idx);
      sbuf->length = 0;
      return 1;
    case LUA_TNUMBER:
      snprintf(ctx->numbuf, sizeof(ctx->numbuf), "%.14g", lua_tonumber(L, idx));
      sbuf->value = ctx->numbuf;
      sbuf->length = 0;
      return 1;
    case LUA_TBOOLEAN:
      sbuf->value = lua_toboolean(L, idx) ? "true" : "false";
      sbuf->length = 0;
      return 1;
    case LUA_TNIL:
      sbuf->value = "";
      sbuf->length = 0;
      return 1;
    default:
      sbuf->value = "";
      sbuf->length = 0;
      return 1;
  }
}

static struct lua_mustache_context *current_partial_context = NULL;

static int lua_mustache_get_partial_hook(const char *name, struct mustach_sbuf *sbuf) {
  if (!current_partial_context || current_partial_context->partials_idx == 0) {
    return MUSTACH_ERROR_PARTIAL_NOT_FOUND;
  }

  lua_State *L = current_partial_context->L;
  lua_pushstring(L, name);
  lua_rawget(L, current_partial_context->partials_idx);

  if (lua_type(L, -1) == LUA_TSTRING) {
    size_t len;
    const char *partial = lua_tolstring(L, -1, &len);
    sbuf->value = partial;
    sbuf->length = len;
    lua_pop(L, 1);
    return MUSTACH_OK;
  }

  // Handle compiled template functions - extract the dedented template from upvalue
  if (lua_type(L, -1) == LUA_TFUNCTION) {
    const char *upvalue = lua_getupvalue(L, -1, 1);
    if (upvalue != NULL && lua_type(L, -1) == LUA_TSTRING) {
      size_t len;
      const char *partial = lua_tolstring(L, -1, &len);
      sbuf->value = partial;
      sbuf->length = len;
      lua_pop(L, 2);
      return MUSTACH_OK;
    }
    if (upvalue != NULL) {
      lua_pop(L, 1);
    }
  }

  lua_pop(L, 1);
  return MUSTACH_ERROR_PARTIAL_NOT_FOUND;
}

static const struct mustach_wrap_itf lua_mustache_itf = {
  .start = lua_mustache_start,
  .stop = NULL,
  .compare = lua_mustache_compare,
  .sel = lua_mustache_sel,
  .subsel = lua_mustache_subsel,
  .enter = lua_mustache_enter,
  .next = lua_mustache_next,
  .leave = lua_mustache_leave,
  .get = lua_mustache_get
};

static char* dedent_template(lua_State *L, const char *tstr, size_t tlen, size_t *out_len) {
  if (tlen == 0) {
    *out_len = 0;
    return NULL;
  }

  size_t min_indent = (size_t)-1;
  const char *line_start = tstr;
  const char *end = tstr + tlen;

  for (const char *p = tstr; p < end; p++) {
    if (*p == '\n' || p == end - 1) {
      const char *line_end = (*p == '\n') ? p : p + 1;
      size_t indent = 0;
      const char *first_nonws = line_start;

      while (first_nonws < line_end && (*first_nonws == ' ' || *first_nonws == '\t')) {
        indent++;
        first_nonws++;
      }

      if (first_nonws < line_end && *first_nonws != '\n') {
        if (indent < min_indent) {
          min_indent = indent;
        }
      }

      line_start = p + 1;
    }
  }

  if (min_indent == (size_t)-1 || min_indent == 0) {
    *out_len = tlen;
    return NULL;
  }

  char *result = malloc(tlen);
  if (!result) {
    *out_len = 0;
    return NULL;
  }

  char *dst = result;
  line_start = tstr;

  for (const char *p = tstr; p < end; p++) {
    if (*p == '\n' || p == end - 1) {
      const char *line_end = (*p == '\n') ? p : p + 1;
      const char *src = line_start;
      size_t skipped = 0;

      while (src < line_end && skipped < min_indent && (*src == ' ' || *src == '\t')) {
        src++;
        skipped++;
      }

      while (src < line_end) {
        *dst++ = *src++;
      }

      if (*p == '\n') {
        *dst++ = '\n';
      }

      line_start = p + 1;
    }
  }

  *out_len = dst - result;
  return result;
}

static int tk_mustache_render(lua_State *L) {
  lua_settop(L, 1);
  size_t tlen;
  const char *tstr = luaL_checklstring(L, lua_upvalueindex(1), &tlen);
  char *result = NULL;
  size_t size = 0;
  int rc;

  int partials_idx = 0;
  if (!lua_isnil(L, lua_upvalueindex(2))) {
    lua_pushvalue(L, lua_upvalueindex(2));
    partials_idx = lua_gettop(L);
  }

  int (*old_hook)(const char*, struct mustach_sbuf*) = mustach_wrap_get_partial;
  struct lua_mustache_context hook_ctx;
  hook_ctx.L = L;
  hook_ctx.partials_idx = partials_idx;
  current_partial_context = &hook_ctx;
  mustach_wrap_get_partial = lua_mustache_get_partial_hook;

  if (lua_type(L, 1) == LUA_TTABLE) {
    struct lua_mustache_context ctx;
    ctx.L = L;
    ctx.root_idx = 1;
    ctx.partials_idx = partials_idx;
    current_partial_context = &ctx;
    rc = mustach_wrap_mem(tstr, tlen, &lua_mustache_itf, &ctx, Mustach_With_AllExtensions, &result, &size);
  } else {
    size_t jlen;
    const char *jstr = tk_lua_checklstring(L, 1, &jlen, "json string");
    cJSON *root = cJSON_Parse(jstr);
    if (!root) {
      mustach_wrap_get_partial = old_hook;
      current_partial_context = NULL;
      const char *err = cJSON_GetErrorPtr();
      tk_lua_verror(L, 3, "mustache render", "json parse failed", err ? err : "unknown error");
      return 0;
    }
    fflush(stderr);
    rc = mustach_cJSON_mem(tstr, tlen, root, Mustach_With_AllExtensions, &result, &size);
    cJSON_Delete(root);
  }

  mustach_wrap_get_partial = old_hook;
  current_partial_context = NULL;

  if (rc != 0) {
    tk_lua_verror(L, 3, "mustache render", "render failed", "error code: %d", rc);
    return 0;
  }
  lua_pushlstring(L, result, size);
  free(result);
  return 1;
}

static int tk_mustache_compile(lua_State *L) {
  int nargs = lua_gettop(L);
  size_t tlen;
  const char *tstr;
  char *dedented = NULL;
  int do_dedent = 1;
  int short_circuit = 0;
  int partials_idx = 0;
  int options_idx = 0;

  if (nargs >= 2) {
    int arg2_type = lua_type(L, 2);
    if (arg2_type == LUA_TTABLE) {
      lua_getfield(L, 2, "partials");
      if (!lua_isnil(L, -1)) {
        short_circuit = 0;
        options_idx = 2;
      } else {
        lua_pop(L, 1);
        short_circuit = 1;
      }
    } else if (arg2_type == LUA_TSTRING) {
      short_circuit = 1;
    } else if (arg2_type == LUA_TBOOLEAN) {
      do_dedent = lua_toboolean(L, 2);
    }
  }

  if (options_idx > 0) {
    lua_getfield(L, options_idx, "dedent");
    if (lua_isboolean(L, -1)) {
      do_dedent = lua_toboolean(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, options_idx, "partials");
    if (lua_type(L, -1) == LUA_TTABLE) {
      partials_idx = lua_gettop(L);
    } else {
      lua_pop(L, 1);
    }
  }

  if (short_circuit && nargs >= 3) {
    int arg3_type = lua_type(L, 3);
    if (arg3_type == LUA_TBOOLEAN) {
      do_dedent = lua_toboolean(L, 3);
    } else if (arg3_type == LUA_TTABLE) {
      lua_getfield(L, 3, "dedent");
      if (lua_isboolean(L, -1)) {
        do_dedent = lua_toboolean(L, -1);
      }
      lua_pop(L, 1);

      lua_getfield(L, 3, "partials");
      if (lua_type(L, -1) == LUA_TTABLE) {
        partials_idx = lua_gettop(L);
      } else {
        lua_pop(L, 1);
      }
    }
  }

  tstr = luaL_checklstring(L, 1, &tlen);

  if (do_dedent) {
    size_t dedented_len;
    dedented = dedent_template(L, tstr, tlen, &dedented_len);
    if (dedented) {
      tstr = dedented;
      tlen = dedented_len;
    }
    if (tlen > 0 && tstr[0] == '\n') {
      tstr++;
      tlen--;
    }
  }

  if (short_circuit) {
    char *result = NULL;
    size_t size = 0;
    int rc;

    int (*old_hook)(const char*, struct mustach_sbuf*) = mustach_wrap_get_partial;
    struct lua_mustache_context hook_ctx;
    hook_ctx.L = L;
    hook_ctx.partials_idx = partials_idx;
    current_partial_context = &hook_ctx;
    mustach_wrap_get_partial = lua_mustache_get_partial_hook;

    if (lua_type(L, 2) == LUA_TTABLE) {
      struct lua_mustache_context ctx;
      ctx.L = L;
      ctx.root_idx = 2;
      ctx.partials_idx = partials_idx;
      current_partial_context = &ctx;
      rc = mustach_wrap_mem(tstr, tlen, &lua_mustache_itf, &ctx, Mustach_With_AllExtensions, &result, &size);
    } else {
      size_t jlen;
      const char *jstr = tk_lua_checklstring(L, 2, &jlen, "json string");
      cJSON *root = cJSON_Parse(jstr);
      if (!root) {
        mustach_wrap_get_partial = old_hook;
        current_partial_context = NULL;
        if (dedented) free(dedented);
        const char *err = cJSON_GetErrorPtr();
        tk_lua_verror(L, 3, "mustache render", "json parse failed", err ? err : "unknown error");
        return 0;
      }
      fflush(stderr);
      rc = mustach_cJSON_mem(tstr, tlen, root, Mustach_With_AllExtensions, &result, &size);
      cJSON_Delete(root);
    }

    mustach_wrap_get_partial = old_hook;
    current_partial_context = NULL;

    if (dedented) free(dedented);
    if (rc != 0) {
      tk_lua_verror(L, 3, "mustache render", "render failed", "error code: %d", rc);
      return 0;
    }
    lua_pushlstring(L, result, size);
    free(result);
    return 1;
  }

  if (dedented) {
    lua_pushlstring(L, dedented, tlen);
    free(dedented);
  } else {
    lua_pushvalue(L, 1);
  }

  if (partials_idx == 0) {
    lua_pushnil(L);
  } else {
    lua_pushvalue(L, partials_idx);
  }

  lua_pushcclosure(L, tk_mustache_render, 2);
  return 1;
}

int luaopen_santoku_mustache(lua_State *L) {
  lua_pushcfunction(L, tk_mustache_compile);
  return 1;
}
