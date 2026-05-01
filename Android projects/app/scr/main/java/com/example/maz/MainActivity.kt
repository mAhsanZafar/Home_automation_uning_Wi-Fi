package com.example.maz

import android.content.Intent
import android.os.Bundle
import android.speech.RecognitionListener
import android.speech.RecognizerIntent
import android.speech.SpeechRecognizer
import android.speech.tts.TextToSpeech
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import okhttp3.Call
import okhttp3.Callback
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.Response
import java.io.IOException
import java.util.Locale

class MainActivity : AppCompatActivity() {

    private lateinit var speechRecognizer: SpeechRecognizer
    private lateinit var textToSpeech: TextToSpeech
    private lateinit var btnStartStop: Button
    private lateinit var txtCommand: TextView
    private lateinit var txtResponse: TextView
    private lateinit var edtEspIp: EditText
    private var isListening = false
    private val client = OkHttpClient()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        btnStartStop = findViewById(R.id.btnStartStop)
        txtCommand = findViewById(R.id.txtCommand)
        txtResponse = findViewById(R.id.txtResponse)
        edtEspIp = findViewById(R.id.edtEspIp)

        speechRecognizer = SpeechRecognizer.createSpeechRecognizer(this)
        textToSpeech = TextToSpeech(this) {
            if (it == TextToSpeech.SUCCESS) {
                textToSpeech.language = Locale.US
            }
        }

        speechRecognizer.setRecognitionListener(object : RecognitionListener {
            override fun onResults(results: Bundle?) {
                val command = results?.getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION)?.get(0)
                txtCommand.text = command
                command?.let { sendCommandToESP(it) }
            }
            override fun onReadyForSpeech(params: Bundle?) {}
            override fun onBeginningOfSpeech() {}
            override fun onRmsChanged(rmsdB: Float) {}
            override fun onBufferReceived(buffer: ByteArray?) {}
            override fun onEndOfSpeech() {}
            override fun onError(error: Int) {}
            override fun onPartialResults(partialResults: Bundle?) {}
            override fun onEvent(eventType: Int, params: Bundle?) {}
        })

        btnStartStop.setOnClickListener {
            if (!isListening) {
                startListening()
            } else {
                stopListening()
            }
        }
    }

    private fun startListening() {
        val intent = Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH)
        intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_FREE_FORM)
        speechRecognizer.startListening(intent)
        isListening = true
        btnStartStop.text = "Stop"
    }

    private fun stopListening() {
        speechRecognizer.stopListening()
        isListening = false
        btnStartStop.text = "Start"
    }

    private fun sendCommandToESP(command: String) {
        val ip = edtEspIp.text.toString().trim()
        if (ip.isEmpty()) {
            txtResponse.text = "Please enter ESP8266 IP address"
            return
        }

        val url = "http://$ip/command?text=${command.replace(" ", "%20")}"
        val request = Request.Builder().url(url).build()

        client.newCall(request).enqueue(object : Callback {
            override fun onFailure(call: Call, e: IOException) {
                runOnUiThread {
                    txtResponse.text = "Error: ${e.message}"
                }
            }

            override fun onResponse(call: Call, response: Response) {
                val resText = response.body?.string()
                runOnUiThread {
                    txtResponse.text = resText
                    textToSpeech.speak(resText, TextToSpeech.QUEUE_FLUSH, null, "")
                }
            }
        })
    }
}
