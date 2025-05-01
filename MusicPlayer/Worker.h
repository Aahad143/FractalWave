#ifndef WORKER_H
#define WORKER_H

#include <QObject>

#include <qdebug.h>
#include <utility>
#include <functional>
#include <iostream>


class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = nullptr)
        : QObject(parent) {}

    // Public interface to schedule a variadic call from any thread:
    template<typename... Args>
    void scheduleFunctions(Args&&... args);

private slots:
    //— The non-template slot that Qt can call —//
    void runScheduledTask()
    {
        if (m_scheduledTask) {
            try {
                m_scheduledTask();
            }
            catch (std::exception &ex) {
                emit error(QString::fromUtf8(ex.what()));
            }
            // clear m_scheduledTask
            m_scheduledTask = nullptr;
        }
    }

signals:
    void finished();
    void error(const QString &err);

private:
    std::function<void()> m_scheduledTask;
};

//— scheduleVarFunc captures the exact functions+tuples into a single std::function —//
template<typename... Callables>
void Worker::scheduleFunctions(Callables&&... funcs)
{
    // Capture everything by value into our std::function:
    m_scheduledTask = [tasks = std::make_tuple(std::forward<Callables>(funcs)...), this]() mutable {
        // // Expand the tuple: it's (f1, t1, f2, t2, …)
        // std::apply(
        //     [](auto&&... callables) {
        //         // Simply invoke each one in turn
        //         (std::invoke(callables), ...);
        //     },
        //     std::move(tasks)
        //     );
        // emit finished();

        // Unpack and invoke each callable in order, with return-value checking:
        // a little local flag
        bool ok = true;

        // 3) unpack the tuple, call each one in turn:
        std::apply(
            [&](auto&&... callables) {
                // fold expression to run them all
                (([&] {
                     using R = std::invoke_result_t<decltype(callables)>;
                     if constexpr (std::is_same_v<R, bool>) {
                         // if it returns bool, check it
                         if (!std::invoke(callables)) {
                             throw std::runtime_error("A step returned false – aborting");
                             ok = false;
                         }
                     } else {
                         // invoke if previous callable not false
                         if (ok)
                             std::invoke(callables);
                     }
                 }()), ...);
            },
            std::move(tasks)
            );

        // 4) if everything was OK, signal finished
        if (ok)
            emit finished();
    };

    // Queue the non-template slot on this thread’s event loop:
    QMetaObject::invokeMethod(
        this,
        "runScheduledTask",
        Qt::QueuedConnection
        );
}

#endif // WORKER_H
