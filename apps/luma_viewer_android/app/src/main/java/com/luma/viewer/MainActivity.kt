// LUMA Viewer Android - Main Activity
// Vulkan-based 3D model viewer with touch gesture controls

package com.luma.viewer

import android.os.Bundle
import android.view.GestureDetector
import android.view.MotionEvent
import android.view.ScaleGestureDetector
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import kotlin.math.abs

class MainActivity : AppCompatActivity(), SurfaceHolder.Callback {

    companion object {
        init {
            System.loadLibrary("luma_viewer")
        }
    }

    // Native methods
    private external fun nativeInit(surface: Surface, width: Int, height: Int): Long
    private external fun nativeDestroy(handle: Long)
    private external fun nativeResize(handle: Long, width: Int, height: Int)
    private external fun nativeRender(handle: Long)
    private external fun nativeOrbit(handle: Long, deltaX: Float, deltaY: Float)
    private external fun nativePan(handle: Long, deltaX: Float, deltaY: Float)
    private external fun nativeZoom(handle: Long, scaleFactor: Float)
    private external fun nativeResetCamera(handle: Long)
    private external fun nativeLoadModel(handle: Long, path: String): Boolean
    private external fun nativeToggleGrid(handle: Long)
    private external fun nativeToggleAutoRotate(handle: Long)
    private external fun nativeGetInfo(handle: Long): String

    private var rendererHandle: Long = 0
    private lateinit var surfaceView: SurfaceView
    private lateinit var infoText: TextView
    
    private var isRendering = false
    private var renderThread: Thread? = null

    // Gesture detectors
    private lateinit var gestureDetector: GestureDetector
    private lateinit var scaleGestureDetector: ScaleGestureDetector

    // Touch state for two-finger pan
    private var lastTouchX = 0f
    private var lastTouchY = 0f
    private var lastTouchCount = 0

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Setup surface view
        surfaceView = findViewById(R.id.surfaceView)
        surfaceView.holder.addCallback(this)

        // Info text
        infoText = findViewById(R.id.infoText)

        // Setup gestures
        setupGestureDetectors()

        // Setup touch handling
        surfaceView.setOnTouchListener { _, event -> handleTouch(event) }

        // Setup toolbar buttons
        setupToolbar()
    }

    private fun setupGestureDetectors() {
        // Single finger gestures (orbit)
        gestureDetector = GestureDetector(this, object : GestureDetector.SimpleOnGestureListener() {
            override fun onScroll(
                e1: MotionEvent?,
                e2: MotionEvent,
                distanceX: Float,
                distanceY: Float
            ): Boolean {
                if (e2.pointerCount == 1) {
                    nativeOrbit(rendererHandle, -distanceX * 0.005f, -distanceY * 0.005f)
                }
                return true
            }

            override fun onDoubleTap(e: MotionEvent): Boolean {
                nativeResetCamera(rendererHandle)
                return true
            }
        })

        // Pinch zoom
        scaleGestureDetector = ScaleGestureDetector(this, object : ScaleGestureDetector.SimpleOnScaleGestureListener() {
            override fun onScale(detector: ScaleGestureDetector): Boolean {
                nativeZoom(rendererHandle, detector.scaleFactor)
                return true
            }
        })
    }

    private fun handleTouch(event: MotionEvent): Boolean {
        // Scale gesture (pinch zoom)
        scaleGestureDetector.onTouchEvent(event)
        
        // Single finger gesture
        if (event.pointerCount == 1) {
            gestureDetector.onTouchEvent(event)
        }
        
        // Two-finger pan
        if (event.pointerCount == 2) {
            val centerX = (event.getX(0) + event.getX(1)) / 2
            val centerY = (event.getY(0) + event.getY(1)) / 2
            
            when (event.actionMasked) {
                MotionEvent.ACTION_POINTER_DOWN -> {
                    lastTouchX = centerX
                    lastTouchY = centerY
                }
                MotionEvent.ACTION_MOVE -> {
                    if (lastTouchCount >= 2) {
                        val dx = centerX - lastTouchX
                        val dy = centerY - lastTouchY
                        nativePan(rendererHandle, dx * 0.002f, -dy * 0.002f)
                    }
                    lastTouchX = centerX
                    lastTouchY = centerY
                }
            }
        }
        
        lastTouchCount = event.pointerCount
        return true
    }

    private fun setupToolbar() {
        findViewById<View>(R.id.btnGrid)?.setOnClickListener {
            nativeToggleGrid(rendererHandle)
        }
        
        findViewById<View>(R.id.btnRotate)?.setOnClickListener {
            nativeToggleAutoRotate(rendererHandle)
        }
        
        findViewById<View>(R.id.btnReset)?.setOnClickListener {
            nativeResetCamera(rendererHandle)
        }
        
        findViewById<View>(R.id.btnLoad)?.setOnClickListener {
            // TODO: Show file picker
            Toast.makeText(this, "Load model (coming soon)", Toast.LENGTH_SHORT).show()
        }
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        // Surface created, initialize renderer
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        if (rendererHandle == 0L) {
            // Initialize Vulkan renderer
            rendererHandle = nativeInit(holder.surface, width, height)
            if (rendererHandle == 0L) {
                Toast.makeText(this, "Failed to initialize Vulkan", Toast.LENGTH_LONG).show()
                return
            }
            
            // Start render loop
            startRenderLoop()
        } else {
            // Resize
            nativeResize(rendererHandle, width, height)
        }
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        stopRenderLoop()
        if (rendererHandle != 0L) {
            nativeDestroy(rendererHandle)
            rendererHandle = 0
        }
    }

    private fun startRenderLoop() {
        isRendering = true
        renderThread = Thread {
            while (isRendering) {
                if (rendererHandle != 0L) {
                    nativeRender(rendererHandle)
                    
                    // Update info text periodically
                    runOnUiThread {
                        infoText.text = nativeGetInfo(rendererHandle)
                    }
                }
                
                // Target 60 FPS
                Thread.sleep(16)
            }
        }
        renderThread?.start()
    }

    private fun stopRenderLoop() {
        isRendering = false
        renderThread?.join()
        renderThread = null
    }

    override fun onPause() {
        super.onPause()
        stopRenderLoop()
    }

    override fun onResume() {
        super.onResume()
        if (rendererHandle != 0L) {
            startRenderLoop()
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        stopRenderLoop()
        if (rendererHandle != 0L) {
            nativeDestroy(rendererHandle)
            rendererHandle = 0
        }
    }
}
