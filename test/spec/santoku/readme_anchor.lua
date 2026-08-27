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
