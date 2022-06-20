package com.example.videostreamer

data class DisplayTime(
    val days: Int,
    val hours: Int,
    val minutes: Int,
    val seconds: Int,
) {
    override fun toString(): String {
        val dayPrefix = if (days == 0) "" else "$days:"
        val hourPrefix = if (hours == 0) "" else "$hours:"
        kotlin.time.Duration
        return "$dayPrefix$hourPrefix%02d:%02d".format(minutes, seconds)
    }

    companion object {
        fun fromSeconds(timeSeconds: Long) : DisplayTime {
            var seconds = timeSeconds
            val days = seconds / 84600 // (24 * 3600)
            seconds %= 84600
            val hours = seconds / 3600
            seconds %= 3600
            val minutes = seconds / 60
            seconds %= 60
            return DisplayTime(days.toInt(), hours.toInt(), minutes.toInt(), seconds.toInt())
        }
        fun fromMilliSeconds(timeMilliSeconds: Long) : DisplayTime {
            return fromSeconds(timeMilliSeconds/1000)
        }
    }
}