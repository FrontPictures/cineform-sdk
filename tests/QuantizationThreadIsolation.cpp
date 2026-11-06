#include <atomic>
#include <cstdio>
#include <thread>
#include "quantize.h"

namespace {
class QuantizationThreadIsolationTest
{
public:
    static bool run()
    {
        alignas(16) PIXEL input[32];
        alignas(16) PIXEL expected[2][32];
        alignas(16) PIXEL actual[2][32];
        for (int i = 0; i < 32; ++i) {
            input[i] = i * 17 - 250;
        }
        for (int index = 0; index < 2; ++index) {
            setQuality(index);
            QuantizeRow16sTo16s(input, expected[index], 32, 16);
        }
        bool different = false;
        for (int i = 0; i < 32; ++i) {
            different = different || expected[0][i] != expected[1][i];
        }
        if (!different) {
            std::fputs("Test qualities must produce different coefficients\n", stderr);
            return false;
        }

        std::atomic<int> ready(0);
        auto quantize = [&](int index) {
            setQuality(index);
            ready.fetch_add(1);
            while (ready.load() < 2) {
                std::this_thread::yield();
            }
            QuantizeRow16sTo16s(input, actual[index], 32, 16);
        };
        std::thread first(quantize, 0);
        std::thread second(quantize, 1);
        first.join();
        second.join();
        for (int index = 0; index < 2; ++index) {
            for (int i = 0; i < 32; ++i) {
                if (actual[index][i] != expected[index][i]) {
                    std::fprintf(stderr, "Concurrent quality changed coefficients: worker=%d column=%d\n", index, i);
                    return false;
                }
            }
        }
        return true;
    }

private:
    static void setQuality(int index)
    {
        QUANTIZER quantizer;
        InitQuantizer(&quantizer);
        const int quality = 3 | ((index * 4) << 17);
        QuantizationSetQuality(&quantizer, quality, true, 10, 1, false, NULL, 0, 1);
    }
};
}

int main()
{
    if (!QuantizationThreadIsolationTest::run()) {
        return 1;
    }
    std::puts("Concurrent quantization matches serial results for distinct qualities");
    return 0;
}
