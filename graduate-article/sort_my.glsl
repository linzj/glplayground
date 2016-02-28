#version 150

#if 1
void  sort(in float depths[8], out int[8]ret) {
    for (int i = 0;i != 8;++i) {
        ret[i] = i;
    }
    int ixj, i;
    //compare times :4+6+7=17 less than 24

    for (i = 0;i <= 3;i++) {
        ixj = i + 4;
        if (depths[i] > depths[ixj]) {
            float tmp_depth;
            int tmp_index;
            tmp_depth = depths[i];
            tmp_index = ret[i];

            depths[i] = depths[ixj];
            depths[ixj] = tmp_depth;

            ret[i] = ret[ixj];
            ret[ixj] = tmp_index;
        }
    }

    for (i = 0;i <= 5;i++) {
        ixj = i + 2;
        if (depths[i] > depths[ixj]) {
            float tmp_depth;
            int tmp_index;
            tmp_depth = depths[i];
            tmp_index = ret[i];

            depths[i] = depths[ixj];
            depths[ixj] = tmp_depth;

            ret[i] = ret[ixj];
            ret[ixj] = tmp_index;
        }
    }

    for (i = 0;i < 7 ;i++) {
        ixj = i + 1;
        if (depths[i] > depths[ixj]) {
            float tmp_depth;
            int tmp_index;
            tmp_depth = depths[i];
            tmp_index = ret[i];

            depths[i] = depths[ixj];
            depths[ixj] = tmp_depth;

            ret[i] = ret[ixj];
            ret[ixj] = tmp_index;
        }
    }

}

#else

void sort (in float depths[8], out int[8] ret) {
    int i, j, k;
    for (i = 0;i != 8;++i) {
        ret[i] = i;
    }

    for (k = 2;k <= 8;k = 2 * k) {
        for (j = k >> 1;j > 0;j = j >> 1) {
            for (i = 0;i < 8;i++) {
                int ixj = i ^ j;
                if ((ixj) > i) {
                    if ((i&k) == 0 ) {
                        if (depths[i] > depths[ixj]) {
                            float tmp_depth;
                            int tmp_index;
                            tmp_depth = depths[i];
                            tmp_index = ret[i];

                            depths[i] = depths[ixj];
                            depths[ixj] = tmp_depth;

                            ret[i] = ret[ixj];
                            ret[ixj] = tmp_index;
                        }
                    } else if (depths[i] < depths[ixj]) {
                        float tmp_depth;
                        int tmp_index;
                        tmp_depth = depths[i];
                        tmp_index = ret[i];

                        depths[i] = depths[ixj];
                        depths[ixj] = tmp_depth;

                        ret[i] = ret[ixj];
                        ret[ixj] = tmp_index;

                    }
                }

            }

        }

    }
}

#endif
