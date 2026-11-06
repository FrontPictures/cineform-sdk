#include <cstdio>
#include <vector>
#include <thread>
#include "CFHDEncoder.h"

namespace {
class AsyncEncoderCompletionTest
{
public:
    static bool run(int threads, int depth, bool polling)
    {
        CFHD_EncoderPoolRef pool = NULL;
        if (CFHD_CreateEncoderPool(&pool, threads, depth, NULL) != CFHD_ERROR_OKAY) {
            return false;
        }
        std::vector<unsigned char> pixels(64 * 64 * 2, 128);
        bool success = false;
        if (CFHD_PrepareEncoderPool(pool, 64, 64, CFHD_PIXEL_FORMAT_YUY2,
                CFHD_ENCODED_FORMAT_YUV_422, CFHD_ENCODING_FLAGS_NONE,
                CFHD_ENCODING_QUALITY_HIGH) == CFHD_ERROR_OKAY &&
            CFHD_StartEncoderPool(pool) == CFHD_ERROR_OKAY) {
            success = encode(pool, depth, polling, &pixels[0]);
        }
        if (CFHD_ReleaseEncoderPool(pool) != CFHD_ERROR_OKAY) {
            success = false;
        }
        return success;
    }

private:
    static bool encode(CFHD_EncoderPoolRef pool, int depth, bool polling, unsigned char *pixels)
    {
        const uint32_t frameCount = 2000;
        uint32_t submitted = 0;
        uint32_t received = 0;
        while (received < frameCount) {
            while (submitted < frameCount && submitted - received < uint32_t(depth)) {
                if (CFHD_EncodeAsyncSample(pool, submitted, pixels, 64 * 2, NULL) != CFHD_ERROR_OKAY) {
                    return false;
                }
                ++submitted;
            }
            uint32_t frameNumber = 0;
            CFHD_SampleBufferRef sample = NULL;
            CFHD_Error error;
            if (polling) {
                error = CFHD_TestForSample(pool, &frameNumber, &sample);
                if (error == CFHD_ERROR_NOT_FINISHED) {
                    std::this_thread::yield();
                    continue;
                }
            } else {
                error = CFHD_WaitForSample(pool, &frameNumber, &sample);
            }
            if (error != CFHD_ERROR_OKAY || sample == NULL) {
                return false;
            }
            void *data = NULL;
            size_t size = 0;
            const bool valid = frameNumber == received &&
                CFHD_GetEncodedSample(sample, &data, &size) == CFHD_ERROR_OKAY &&
                data != NULL && size > 0;
            if (CFHD_ReleaseSampleBuffer(pool, sample) != CFHD_ERROR_OKAY || !valid) {
                return false;
            }
            ++received;
        }
        return true;
    }
};
}

int main()
{
    const int threadCounts[] = {1, 4};
    const int depths[] = {1, 8};
    for (int threads : threadCounts) {
        for (int depth : depths) {
            for (int polling = 0; polling < 2; ++polling) {
                if (!AsyncEncoderCompletionTest::run(threads, depth, polling != 0)) {
                    std::fprintf(stderr, "Async completion failed: threads=%d depth=%d polling=%d\n",
                        threads, depth, polling);
                    return 1;
                }
            }
        }
    }
    std::puts("Async completion passed: 16000 frames, blocking/polling, 1/4 workers, queue depths 1/8");
    return 0;
}
