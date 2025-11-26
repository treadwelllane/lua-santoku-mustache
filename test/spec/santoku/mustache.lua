local test = require("santoku.test")
local mch = require("santoku.mustache")
local cjson = require("cjson")

test("mustache", function()

  local tpl = mch([[
    <!doctype html>
    <body>
      <div>
        {{#items}}
        <li>{{.}}</li>
        {{/items}}
      </div>
    </body>
  ]])

  local t = { items = { 1, 2, 3 } }
  local jstr = cjson.encode(t)
  print(tpl(jstr))
  print(tpl(t))

end)
