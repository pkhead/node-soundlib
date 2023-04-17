{
    "targets": [
        {
            "target_name": "audiolib",
            "sources": ["binding.cpp"],
            "include_dirs": [
                "<!(node -p \"require('node-addon-api').include_dir\")",
                "libsoundio"
            ],
            "dependencies": ["<!(node -p \"require('node-addon-api').gyp\")"],
            #"libraries": ["-lasound", "-lpulse"],

            # "defines": ["NAPI_CPP_EXCEPTIONS"],
            "cflags!": ["-fno-exceptions"],
            "cflags_cc!": ["-fno-exceptions"],

            "conditions": [
                ["OS=='win'", {
                    "defines": ["_HAS_EXCEPTIONS=1"],
                    "msvs_settings": {
                        "VCCLCompilerTool": {
                            "ExceptionHandling": 1
                        }
                    }
                }],
                ["OS=='mac'", {
                    "xcode_settings": {
                        "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
                        "CLANG_CXX_LIBRARY": "libc++",
                        "MACOSX_DEPLOYMENT_TARGET": "10.7",
                    },
                }],
                ["OS=='linux'", {
                    "link_settings": {
                        "libraries": ["<@(module_root_dir)/build/Release/libsoundio.so.2.0.0"],
                        "ldflags": [
                            "-L<@(module_root_dir)/build/Release",
                            "-Wl,-rpath,<@(module_root_dir)/build/Release"
                        ],
                    },
                    "copies": [
                        {
                            "destination": "build/Release/",
                            "files": ["<@(module_root_dir)/libsoundio/build/libsoundio.so.2.0.0"],
                        },
                    ],
                }],
            ],
        }
    ]
}