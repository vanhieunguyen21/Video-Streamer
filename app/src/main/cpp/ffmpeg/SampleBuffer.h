#ifndef SAMPLE_BUFFER_H
#define SAMPLE_BUFFER_H

#include <stdint.h>
extern "C" {
#include "libavcodec/avcodec.h"
}

class SampleBuffer {
private:
    /** Double buffer size, automatically called when trying to put data that exceeds current size. */
    void expandBuffer();
    void expandBuffer(int requiredSize);
public:
    uint8_t **mBuffer = nullptr;
    uint8_t **mTmpBuffer = nullptr;
    int mNbChannels = 0;
    AVSampleFormat mSampleFmt = AV_SAMPLE_FMT_NONE;
    // init number of samples
    int mMaxNbSamples = 1024;
    int mChunkSize = 0;
    int mCurrSampleCount = 0;
    int mCurrPos = 0;
    int mTmpNbSamples = 0;

    SampleBuffer(int nbChannels, AVSampleFormat sampleFmt, int chunkSize);
    ~SampleBuffer();

    /** Allocate memory for buffer.
     * Function will try to free the memory of the buffer if already allocated before re-allocating.
     * @return success: 1, failure: 0 */
    void freeBuffer();

    /** Check if buffer have enough samples for a chunk equals to chunk_size.
     * @return 1 if true, 0 if false */
    int available();

    /** Copy a chunk from buffer into destination sample array if a chunk is available.
     * @return 1 if success, 0 if failure or not available */
    int getChunk(uint8_t **dst, int dstOffset);

    /** Preprocess buffer and return a tmp_buffer to write into.
     * tmp_buffer will later be copied into main buffer by calling commit_data
     * @param nbSamples number of samples space the tmp_buffer will need to provide */
     uint8_t **prepareBuffer(int nbSamples);

    /** Copy data from mTmpBuffer which was processed from prepareBuffer into main buffer.
     * @param nbSamples actual number of samples will be written into buffer */
    void commit_data(int nbSamples);
};

#endif //SAMPLE_BUFFER_H