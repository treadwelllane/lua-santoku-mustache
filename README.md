<p align="center">
  <img src="https://santoku.dev/logo-santoku-mustache.png" height="64" alt="santoku-mustache">
</p>

# santoku-mustache

Renders Mustache templates against Lua values. A binding over the
[mustach](https://gitlab.com/jobol/mustach) C library with all extensions enabled:
compile a template once, then render it many times.

## Install

```sh
luarocks install santoku-mustache
```

## Example

```lua
local mustache = require("santoku.mustache")

local render = mustache("{{greeting}} {{target}}")

print(render({ greeting = "hello", target = "world" }))
```

## Documentation

Runnable examples and the full API: [santoku.dev](https://santoku.dev/#santoku-mustache).

For agents and LLM tooling: [llms.txt](https://santoku.dev/llms.txt) for the index,
[llms-full.txt](https://santoku.dev/llms-full.txt) for every documented example.

## Tests

The tests are the spec. For the exhaustive case list, read them:
[`test/spec/santoku/mustache.lua`](test/spec/santoku/mustache.lua). For Mustache
template syntax itself, see the [mustach](https://gitlab.com/jobol/mustach)
documentation.

## License

MIT, see [LICENSE](LICENSE).

## More examples

```lua
local test = require("santoku.test")

local err = require("santoku.error")
local assert = err.assert

local validate = require("santoku.validate")
local eq = validate.isequal

local mustache = require("santoku.mustache")

test("compile once, render many", function ()
  local render = mustache("{{greeting}} {{target}}")
  assert(eq("hello world", render({ greeting = "hello", target = "world" })))
  assert(eq("goodbye world", render({ greeting = "goodbye", target = "world" })))
end)

test("reach into nested tables with dot notation", function ()
  assert(eq("value", mustache("{{a.b.c}}")({ a = { b = { c = "value" } } })))
  assert(eq("", mustache("{{a.b.c}}")({ a = { b = {} } })))
end)

test("sections render on truthy values and omit on falsy", function ()
  assert(eq("yes", mustache("{{#show}}yes{{/show}}")({ show = true })))
  assert(eq("", mustache("{{#show}}yes{{/show}}")({ show = false })))
  assert(eq("no", mustache("{{^show}}no{{/show}}")({ show = false })))
end)

test("iterate an array, with . as the current item", function ()
  assert(eq("123", mustache("{{#items}}{{.}}{{/items}}")({ items = { 1, 2, 3 } })))
end)
```
