package com.example.videostreamer

import android.app.Application

class MyApplication : Application() {
    init {
        System.loadLibrary("videostreamer")
    }
}