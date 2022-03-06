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
                        "library_dirs": [
                            "<!(python3-config --ldflags --embed)",
                        ],
                        "libraries": [
                            "<!(python3-config --libs --embed)"
                        ]
                    },
                    'cflags': ['-fexceptions'],
                    'cflags_cc': ['-fexceptions']
                }],
                ['OS=="mac"', {
                    "include_dirs" : [
                        "<!(python3-config --includes)"
                    ],
                    "link_settings": {
                        "libraries": [
                            "<!(python3-config --ldflags --embed)"
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