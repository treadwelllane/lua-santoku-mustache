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

  print(tpl(cjson.encode({ items = { 1, 2, 3 } })))
  print(tpl({ items = { 1, 2, 3 } }))

end)
