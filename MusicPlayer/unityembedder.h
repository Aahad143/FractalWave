#ifndef UNITYEMBEDDER_H
#define UNITYEMBEDDER_H

#include <Windows.h>
#include <QApplication>
#include <QWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWindow>
#include <filesystem>

namespace fs = std::filesystem;

class UnityEmbedder {
public:
    UnityEmbedder(QWidget* parentWidget, const std::string& windowTitle, const std::string& exeName);
    ~UnityEmbedder();

    bool launchUnity();
    bool findUnityWindow();
    static bool waitForUnityReadySignal(int timeoutMs = 10000);
    void embedUnity();
    void debugWindowHierarchy() const;

    void terminateUnity();
    // void resizeUnityWindow(const QSize& newSize);

    HWND getHwnd() {
        return unityHwnd;
    }

    // Add this to UnityEmbedder.h
    QWidget* getQWidget() const {
        return hostWidget;
    }

    void resizeToHost();

private:
    PROCESS_INFORMATION processInfo;
    std::string unityWindowTitle;
    std::string unityExeName;

    fs::path findUnityExe() const;
    void terminateExistingUnityProcesses();

    // Add these as class members to track window state
    bool isInitiallyHidden = false;
    static BOOL CALLBACK EnumWindowCallback(HWND hWnd, LPARAM lParam);
    void forceHideAllUnityWindows();

    QWidget *hostWidget = nullptr;
    HWND hostHwnd = nullptr;
    HWND unityHwnd = nullptr;
};

#endif // UNITYEMBEDDER_H
