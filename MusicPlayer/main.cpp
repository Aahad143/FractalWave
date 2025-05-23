#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    qRegisterMetaType<Track*>("Track");
    // Set up Windows unhandled exception handler
    SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* exceptionInfo) -> LONG {
        qCritical() << "Unhandled exception caught! Code:" << exceptionInfo->ExceptionRecord->ExceptionCode;
        return EXCEPTION_CONTINUE_SEARCH;  // Let other handlers run
    });

    QApplication a(argc, argv);
    a.setStyleSheet(R"(
    * {
        font-family: "Fira Sans";
    }
)");
    MainWindow w;
    w.show();
    return a.exec();
}
