{
    'conditions': [
        [
            "arch_cpu == 32",
            {
                'includes': [
                     '../../build/common_arch32.gypi',
                     'squish-base.gypi',
                ],
            },
        ],
        [
            "arch_cpu == 64",
            {
                'includes': [
                     '../../build/common.gypi',
                     'squish-base.gypi',
                ],
            },
        ],
    ],
}
