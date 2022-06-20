#include "../common/JNILogHelper.h"
#include <android/log.h>
#include "SampleBuffer.h"
#define LOG_TAG "SampleBuffer"

SampleBuffer::SampleBuffer(int channels, AVSampleFormat sampleFmt, int chunkSize) {
    mNbChannels = channels;
    mSampleFmt = sampleFmt;
    mChunkSize = chunkSize;

    av_samples_alloc_array_and_samples(&mBuffer, nullptr, mNbChannels, mMaxNbSamples, mSampleFmt, 0);
    av_samples_alloc_array_and_samples(&mTmpBuffer, nullptr, mNbChannels, mMaxNbSamples, mSampleFmt, 0);
}

SampleBuffer::~SampleBuffer() {

}

void SampleBuffer::expandBuffer() {
    expandBuffer(mMaxNbSamples * 2);
}

void SampleBuffer::expandBuffer(int requiredSize) {
    LOGV("Not enough buffer, expanding");
    while (mMaxNbSamples < requiredSize) mMaxNbSamples *= 2;
    freeBuffer();

    av_samples_alloc_array_and_samples(&mBuffer, nullptr, mNbChannels, mMaxNbSamples, mSampleFmt, 0);
    av_samples_alloc_array_and_samples(&mTmpBuffer, nullptr, mNbChannels, mMaxNbSamples, mSampleFmt, 0);
}

void SampleBuffer::freeBuffer() {
    LOGV("Freeing buffer...");
    if (mBuffer) av_freep(&mBuffer[0]);
    if (mTmpBuffer) av_freep(&mTmpBuffer[0]);
}

int SampleBuffer::available() {
    return mCurrSampleCount >= mChunkSize;
}

int SampleBuffer::getChunk(uint8_t **dst, int dstOffset) {
    if (available()) {
        av_samples_copy(dst, mBuffer, dstOffset, mCurrPos, mChunkSize, mNbChannels, mSampleFmt);
        mCurrPos += mChunkSize;
        mCurrSampleCount -= mChunkSize;
        return 1;
    }
    return 0;
}

uint8_t **SampleBuffer::prepareBuffer(int nbSamples){
    // Copy leftover data into a temp buffer
    uint8_t **tmpBuf;
    if (mCurrSampleCount > 0) {
        av_samples_alloc_array_and_samples(&tmpBuf, nullptr, mNbChannels, mCurrSampleCount, mSampleFmt, 0);
        av_samples_copy(tmpBuf, mBuffer, 0, mCurrPos, mCurrSampleCount, mNbChannels, mSampleFmt);
    }

    // Expand the buffer if current size is not enough to hold additional data
    if (mCurrSampleCount + nbSamples > mMaxNbSamples) {
        expandBuffer(mCurrSampleCount + nbSamples);
    }

    // Copy leftover data back into the buffer and free it
    if (mCurrSampleCount > 0) {
        av_samples_copy(mBuffer, tmpBuf, 0, 0, mCurrSampleCount, mNbChannels, mSampleFmt);
        av_freep(&tmpBuf[0]);
    }

    // Reset data point to start
    mCurrPos = 0;

    mTmpNbSamples = nbSamples;
    return mTmpBuffer;
}

void SampleBuffer::commit_data(int nbSamples) {
    // Put additional data in after leftover data (ends at curr_sample_count)
    av_samples_copy(mBuffer, mTmpBuffer, mCurrSampleCount, 0, nbSamples, mNbChannels, mSampleFmt);

    mCurrSampleCount += nbSamples;
}