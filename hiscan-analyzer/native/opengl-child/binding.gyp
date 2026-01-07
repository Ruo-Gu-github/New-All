{
  "targets": [
    {
      "target_name": "opengl_child",
      "sources": [ "src/addon.cc" ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS", "UNICODE", "_UNICODE" ],
      "cflags_cc": [ "-std=c++17" ],
      "msvs_settings": {
        "VCCLCompilerTool": {
          "ExceptionHandling": 0,
          "AdditionalOptions": [ "/std:c++17", "/utf-8" ]
        }
      },
      "libraries": [ "opengl32.lib", "user32.lib", "gdi32.lib" ]
    }
  ]
}
