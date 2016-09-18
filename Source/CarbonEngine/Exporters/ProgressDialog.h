/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/Threads/Mutex.h"
#include "CarbonEngine/Exporters/DialogBase.h"

namespace Carbon
{

/**
 * This class shows a progress dialog for the invocation of the Runnable::run() method for a Runnable subclass instance.
 * It is fully multithreaded and interacts with the running task through the methods on the Runnable class.
 */
class CARBON_API ProgressDialog : public DialogBase, public Logfile::OutputSink
{
public:

    /**
     * Constructs this progress dialog with the specified title.
     */
    ProgressDialog(const UnicodeString& title) { setTitle(title); }

    /**
     * Shows the progress dialog for the invocation of the Runnable::run() method on the given Runnable instance.
     */
    void show(Runnable& r,
#if defined(WINDOWS)
              HWND hWndParent);
#else
              void* parentWindow);
#endif

private:

#ifdef WINDOWS

    Runnable* runnable_ = nullptr;
    UnicodeString detailedOutput_;
    bool isTaskComplete_ = false;
    String currentTaskString_;

    mutable Mutex detailedOutputMutex_;    // Mutex for access to the 'detailedOutput_' variable

    HANDLE hUpdateOutputEvent_ = nullptr;       // Set when the log output is updated
    HANDLE hWorkerCompleteEvent_ = nullptr;     // Set when the worker thread finishes
    HANDLE hWorkerSucceededEvent_ = nullptr;    // Set when the worker thread successfully completes the job

    HANDLE workerThread_ = nullptr;

    // This is called when something is written to a logfile. It prepends the text to detailedOutput_ so it can be
    // displayed in the detailed output window in the dialog.
    void processLogfileOutput(Logfile::OutputType outputType, const UnicodeString& line);

    // Entry point for the worker thread which does the actual invocation of runnable_->run()
    unsigned int workerThreadMain();
    static unsigned int __stdcall staticWorkerThreadMain(void* lpParameter);

    // Main dialog procedure
    LRESULT dialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) override;

#endif
};

}
