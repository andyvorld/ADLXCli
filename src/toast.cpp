#include "toast.hpp"

#include <windows.h>
#include <wintoastlib.h>

#include <future>

#include "version.h"

#define APP_NAME L"ADLXCli"
#define APP_VERSION APP_VERSION_STRING_W
#define APP_AUMI L"andyvorld." APP_NAME L".App." APP_VERSION

using namespace WinToastLib;

class CustomHandler : public IWinToastHandler {
   private:
    void _toastClosed() const { toastClosed.set_value(); };

   public:
    mutable std::promise<void> toastClosed;

    CustomHandler() : toastClosed() {}

    void toastActivated() const { _toastClosed(); };
    void toastActivated(int actionIndex) const { _toastClosed(); };
    void toastActivated(std::wstring response) const { _toastClosed(); };
    void toastDismissed(WinToastDismissalReason state) const { _toastClosed(); };
    void toastFailed() const { _toastClosed(); };
};

void ShowToast(std::wstring title, std::wstring content) {
    if (!WinToast::isCompatible()) return;

    WinToast::instance()->setAppName(APP_NAME);
    WinToast::instance()->setAppUserModelId(APP_AUMI);
    WinToast::instance()->initialize();

    WinToastTemplate templ = WinToastTemplate(WinToastTemplate::Text02);
    templ.setTextField(title, WinToastTemplate::FirstLine);
    templ.setTextField(content, WinToastTemplate::SecondLine);
    templ.setAudioOption(WinToastTemplate::AudioOption::Silent);

    auto handler = new CustomHandler();
    WinToast::instance()->showToast(templ, handler);

    handler->toastClosed.get_future().wait();
}