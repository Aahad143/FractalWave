using System;
// using System.IO.MemoryMappedFiles;
using System.Threading;
using UnityEngine;

// Script for implementing shared memory communication with Qt for sending rendering flag.
public class RenderFlag : MonoBehaviour
{
    private const string EventName = "Local\\FractalWaveUnityReady";

    private EventWaitHandle unityReadyEvent;

    void Awake()
    {
        // keep ticking even if window is hidden or in the background
        Application.runInBackground = true;
    }

    // Start is called once before the first execution of Update after the MonoBehaviour is created
    void Start()
    {
        try
        {
            // Create the Windows event used to signal that Unity is ready.
            unityReadyEvent = new EventWaitHandle(false, EventResetMode.AutoReset, EventName);

            // Signal the event so that the Qt application can detect that Unity is ready.
            unityReadyEvent.Set();
        }

        catch (Exception ex)
        {
            Debug.LogError("Error initializing shared memory: " + ex.Message);
        }
    }

    // Update is called once per frame
    void Update()
    {

    }

    void OnDestroy()
    {
        unityReadyEvent?.Close();
    }
}
