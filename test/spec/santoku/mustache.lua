local test = require("santoku.test")
local err = require("santoku.error")
local assert = err.assert
local vdt = require("santoku.validate")
local eq = vdt.isequal
local cjson = require("cjson")

local mch = require("santoku.mustache")

test("mustache", function ()

  test("variables", function ()
    assert(eq("hello world", mch("{{greeting}} {{target}}")({ greeting = "hello", target = "world" })))
    assert(eq("", mch("{{missing}}")({})))
    assert(eq("42", mch("{{num}}")({ num = 42 })))
    assert(eq("true", mch("{{flag}}")({ flag = true })))
    assert(eq("false", mch("{{flag}}")({ flag = false })))
  end)

  test("dot notation", function ()
    assert(eq("value", mch("{{a.b.c}}")({ a = { b = { c = "value" } } })))
    assert(eq("", mch("{{a.b.c}}")({ a = { b = {} } })))
  end)

  test("sections", function ()
    assert(eq("yes", mch("{{#show}}yes{{/show}}")({ show = true })))
    assert(eq("", mch("{{#show}}yes{{/show}}")({ show = false })))
    assert(eq("", mch("{{#show}}yes{{/show}}")({ show = nil })))
    assert(eq("", mch("{{#show}}yes{{/show}}")({})))
    assert(eq("yes", mch("{{#show}}yes{{/show}}")({ show = "truthy" })))
    assert(eq("yes", mch("{{#show}}yes{{/show}}")({ show = 1 })))
    assert(eq("", mch("{{#show}}yes{{/show}}")({ show = 0 })))
    assert(eq("", mch("{{#show}}yes{{/show}}")({ show = "" })))
  end)

  test("inverted sections", function ()
    assert(eq("no", mch("{{^show}}no{{/show}}")({ show = false })))
    assert(eq("no", mch("{{^show}}no{{/show}}")({ show = nil })))
    assert(eq("no", mch("{{^show}}no{{/show}}")({})))
    assert(eq("", mch("{{^show}}no{{/show}}")({ show = true })))
    assert(eq("", mch("{{^show}}no{{/show}}")({ show = "truthy" })))
  end)

  test("iteration arrays", function ()
    assert(eq("123", mch("{{#items}}{{.}}{{/items}}")({ items = { 1, 2, 3 } })))
    assert(eq("abc", mch("{{#items}}{{.}}{{/items}}")({ items = { "a", "b", "c" } })))
    assert(eq("", mch("{{#items}}{{.}}{{/items}}")({ items = {} })))
    assert(eq("a:1,b:2,", mch("{{#items}}{{name}}:{{val}},{{/items}}")({
      items = { { name = "a", val = 1 }, { name = "b", val = 2 } }
    })))
  end)

  test("nested context", function ()
    assert(eq("outer-inner", mch("{{name}}-{{#child}}{{name}}{{/child}}")({
      name = "outer",
      child = { name = "inner" }
    })))
    assert(eq("outer-", mch("{{name}}-{{#child}}{{name}}{{/child}}")({
      name = "outer",
      child = {}
    })))
  end)

  test("partials", function ()
    local item = mch("<li>{{.}}</li>")
    local list = mch("{{#items}}{{>item}}{{/items}}", { partials = { item = item } })
    assert(eq("<li>1</li><li>2</li><li>3</li>", list({ items = { 1, 2, 3 } })))
  end)

  test("partials nested", function ()
    local inner = mch("[{{val}}]")
    local outer = mch("({{>inner}})", { partials = { inner = inner } })
    assert(eq("([hello])", outer({ val = "hello" })))
  end)

  test("json input", function ()
    local tpl = mch("{{name}}: {{value}}")
    local json = cjson.encode({ name = "test", value = 42 })
    assert(eq("test: 42", tpl(json)))
  end)

  test("scalar context", function ()
    assert(eq("42", mch("{{.}}")(42)))
    assert(eq("true", mch("{{.}}")(true)))
    assert(eq("", mch("{{.}}")(nil)))
    assert(eq("yes", mch("{{#.}}yes{{/.}}")(true)))
    assert(eq("no", mch("{{^.}}no{{/.}}")(false)))
    assert(eq("no", mch("{{^.}}no{{/.}}")(nil)))
  end)

  test("dedent", function ()
    local tpl = mch([[
      hello
        world
    ]])
    assert(eq("hello\n  world\n", tpl({})))
  end)

  test("dedent disabled", function ()
    local tpl = mch("\n  hello\n    world\n", { dedent = false })
    assert(eq("\n  hello\n    world\n", tpl({})))
  end)

  test("html escaping", function ()
    assert(eq("&lt;b&gt;", mch("{{html}}")({ html = "<b>" })))
    assert(eq("<b>", mch("{{{html}}}")({ html = "<b>" })))
    assert(eq("<b>", mch("{{&html}}")({ html = "<b>" })))
  end)

  test("empty template", function ()
    assert(eq("", mch("")({})))
  end)

  test("whitespace only", function ()
    assert(eq("   ", mch("   ")({})))
  end)

  test("no variables", function ()
    assert(eq("hello world", mch("hello world")({})))
  end)

end)
