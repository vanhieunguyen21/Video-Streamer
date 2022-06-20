package com.example.videostreamer

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import androidx.lifecycle.lifecycleScope
import com.example.videostreamer.databinding.ActivityGlesBinding
import kotlinx.coroutines.launch

class GLESActivity : AppCompatActivity() {
    private lateinit var glSurfaceView: GLES3JNIView
    private lateinit var binding: ActivityGlesBinding
    private lateinit var mediaStreamer: MediaStreamer

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityGlesBinding.inflate(layoutInflater)
        setContentView(binding.root)

        glSurfaceView = binding.glesView

        mediaStreamer = MediaStreamer()
        mediaStreamer.mUrl = "${getExternalFilesDir(null)}/vid.mp4"
        mediaStreamer.mOutUrl = "${getExternalFilesDir(null)}/out.mp4"
//        mediaPlayer.mUrl = "rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mp4"
//        mediaPlayer.mUrl = "rtsp://rtsp.stream/pattern"

        lifecycleScope.launch {
            mediaStreamer.create()
            mediaStreamer.start()
        }
    }

    override fun onPause() {
        super.onPause()
        glSurfaceView.onPause()
        mediaStreamer.pause()
    }

    override fun onResume() {
        super.onResume()
        glSurfaceView.onResume()
        mediaStreamer.resume()
    }
}