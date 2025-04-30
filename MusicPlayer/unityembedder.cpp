#include "UnityEmbedder.h"
#include <QDebug>
#include <QTimer>
#include <TlHelp32.h>

UnityEmbedder::UnityEmbedder(QWidget *parentWidget,
                             const std::string& windowTitle,
                             const std::string& exeName) :
    hostWidget(parentWidget),
    unityWindowTitle(windowTitle),
    unityExeName(exeName)
{
    ZeroMemory(&processInfo, sizeof(processInfo));

    // 1. Get the parent widget's native handle
    hostHwnd = reinterpret_cast<HWND>(hostWidget->winId());
}

UnityEmbedder::~UnityEmbedder() {
    terminateUnity();
}

fs::path UnityEmbedder::findUnityExe() const {
    fs::path projectRoot = fs::current_path().parent_path().parent_path().parent_path();
    fs::path unityExePath = projectRoot / unityWindowTitle / "build" / unityExeName;
    return unityExePath;
}

// Add this function to catch any Unity-related windows
BOOL CALLBACK UnityEmbedder::EnumWindowCallback(HWND hWnd, LPARAM lParam) {
    UnityEmbedder* self = reinterpret_cast<UnityEmbedder*>(lParam);

    char windowTitle[256];
    GetWindowTextA(hWnd, windowTitle, 256);
    std::string title(windowTitle);

    // Check for both the main window and any Unity-related windows
    if (title == self->unityWindowTitle) {
        SetWindowLong(hWnd, GWL_STYLE, WS_CHILD | WS_CLIPSIBLINGS);
        SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT);
        ShowWindow(hWnd, SW_HIDE);
        SetWindowPos(hWnd, HWND_BOTTOM, -32000, -32000, 0, 0,
                     SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
    }
    return TRUE;
}

void UnityEmbedder::forceHideAllUnityWindows() {
    EnumWindows(EnumWindowCallback, reinterpret_cast<LPARAM>(this));
}

void UnityEmbedder::terminateExistingUnityProcesses() {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;

    // Take a snapshot of all processes
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        qDebug() << "CreateToolhelp32Snapshot failed";
        return;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hProcessSnap, &pe32)) {
        qDebug() << "Process32First failed";
        CloseHandle(hProcessSnap);
        return;
    }

    // Convert Unity exe name to wide string for comparison
    std::wstring unityExeWide(unityExeName.begin(), unityExeName.end());

    do {
        // Compare process name with Unity exe name
        if (wcscmp(pe32.szExeFile, unityExeWide.c_str()) == 0) {
            qDebug() << "Found existing Unity process. Terminating...";

            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
            if (hProcess) {
                TerminateProcess(hProcess, 0);
                CloseHandle(hProcess);
                qDebug() << "Existing Unity process terminated.";
            }
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);

    // Wait a bit to ensure process cleanup
    Sleep(1000);
}

bool UnityEmbedder::launchUnity() {
    // First, check and terminate any existing Unity processes with the same name
    terminateExistingUnityProcesses();

    fs::path unityExePath = findUnityExe();
    if (!fs::exists(unityExePath)) {
        qDebug() << "Unity executable not found!";
        return false;
    }

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOW;  // Start hidden

    LPCWSTR lpApplicationName = unityExePath.c_str();
    if (!CreateProcess(lpApplicationName,
                       nullptr,
                       nullptr,
                       nullptr,
                       FALSE,
                       CREATE_NO_WINDOW,
                       nullptr,
                       nullptr,
                       &si,
                       &processInfo))
    {
        qDebug() << "Failed to start Unity process.";
        return false;
    }

    // Resume the process after setting up hiding
    ResumeThread(processInfo.hThread);

    // Immediately start hiding any Unity windows
    forceHideAllUnityWindows();

    qDebug() << "Unity process started successfully.";
    return true;
}

bool UnityEmbedder::findUnityWindow() {
    const int maxAttempts = 100; // 10 seconds timeout
    int attempts = 0;

    while (!unityHwnd && attempts < maxAttempts) {
        forceHideAllUnityWindows();  // Keep hiding windows while searching
        unityHwnd = FindWindowA(nullptr, unityWindowTitle.c_str());

        if (unityHwnd) {
            // Additional window style modifications
            LONG style = GetWindowLong(unityHwnd, GWL_STYLE);
            style &= ~(WS_POPUP | WS_VISIBLE | WS_MAXIMIZE | WS_OVERLAPPEDWINDOW);
            style |= WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
            SetWindowLong(unityHwnd, GWL_STYLE, style);

            LONG exStyle = GetWindowLong(unityHwnd, GWL_EXSTYLE);
            exStyle |= WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;
            SetWindowLong(unityHwnd, GWL_EXSTYLE, exStyle);

            SetWindowPos(unityHwnd, HWND_BOTTOM, -32000, -32000, 0, 0,
                         SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
            ShowWindow(unityHwnd, SW_HIDE);

            qDebug() << "Found Unity window handle:" << unityHwnd;
            return true;
        }

        Sleep(100);
        attempts++;
    }

    qDebug() << "Failed to find Unity window within timeout.";
    return false;
}

bool UnityEmbedder::waitForUnityReadySignal(int timeoutMs) {
    // Open the Windows event created by Unity.
    LPCWSTR eventName = L"Local\\FractalWaveUnityReady";
    HANDLE hEvent = OpenEvent(SYNCHRONIZE, FALSE, eventName);
    int retryIntervalMs = 100;  // Retry every 100ms
    DWORD elapsedTime = 0;

    // Try opening the event multiple times until timeout
    while (elapsedTime < timeoutMs) {
        hEvent = OpenEvent(SYNCHRONIZE, FALSE, eventName);
        if (hEvent) {
            break;  // Event found, break out of loop
        }

        // Event not found, wait for retry interval
        Sleep(retryIntervalMs);
        elapsedTime += retryIntervalMs;
    }

    // Wait for the event to be signaled (i.e. Unity indicates it has started rendering).
    DWORD result = WaitForSingleObject(hEvent, timeoutMs);
    CloseHandle(hEvent);

    if (result == WAIT_OBJECT_0) {
        qDebug() << "Unity ready signal received.";
        return true;
    } else {
        qDebug() << "Timeout waiting for Unity ready signal.";
        return false;
    }
}

// void UnityEmbedder::embedUnity(QWidget *stackedWidget) {
//     if (!unityHwnd) return;

//     // Store reference to parent widget
//     this->parentWidget = stackedWidget;

//     // 1. Get the parent widget's native handle
//     HWND parentHwnd = reinterpret_cast<HWND>(parentWidget->winId());

//     // 2. Reparent Unity's window using Windows API
//     SetParent(unityHwnd, parentHwnd);

//     // Ensure window is completely hidden before embedding
//     forceHideAllUnityWindows();

//     QWindow *unityWindow = QWindow::fromWinId((WId)unityHwnd);
//     unityWidget = QWidget::createWindowContainer(unityWindow, parentWidget);
//     unityWidget->setAttribute(Qt::WA_DontCreateNativeAncestors);
//     unityWidget->setAttribute(Qt::WA_NativeWindow);
//     unityWidget->setMinimumSize(parentWidget->size());

//     // Clear any previous layout in case of re-embedding
//     QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(parentWidget->layout());
//     if (!layout) {
//         layout = new QVBoxLayout(parentWidget);
//         parentWidget->setLayout(layout);
//     }
//     layout->setContentsMargins(0, 0, 0, 0);  // Remove margins
//     layout->setSpacing(0);  // Remove spacing

//     // Add to parent widget
//     layout->addWidget(unityWidget);
// }

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

void UnityEmbedder::embedUnity() {
    if (!unityHwnd || !hostWidget) return;

    // 2. Reparent Unity's window using Windows API
    SetParent(unityHwnd, hostHwnd);

    // give it WS_CHILD so it clips to the parent
    LONG_PTR style = GetWindowLongPtr(unityHwnd, GWL_STYLE);
    style = (style & ~WS_POPUP) | WS_CHILD;
    SetWindowLongPtr(unityHwnd, GWL_STYLE, style);

    resizeToHost();
}

void UnityEmbedder::resizeToHost()
{
    RECT r;
    GetClientRect(hostHwnd, &r);

    // move and resize Unity to exactly fill the host’s client area
    MoveWindow(
        unityHwnd,
        r.left, r.top,
        r.right - r.left,
        r.bottom - r.top,
        TRUE  // repaint
        );
}

void UnityEmbedder::debugWindowHierarchy() const {
    if (!unityHwnd) return;

    HWND current = unityHwnd;
    qDebug() << "Unity window hierarchy:";
    while (current) {
        char className[256];
        GetClassNameA(current, className, sizeof(className));
        qDebug() << "  HWND:" << current << "Class:" << className;
        current = GetParent(current);
    }
}

void UnityEmbedder::terminateUnity() {
    if (processInfo.hProcess) {
        TerminateProcess(processInfo.hProcess, 0);
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
        qDebug() << "Unity process terminated.";
    }
}

// void UnityEmbedder::resizeUnityWindow(const QSize& newSize) {
//     if (!unityHwnd || !unityWidget) return;

//     // Resize the Unity window via Windows API
//     SetWindowPos(unityHwnd, nullptr, 0, 0, newSize.width(), newSize.height(),
//                  SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

//     // Also resize the Qt widget container
//     unityWidget->setFixedSize(newSize);

//     qDebug() << "Resized Unity window to:" << newSize;
// }
