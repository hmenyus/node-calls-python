{
    "targets": [
        {
            "target_name": "nodecallspython",
            "conditions": [
                ['OS=="win"', {
                    "include_dirs" : [ 
                        "<!(node ./scripts/pysearch.js include)"
                    ],
                    "defines": [
                        "_HAS_EXCEPTIONS=1"
                    ],
                    "msvs_settings": {
                        "VCCLCompilerTool": {
                            "ExceptionHandling": 1
                        }
                    },
                    "link_settings": {
                        "library_dirs": [
                            "<!(node ./scripts/pysearch.js libs)"
                        ],
                        "libraries": [
                            "<!(node ./scripts/pysearch.js lib)"
                        ],
                    }
                }],
                ['OS=="linux"', {
                    "include_dirs" : [
                        "<!(python3-config --includes)"
                    ],
                    "link_settings": {
                        "libraries": [
                            "<!(node ./scripts/pylink.js)",
                            "<!(node ./scripts/rpaths.js)"
                        ]
                    },
                    'cflags': ["<!(python3-config --cflags)", "-fexceptions"],
                    'cflags_cc': ["<!(python3-config --cflags)", "-fexceptions"]
                }],
                ['OS=="mac"', {
                    "include_dirs" : [
                        "<!(python3-config --includes)"
                    ],
                    "link_settings": {
                        "libraries": [
                            "<!(node ./scripts/pylink.js)"
                            "<!(node ./scripts/rpaths.js)"
                        ]
                    },
                    'xcode_settings': {
                        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
                        'CLANG_CXX_LIBRARY': 'libc++'
                    }
                }]
            ],
            "sources": [
                "src/addon.cpp",
                "src/pyinterpreter.cpp"
            ]
        }
    ]
}