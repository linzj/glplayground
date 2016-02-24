{
    'includes': [
        'squish.gypi',
    ],
    'targets': [
        {
            'target_name': 'squish',
            'type': 'static_library',
            'cflags': [
                '<@(mycflags)',
            ], 
            'sources': [
                '<@(mysources)',
            ],
            'include_dirs': [
                '.',
            ],
            'defines': [
                'SQUISH_USE_SSE=2',
            ],
            'direct_dependent_settings': {
                'include_dirs': [
                    '.',
                ],
            },
        },
    ],
}
