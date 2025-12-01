# santoku.mustache

Mustache template renderer with all extensions enabled.

## API

```lua
local mustache = require("santoku.mustache")
```

The module returns a **single function** that compiles templates:

```lua
render = mustache(template) -- dedent=true
render = mustache(template, { dedent = false }) -- dedent=false
render = mustache(template, { partials = { ... } }) -- with partials
result = render(data)
```

## Parameters

### Compile: `mustache(template [, opts])`

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `template` | `string` | required | Mustache template |
| `opts.dedent` | `boolean` | `true` | Strip leading whitespace |
| `opts.partials` | `table` | `nil` | Partial templates (string or compiled) |

### Render: `render(data)`

| Parameter | Type | Description |
|-----------|------|-------------|
| `data` | `table` or `string` | Context (Lua table or JSON string) |

## Features

- **Auto-dedent**: Removes shared leading whitespace + leading/trailing `\n`
- **Partials**: In-memory only (no file system access)
- **Extensions**: Equal/compare, JSON pointer, object iteration, colon substitution
- **Fast path**: Zero-copy for Lua tables
- **Fallback**: JSON string parsing via cJSON
- **Max depth**: 256 levels of nesting

## Examples

### Basic Usage

```lua
mustache("Hello {{name}}")({name = "World"})
--> "Hello World"
```

### With Dedent

```lua
mustache([[
  <div>
    {{content}}
  </div>
]])({content = "Hi"})
--> "<div>\n  Hi\n</div>"
```

### Partials

```lua
mustache("{{> header}}\n{{body}}", {
  partials = {header = "<h1>Title</h1>"}
})({body = "text"})
--> "<h1>Title</h1>\ntext"
```

### Compile + Reuse

```lua
local render = mustache("Hi {{name}}")
render({name = "Alice"})  --> "Hi Alice"
render({name = "Bob"})    --> "Hi Bob"
```

### Sections (Arrays)

```lua
mustache([[
  {{#users}}
    - {{name}}
  {{/users}}
]])({
  users = {
    {name = "Alice"},
    {name = "Bob"}
  }
})
--> "  - Alice\n  - Bob"
```

### Conditionals

```lua
mustache("{{#show}}Visible{{/show}}{{^show}}Hidden{{/show}}")({show = true})
--> "Visible"

mustache("{{#show}}Visible{{/show}}{{^show}}Hidden{{/show}}")({show = false})
--> "Hidden"
```

### Object Iteration (Extension)

```lua
mustache([[
  {{#data.*}}
    {{*}}: {{.}}
  {{/data.*}}
]])({
  data = {name = "Alice", age = 30}
})
--> "  name: Alice\n  age: 30"
```

### Comparisons (Extension)

```lua
mustache("{{#age>18}}Adult{{/age>18}}")({age = 25})
--> "Adult"

mustache("{{#price<=100}}Affordable{{/price<=100}}")({price = 50})
--> "Affordable"
```

## Type Conversion

| Lua Type | Mustache Behavior |
|----------|-------------------|
| `nil` | Empty string `""` |
| `boolean` | `"true"` or `"false"` |
| `number` | Formatted with `"%.14g"` |
| `string` | As-is |
| `table` | Array (iterate) or Object (truthy test) |

### Array Detection

Tables are treated as arrays when they have:
- Consecutive integer keys starting at 1
- No holes (no `nil` values)
- No string/non-sequential keys

Otherwise, they're treated as objects.

### Truthiness

- **Falsy**: `nil`, `false`, `0`, `""` (empty string), `{}` (empty table)
- **Truthy**: All other values

## Mustache Tags

| Tag | Description | Example |
|-----|-------------|---------|
| `{{name}}` | Variable (escaped) | `{{title}}` |
| `{{{name}}}` | Unescaped variable | `{{{html}}}` |
| `{{#section}}` | Section (loop/conditional) | `{{#users}}...{{/users}}` |
| `{{^inverted}}` | Inverted section | `{{^logged_in}}...{{/logged_in}}` |
| `{{! comment}}` | Comment (ignored) | `{{! TODO: fix }}` |
| `{{> partial}}` | Include partial | `{{> header}}` |
| `{{.}}` | Current context | `{{#items}}{{.}}{{/items}}` |
| `{{=<% %>=}}` | Change delimiters | `{{=<% %>=}}` |

## Extensions Enabled

All mustach extensions are enabled by default (`Mustach_With_AllExtensions`):

- **Equality testing**: `{{#key=value}}` or `{{#key=!value}}`
- **Comparisons**: `{{#key>value}}`, `{{#key>=value}}`, `{{#key<value}}`, `{{#key<=value}}`
- **JSON Pointer**: `{{/path/to/value}}`
- **Object iteration**: `{{#obj.*}}{{*}}:{{.}}{{/obj.*}}`
- **Colon substitution**: `{{:#key}}` for keys starting with reserved chars
- **Empty tags**: `{{}}` allowed
- **Escape first compare**: Auto-escape comparison operators at start

## License

Copyright 2025 Matthew Brooks

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the “Software”), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
