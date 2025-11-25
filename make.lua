local env = {
  name = "santoku-mustache",
  version = "0.0.2-1",
  license = "MIT",
  public = true,
  dependencies = {
    "lua >= 5.1",
    "santoku >= 0.0.294-1",
  },
  cflags = {
    "-I$(shell luarocks show santoku --rock-dir)/include/",
    "-I$(PWD)/deps/mustach/cJSON-1.7.19/install/include",
    "-I$(PWD)/deps/mustach/mustach-1.2.10/",
  },
  ldflags = {
    "$(PWD)/deps/mustach/cJSON-1.7.19/libcjson.a",
    "$(PWD)/deps/mustach/mustach-1.2.10/build/libmustach.a"
  },
  test = {
    dependencies = {
      "lua-cjson >= 2.1.0.10-1"
    }
  }
}

env.homepage = "https://github.com/treadwelllane/lua-" .. env.name
env.tarball = env.name .. "-" .. env.version .. ".tar.gz"
env.download = env.homepage .. "/releases/download/" .. env.version .. "/" .. env.tarball

return {
  env = env,
}
