{
  "targets": [
    {
      "target_name": "console_dll_addon",
      "sources": [
        "src/addon.cpp",
        "src/dicom_wrapper.cpp",
        "src/image_processing_wrapper.cpp",
        "src/visualization_wrapper.cpp",
        "src/FatAnalysis.cpp",
        "src/VascularAnalysis.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "../../../ConsoleDllTest/Common",
        "../../../ConsoleDllTest/DllDicom",
        "../../../ConsoleDllTest/DllImageProcessing",
        "../../../ConsoleDllTest/DllVisualization"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "defines": [
        "NAPI_DISABLE_CPP_EXCEPTIONS"
      ],
      "libraries": [
        "DllDicom.lib",
        "DllCore.lib",
        "DllImageProcessing.lib",
        "DllVisualization.lib"
      ],
      "conditions": [
        ["OS=='win'", {
          "msvs_settings": {
            "VCCLCompilerTool": {
              "ExceptionHandling": 1,
              "AdditionalOptions": ["/std:c++17"]
            }
          },
          "conditions": [
            ["\"<(CONFIGURATION_NAME)\"==\"Debug\"", {
              "link_settings": {
                "library_dirs": [
                  "../../../ConsoleDllTest/Dlls/debug/lib"
                ]
              },
              "copies": [
                {
                  "destination": "<(module_root_dir)/build/Debug/",
                  "files": [
                    "../../../ConsoleDllTest/Dlls/debug/bin/DllDicom.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/DllCore.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/DllImageProcessing.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/DllAnalysisBase.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/DllBoneAnalysis.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/DllFatAnalysis.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/DllLungAnalysis.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/DllVisualization.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/gdcmCommon.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/gdcmDICT.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/gdcmDSED.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/gdcmIOD.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/gdcmMSFF.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/gdcmMEXD.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/gdcmgetopt.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/gdcmcharls.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/gdcmjpeg8.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/gdcmjpeg12.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/gdcmjpeg16.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/openjp2.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/zlibd1.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/socketxx.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/fmtd.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/libexpatd.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/glew32d.dll",
                    "../../../ConsoleDllTest/Dlls/debug/bin/glfw3.dll"
                  ]
                }
              ]
            }, {
              "link_settings": {
                "library_dirs": [
                  "../../../ConsoleDllTest/Dlls/lib"
                ]
              },
              "copies": [
                {
                  "destination": "<(module_root_dir)/build/Release/",
                  "files": [
                    "../../../ConsoleDllTest/Dlls/bin/DllDicom.dll",
                    "../../../ConsoleDllTest/Dlls/bin/DllCore.dll",
                    "../../../ConsoleDllTest/Dlls/bin/DllImageProcessing.dll",
                    "../../../ConsoleDllTest/Dlls/bin/DllAnalysisBase.dll",
                    "../../../ConsoleDllTest/Dlls/bin/DllBoneAnalysis.dll",
                    "../../../ConsoleDllTest/Dlls/bin/DllFatAnalysis.dll",
                    "../../../ConsoleDllTest/Dlls/bin/DllLungAnalysis.dll",
                    "../../../ConsoleDllTest/Dlls/bin/DllVisualization.dll",
                    "../../../ConsoleDllTest/Dlls/bin/gdcmCommon.dll",
                    "../../../ConsoleDllTest/Dlls/bin/gdcmDICT.dll",
                    "../../../ConsoleDllTest/Dlls/bin/gdcmDSED.dll",
                    "../../../ConsoleDllTest/Dlls/bin/gdcmIOD.dll",
                    "../../../ConsoleDllTest/Dlls/bin/gdcmMSFF.dll",
                    "../../../ConsoleDllTest/Dlls/bin/gdcmMEXD.dll",
                    "../../../ConsoleDllTest/Dlls/bin/gdcmgetopt.dll",
                    "../../../ConsoleDllTest/Dlls/bin/gdcmcharls.dll",
                    "../../../ConsoleDllTest/Dlls/bin/gdcmjpeg8.dll",
                    "../../../ConsoleDllTest/Dlls/bin/gdcmjpeg12.dll",
                    "../../../ConsoleDllTest/Dlls/bin/gdcmjpeg16.dll",
                    "../../../ConsoleDllTest/Dlls/bin/openjp2.dll",
                    "../../../ConsoleDllTest/Dlls/bin/zlib1.dll",
                    "../../../ConsoleDllTest/Dlls/bin/socketxx.dll",
                    "../../../ConsoleDllTest/Dlls/bin/fmt.dll",
                    "../../../ConsoleDllTest/Dlls/bin/libexpat.dll",
                    "../../../ConsoleDllTest/Dlls/bin/glew32.dll",
                    "../../../ConsoleDllTest/Dlls/bin/glfw3.dll"
                  ]
                }
              ]
            }]
          ]
        }]
      ]
    }
  ]
}
