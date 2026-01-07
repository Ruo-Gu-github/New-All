{
  "targets": [
    {
      "target_name": "dicom_native",
      "sources": ["src/dicom_native.cpp"],
      "include_dirs_full": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "../../../vtk/VTK-9.5.2/Common/Core",
        "../../../vtk/VTK-9.5.2/Common/DataModel",
        "../../../vtk/VTK-9.5.2/Common/ExecutionModel",
        "../../../vtk/VTK-9.5.2/IO/Image",
        "../../../vtk/VTK-9.5.2/Imaging/Core",
        "../../../vtk/VTK-9.5.2/Imaging/General",
        "../../../vtk/VTK-9.5.2/Utilities/KWSys",
        "../../../vtk/VTK-9.5.2/Utilities/KWIML",
        "../../../vtk/VTK-9.5.2/build/Common/Core",
        "../../../vtk/VTK-9.5.2/build/Common/DataModel",
        "../../../vtk/VTK-9.5.2/build/Common/ExecutionModel",
        "../../../vtk/VTK-9.5.2/build/IO/Image",
        "../../../vtk/VTK-9.5.2/build/Imaging/Core",
        "../../../vtk/VTK-9.5.2/build/Imaging/General",
        "../../../vtk/VTK-9.5.2/build/Utilities/KWSys",
        "../../../vtk/VTK-9.5.2/build/Utilities/KWIML"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "../../../vtk/VTK-9.5.2/Common/Core",
        "../../../vtk/VTK-9.5.2/IO/Image",
        "../../../vtk/VTK-9.5.2/Common/ExecutionModel",
        "../../../vtk/VTK-9.5.2/build/Common/Core",
        "../../../vtk/VTK-9.5.2/build/IO/Image",
        "../../../vtk/VTK-9.5.2/build/Common/ExecutionModel"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "defines": [
        "NAPI_DISABLE_CPP_EXCEPTIONS"
      ],
      "cflags_cc": [
        "-DNAPI_DISABLE_CPP_EXCEPTIONS"
      ],
      "libraries_full": [
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkIOImage-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkIOCore-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkDICOMParser-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkImagingCore-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkImagingGeneral-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkImagingMath-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkCommonCore-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkCommonDataModel-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkCommonExecutionModel-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkCommonMath-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkCommonMisc-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkCommonSystem-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkCommonTransforms-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkmetaio-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkpng-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkjpeg-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtktiff-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkzlib-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtksys-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkdoubleconversion-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkfmt-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkloguru-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkpugixml-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtklz4-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtklzma-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtktoken-9.5.lib"
      ],
      "libraries": [
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkIOImage-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkCommonCore-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtksys-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkImagingCore-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkpng-9.5.lib",
        "D:/2025-09-25 新系列/vtk/VTK-9.5.2/build/lib/Release/vtkDICOMParser-9.5.lib"
      ],
      "msvs_settings": {
        "VCCLCompilerTool": {
          "ExceptionHandling": 1,
          "RuntimeLibrary": "MultiThreadedDLL"
        }
        ,
        "VCLinkerTool": {
          "AdditionalDependencies": ["legacy_stdio_definitions.lib;OLDNAMES.lib;%(AdditionalDependencies)"],
          "AdditionalOptions": ["/NODEFAULTLIB:LIBCMT %(AdditionalOptions)"]
        }
      }
    }
  ]
}
