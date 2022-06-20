package com.example.videostreamer

import android.util.Log
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData

class MediaStreamer {
    enum class State(val value: Int) {
        INIT(0), READY(1), STARTED(2), PAUSED(3), STOPPED(4);

        companion object {
            fun fromInt(value: Int) = values().first { it.value == value }
        }
    }

    companion object {
        private const val TAG = "MediaStreamer";
    }

    var mUrl: String? = null
    var mOutUrl : String? = null

    private val mDurationMs: MutableLiveData<Long> = MutableLiveData(0)
    private val mCurrPosMs: MutableLiveData<Long> = MutableLiveData(0)
    private val mPlayerState: MutableLiveData<State> = MutableLiveData(State.INIT)

    fun create(){
        create(mUrl!!, mOutUrl!!)
    }

    private external fun create(url: String, outUrl: String)

    external fun start()

    external fun pause()

    external fun resume()

    external fun stop()

    external fun clean()
}