package com.example.videostreamer

class GLES3JNILib {
    companion object {
        external fun init()
        external fun resize(width: Int, height: Int)
        external fun draw()
    }
}