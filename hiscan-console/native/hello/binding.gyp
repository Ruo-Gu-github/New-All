{
  "targets": [
    {
      "target_name": "hello",
      "sources": [ "hello.cc" ],
      "include_dirs": [
        "node_modules/node-addon-api"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "defines": [
        "NAPI_DISABLE_CPP_EXCEPTIONS"
      ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "cflags!": [ "-fno-exceptions" ],
      "msvs_disabled_warnings": [ "4244", "4267" ]
    }
  ]
}
