{
    'variables': {
        'mysources': [
            'alpha.cpp',
            'clusterfit.cpp',
            'colourblock.cpp',
            'colourfit.cpp',
            'colourset.cpp',
            'maths.cpp',
            'rangefit.cpp',
            'singlecolourfit.cpp',
            'squish.cpp',
        ],
        'conditions': [
            ['arch_cpu == 32',{
                'mycflags': [
                     '-m32',
                     '-fvisibility=hidden',
                     '-g',
                     '-mincoming-stack-boundary=2',
                ],
            }],
            ['arch_cpu == 64',{
                'mycflags': [
                     '-fvisibility=hidden',
                     '-g',
                ],
            }],
        ],
    },
}
