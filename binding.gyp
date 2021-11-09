{
    "targets": [
        {
            "target_name": "nodecallspython",
            "conditions": [
                ['OS=="win"', {
                    "include_dirs" : [ 
                        "<!(node ./scripts/pysearch.js include)"
                    ],
                    "link_settings": {
                        "library_dirs": [
                            "<!(node ./scripts/pysearch.js libs)"
                        ],
                        "libraries": [
                            "<!(node ./scripts/pysearch.js lib)"
                        ],
                    }
                },
                {
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
                    }
                }
                ]
            ],
            "sources": [
                "src/addon.cpp",
                "src/pyinterpreter.cpp"
            ]
        }
    ]
}