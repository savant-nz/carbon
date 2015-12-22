/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Resource.h"
#include "CarbonEngine/Core/BuildInfo.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/SettingsManager.h"
#include "CarbonEngine/Core/Runnable.h"
#include "CarbonEngine/Exporters/ProgressDialog.h"

namespace Carbon
{

#ifdef WINDOWS

void ProgressDialog::processLogfileOutput(Logfile::OutputType outputType, const UnicodeString& line)
{
    auto lock = ScopedMutexLock(detailedOutputMutex_);

    detailedOutput_ = line + "\13\10" + detailedOutput_;

    SetEvent(hUpdateOutputEvent_);
}

unsigned int ProgressDialog::workerThreadMain()
{
    Logfile::addOutputSink(this);

    // Run the task
    if (runnable_->run())
    {
        LOG_INFO << "Job complete";
        SetEvent(hWorkerSucceededEvent_);
    }
    else
    {
        if (runnable_->isCancelled())
            LOG_INFO << "Job cancelled";
        else
            LOG_INFO << "Job failed";
    }

    Logfile::addOutputSink(this);

    // Tell the dialog we're done
    SetEvent(hWorkerCompleteEvent_);

    return 1;
}

unsigned int __stdcall ProgressDialog::staticWorkerThreadMain(void* lpParameter)
{
    return reinterpret_cast<ProgressDialog*>(lpParameter)->workerThreadMain();
}

LRESULT ProgressDialog::dialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    const auto updateDialogTimerID = 1U;
    const auto progressDialogShowDetailedOutputSetting = String("ProgressDialogShowDetailedOutput");

    switch (message)
    {
        case WM_INITDIALOG:
        {
            detailedOutput_.clear();
            isTaskComplete_ = false;
            currentTaskString_.clear();

            // Set the exporter version label
            SetWindowTextA(GetDlgItem(hDlg, IDC_VERSION), ("Version " + BuildInfo::getVersion()).cStr());

            // Hide the Close button
            ShowWindow(GetDlgItem(hDlg, IDC_CLOSE), SW_HIDE);

            // Start a timer to update the progress bar at 25Hz
            SetTimer(hDlg, updateDialogTimerID, 40, nullptr);

            // Set range of progress bar to 0-1000
            SendMessage(GetDlgItem(hDlg, IDC_PROGRESS), PBM_SETRANGE32, 0, 1000);

            // Move the Close button on top of the Cancel button, only one is available at any given time
            auto cancelRect = RECT();
            GetWindowRect(GetDlgItem(hDlg, IDC_CANCEL), &cancelRect);
            auto p = POINT{cancelRect.left, cancelRect.top};
            ScreenToClient(hDlg, &p);
            SetWindowPos(GetDlgItem(hDlg, IDC_CLOSE), nullptr, p.x, p.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

            // Initialize the show detailed output checkbox and state
            if (settings().getBoolean(progressDialogShowDetailedOutputSetting))
                CheckDlgButton(hDlg, IDC_SHOW_DETAILED_OUTPUT, BST_CHECKED);
            else
            {
                CheckDlgButton(hDlg, IDC_SHOW_DETAILED_OUTPUT, BST_UNCHECKED);
                dialogProc(hDlg, WM_COMMAND, MAKEWPARAM(IDC_SHOW_DETAILED_OUTPUT, 0), 0);
            }

            // Create events used for inter-thread communication
            hUpdateOutputEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            hWorkerCompleteEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            hWorkerSucceededEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);

            // Start worker thread
            workerThread_ = HANDLE(_beginthreadex(nullptr, 0, staticWorkerThreadMain, this, 0, nullptr));
            if (workerThread_)
                SetThreadPriority(workerThread_, THREAD_PRIORITY_BELOW_NORMAL);

            return TRUE;
        }

        case WM_DESTROY:
        {
            // The worker thread should be finished by now assuming everything worked correctly. However, it is important to
            // check it is finished, as odd behavior or crashes could result from lurking worker threads
            auto workerThreadExitCode = DWORD();
            GetExitCodeThread(workerThread_, &workerThreadExitCode);
            if (workerThreadExitCode == STILL_ACTIVE)
            {
                // Give the worker thread more time to close
                Sleep(10000);

                // Check the thread state again
                GetExitCodeThread(workerThread_, &workerThreadExitCode);
                if (workerThreadExitCode == STILL_ACTIVE)
                {
                    LOG_WARNING << "Worker thread is still active after progress dialog closed";

                    MessageBoxA(GetParent(hDlg), "Warning: Worker thread still active after progress dialog closed",
                                Globals::getClientName().cStr(), MB_ICONERROR);
                }
            }

            // Cleanup
            CloseHandle(workerThread_);
            CloseHandle(hUpdateOutputEvent_);
            CloseHandle(hWorkerCompleteEvent_);
            CloseHandle(hWorkerSucceededEvent_);

            break;
        }

        case WM_TIMER:
        {
            if (wParam == updateDialogTimerID)
            {
                events().dispatchEvent(UpdateEvent());

                // Update log output display if required
                if (WaitForSingleObject(hUpdateOutputEvent_, 0) == WAIT_OBJECT_0)
                {
                    auto lock = ScopedMutexLock(detailedOutputMutex_);

                    // Update text in the output editbox from the 'output' variable
                    SetWindowTextW(GetDlgItem(hDlg, IDC_OUTPUT), detailedOutput_.toUTF16().as<wchar_t>());
                }

                if (WaitForSingleObject(hWorkerCompleteEvent_, 0) == WAIT_OBJECT_0)
                {
                    isTaskComplete_ = true;

                    // Hide the Cancel button and show the Close button
                    ShowWindow(GetDlgItem(hDlg, IDC_CANCEL), SW_HIDE);
                    ShowWindow(GetDlgItem(hDlg, IDC_CLOSE), SW_SHOW);
                    SetFocus(GetDlgItem(hDlg, IDC_CLOSE));

                    // Check if job completed successfully
                    if (WaitForSingleObject(hWorkerSucceededEvent_, 0) == WAIT_OBJECT_0)
                    {
                        // Set task label to 'Done' and progress bar to full
                        SetWindowTextA(GetDlgItem(hDlg, IDC_TASK), "Done");
                        PostMessage(GetDlgItem(hDlg, IDC_PROGRESS), PBM_SETPOS, 1000, 0);
                    }
                    else
                    {
                        // The job did not complete successfully

                        if (runnable_->isCancelled())
                        {
                            // Close the dialog following cancellation
                            EndDialog(hDlg, IDC_CLOSE);
                        }
                        else
                            SetWindowTextA(GetDlgItem(hDlg, IDC_TASK), "Failed");
                    }
                }

                if (!isTaskComplete_)
                {
                    // Update current task display
                    if (runnable_->isTaskStringDirty())
                    {
                        auto newTaskString = String();
                        if (IsDlgButtonChecked(hDlg, IDC_SHOW_DETAILED_OUTPUT))
                            newTaskString = runnable_->getTaskString();
                        else
                            newTaskString = runnable_->getSimpleTaskString();

                        newTaskString += " ...";

                        // Check the new task string is different
                        if (currentTaskString_ != newTaskString)
                        {
                            SetWindowTextA(GetDlgItem(hDlg, IDC_TASK), newTaskString.cStr());
                            currentTaskString_ = newTaskString;
                        }
                    }

                    // Update progress bar position
                    if (runnable_->isPercentageDirty())
                        PostMessage(GetDlgItem(hDlg, IDC_PROGRESS), PBM_SETPOS, int(runnable_->getPercentage() * 10.0f), 0);
                }
            }

            return TRUE;
        }

        case WM_CLOSE:
        {
            // Close the dialog if the task is complete, otherwise cancel it
            if (isTaskComplete_)
                EndDialog(hDlg, IDC_CLOSE);
            else
                dialogProc(hDlg, WM_COMMAND, MAKEWPARAM(IDC_CANCEL, 0), 0);

            return TRUE;
        }

        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDC_CANCEL)
            {
                runnable_->cancel();
                EnableWindow(GetDlgItem(hDlg, IDC_CANCEL), FALSE);
                SetWindowTextA(GetDlgItem(hDlg, IDC_TASK), "Cancelling ...");
            }
            else if (LOWORD(wParam) == IDC_CLOSE)
            {
                // Close the dialog
                EndDialog(hDlg, IDC_CLOSE);
            }
            else if (LOWORD(wParam) == IDC_SHOW_DETAILED_OUTPUT)
            {
                // Show/hide the detailed output
                auto showDetailedOutput = (IsDlgButtonChecked(hDlg, IDC_SHOW_DETAILED_OUTPUT) == BST_CHECKED);

                // Update output window visibility
                ShowWindow(GetDlgItem(hDlg, IDC_OUTPUT), showDetailedOutput);

                // Get sizes of the dialog items needed to do resize the dialog window correctly
                auto cancelRect = RECT();
                auto outputRect = RECT();
                auto dialogRect = RECT();
                GetWindowRect(GetDlgItem(hDlg, IDC_CANCEL), &cancelRect);
                GetWindowRect(GetDlgItem(hDlg, IDC_OUTPUT), &outputRect);
                GetWindowRect(hDlg, &dialogRect);

                auto heightDelta = (outputRect.bottom - cancelRect.bottom);
                if (!showDetailedOutput)
                    heightDelta = -heightDelta;

                // Update dialog size
                SetWindowPos(hDlg, nullptr, 0, 0, dialogRect.right - dialogRect.left,
                             dialogRect.bottom - dialogRect.top + heightDelta, SWP_NOZORDER | SWP_NOMOVE);

                // Force the task string to update
                runnable_->setTaskStringDirty();

                // Update the persistent show detailed output setting
                settings().set(progressDialogShowDetailedOutputSetting, showDetailedOutput);
            }

            break;
        }
    }

    return FALSE;
}

void ProgressDialog::show(Runnable& r, HWND hWndParent)
{
    // Store pointer to the Runnable instance
    runnable_ = &r;

    // Show dialog box
    if (DialogBoxParam(Globals::getHInstance(), LPCTSTR(IDD_PROGRESS), hWndParent, DLGPROC(staticDialogProc), LPARAM(this)) < 1)
    {
        LOG_ERROR << "Failed showing progress dialog";
        MessageBoxA(hWndParent, "Error: Failed showing progress dialog", Globals::getClientName().cStr(), MB_ICONERROR);
    }
}

#else

void ProgressDialog::show(Runnable& r, void* parentWindow)
{
    LOG_ERROR << "ProgressDialog is not implemented on this platform";

    r.run();
}

#endif
}
