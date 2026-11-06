#include <cstdio>
#include "spatial.h"

namespace {
class SpatialFilterSmallRowsTest
{
public:
    static bool run(int width)
    {
        alignas(16) PIXEL input[64];
        alignas(16) PIXEL lowpass[32];
        alignas(16) PIXEL highpass[32];
        for (int i = 0; i < 64; ++i) {
            input[i] = (i * i * 17 + i * 29) % 1000 - 500;
        }
        for (int i = 0; i < 32; ++i) {
            lowpass[i] = highpass[i] = 12345;
        }
        FilterHorizontalRow16s(input, lowpass, highpass, width);
        for (int column = 0; column < width; column += 2) {
            int high;
            if (column == 0) {
                high = (5 * input[0] - 11 * input[1] + 4 * input[2] + 4 * input[3]
                        - input[4] - input[5] + 4) >> 3;
            } else if (column == width - 2) {
                high = (11 * input[column] - 5 * input[column + 1]
                        - 4 * input[column - 1] - 4 * input[column - 2]
                        + input[column - 3] + input[column - 4] + 4) >> 3;
            } else {
                high = ((-input[column - 2] - input[column - 1]
                         + input[column + 2] + input[column + 3] + 4) >> 3)
                       + input[column] - input[column + 1];
            }
            if (lowpass[column / 2] != input[column] + input[column + 1] ||
                highpass[column / 2] != high) {
                std::fprintf(stderr, "Filter mismatch at width=%d column=%d: low=%d high=%d expectedHigh=%d\n",
                             width, column, lowpass[column / 2], highpass[column / 2], high);
                return false;
            }
        }
        return true;
    }
};
}

int main()
{
    const int widths[] = {8, 16, 32, 64};
    for (int width : widths) {
        if (!SpatialFilterSmallRowsTest::run(width)) {
            return 1;
        }
    }
    std::puts("Spatial filter passed for widths 8, 16, 32 and 64");
    return 0;
}
