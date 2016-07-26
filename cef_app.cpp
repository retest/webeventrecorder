
#include "cef_app.h"

#include <string>

#include "cef_handler.h"
#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/wrapper/cef_helpers.h"

MyCefApp::MyCefApp()
	:h_win(NULL)
{
}

void MyCefApp::OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line)
{
	command_line->AppendSwitch("disable-extensions");
}

void MyCefApp::OnContextInitialized() {
  CEF_REQUIRE_UI_THREAD();
}

void MyCefApp::StartBrowserOnTab(HWND hwnd)
{
	DWORD ThreadId = GetCurrentThreadId();
		// Information used when creating the native window.
  CefWindowInfo window_info;

#if defined(OS_WIN)
  // On Windows we need to specify certain flags that will be passed to
  // CreateWindowEx().
  window_info.SetAsPopup(hwnd, "xyrec_cef");
#endif
  RECT rect;
  //GetClientRect(hwnd, &rect);
  rect.bottom = rect.left = 0;
  rect.top = rect.right = 0;
  window_info.SetAsChild(hwnd, rect);
  // SimpleHandler implements browser-level callbacks.
  CefRefPtr<SimpleHandler> handler(new SimpleHandler());

  // Specify CEF browser settings here.
  CefBrowserSettings browser_settings;

  std::string url;

  // Check if a "--url=" value was provided via the command-line. If so, use
  // that instead of the default URL.
  CefRefPtr<CefCommandLine> command_line =
      CefCommandLine::GetGlobalCommandLine();
  url = command_line->GetSwitchValue("url");
  if (url.empty())
    url = "http://www.google.com";
	//url = "http://www.w3schools.com/tags/tryit.asp?filename=tryhtml_select";
	//url = "https://www.expedia.com/";
		

	// Create the first browser window.
	CefBrowserHost::CreateBrowser(window_info, handler.get(), url,
                                browser_settings, NULL);	
}

void MyCefApp::StopBrowserOnTab()
{	
	SimpleHandler::GetInstance()->CloseAllBrowsers(true);
}