//
//  browser.cpp
//  webview
//
//  Created by Mr.Panda on 2023/4/26.
//

#include "browser.h"

#include "include/base/cef_callback.h"
#include "include/cef_app.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

IBrowser::IBrowser(PageOptions settings,
                   PageObserver observer,
                   void* ctx)
    : _settings(settings)
    , _observer(observer)
    , _ctx(ctx)
    , IRender(settings, observer, ctx)
    , IDisplay(settings, observer, ctx)
{
}

CefRefPtr<CefDragHandler> IBrowser::GetDragHandler()
{
    return this;
}

void IBrowser::OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefFrame> frame,
                                   CefRefPtr<CefContextMenuParams> params,
                                   CefRefPtr<CefMenuModel> model)
{
    CEF_REQUIRE_UI_THREAD();

    if (params->GetTypeFlags() & (CM_TYPEFLAG_SELECTION | CM_TYPEFLAG_EDITABLE))
    {
        return;
    }

    model->Clear();
}

CefRefPtr<CefContextMenuHandler> IBrowser::GetContextMenuHandler()
{
    return this;
}

bool IBrowser::OnContextMenuCommand(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    CefRefPtr<CefContextMenuParams> params,
                                    int command_id,
                                    EventFlags event_flags)
{
    CEF_REQUIRE_UI_THREAD();
    return false;
};

CefRefPtr<CefDisplayHandler> IBrowser::GetDisplayHandler()
{
    if (_is_closed)
    {
        return nullptr;
    }

    return this;
}

CefRefPtr<CefLifeSpanHandler> IBrowser::GetLifeSpanHandler()
{
    if (_is_closed)
    {
        return nullptr;
    }

    return this;
}

CefRefPtr<CefLoadHandler> IBrowser::GetLoadHandler()
{
    if (_is_closed)
    {
        return nullptr;
    }

    return this;
}

CefRefPtr<CefRenderHandler> IBrowser::GetRenderHandler()
{
    if (this->_settings.is_offscreen)
    {
        return this;
    }

    return nullptr;
}


void IBrowser::OnLoadStart(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame,
                           TransitionType transition_type)
{
    if (_is_closed)
    {
        return;
    }

    _observer.on_state_change(PageState::BeforeLoad, _ctx);
}

void IBrowser::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         int httpStatusCode)
{
    CEF_REQUIRE_UI_THREAD();

    if (_is_closed)
    {
        return;
    }

    _observer.on_state_change(PageState::Load, _ctx);
}

void IBrowser::OnLoadError(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame,
                           ErrorCode error_code,
                           const CefString& error_text,
                           const CefString& failed_url)
{
    CEF_REQUIRE_UI_THREAD();

    if (_is_closed)
    {
        return;
    }

    _observer.on_state_change(PageState::LoadError, _ctx);

    if (error_code == ERR_ABORTED)
    {
        return;
    }

    // TODO: send error web page.
    // frame->LoadURL(GetDataURI(html.str(), "text/html"));
}

void IBrowser::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
    if (_is_closed)
    {
        return;
    }

    browser->GetHost()->WasResized();

    IRender::SetBrowser(browser);
    IControl::SetBrowser(browser);
    _browser = browser;
}

bool IBrowser::DoClose(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();
    return false;
}

bool IBrowser::OnBeforePopup(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             const CefString& target_url,
                             const CefString& target_frame_name,
                             CefLifeSpanHandler::WindowOpenDisposition target_disposition,
                             bool user_gesture,
                             const CefPopupFeatures& popupFeatures,
                             CefWindowInfo& window_info,
                             CefRefPtr<CefClient>& client,
                             CefBrowserSettings& settings,
                             CefRefPtr<CefDictionaryValue>& extra_info,
                             bool* no_javascript_access)
{
    browser->GetMainFrame()->LoadURL(target_url);
    return true;
}

bool IBrowser::OnDragEnter(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefDragData> dragData,
                           CefDragHandler::DragOperationsMask mask)
{
    return true;
}

void IBrowser::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();

    _observer.on_state_change(PageState::BeforeClose, _ctx);
    _observer.on_state_change(PageState::Close, _ctx);
    _browser = std::nullopt;
}

void IBrowser::SetDevToolsOpenState(bool is_open)
{
    if (_is_closed)
    {
        return;
    }

    if (!_browser.has_value())
    {
        return;
    }

    if (is_open)
    {
        _browser.value()->GetHost()->ShowDevTools(CefWindowInfo(), 
                                                  nullptr, 
                                                  CefBrowserSettings(),
                                                  CefPoint());
    }
    else
    {
        _browser.value()->GetHost()->CloseDevTools();
    }
}

const void* IBrowser::GetHWND()
{
    return _browser.has_value() ? _browser.value()->GetHost()->GetWindowHandle() : nullptr;
}

bool IBrowser::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                        CefRefPtr<CefFrame> frame,
                                        CefProcessId source_process,
                                        CefRefPtr<CefProcessMessage> message)
{
    if (_is_closed)
    {
        return false;
    }

    if (!_browser.has_value())
    {
        return false;
    }

    auto args = message->GetArgumentList();
    std::string payload = args->GetString(0);
    _observer.on_message(payload.c_str(), _ctx);
    return true;
}

void IBrowser::ISendMessage(std::string message)
{
    if (_is_closed)
    {
        return;
    }

    if (!_browser.has_value())
    {
        return;
    }

    auto msg = CefProcessMessage::Create("MESSAGE_TRANSPORT");
    CefRefPtr<CefListValue> args = msg->GetArgumentList();
    args->SetSize(1);
    args->SetString(0, message);
    _browser.value()->GetMainFrame()->SendProcessMessage(PID_RENDERER, msg);
}

void IBrowser::IClose()
{
    if (_is_closed)
    {
        return;
    }

    if (!_browser.has_value())
    {
        return;
    }

    IRender::IClose();
    IDisplay::IClose();
    IControl::IClose();
    _browser.value()->GetHost()->CloseBrowser(true);

    _browser = std::nullopt;
    _is_closed = true;
}
