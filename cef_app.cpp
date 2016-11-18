
#include "cef_app.h"

#include <string>
#include <fstream>
#include "cef_handler.h"
#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/wrapper/cef_helpers.h"

#include "web_event_browser.h"

wxDECLARE_APP(WevebApp);


MyCefApp::MyCefApp()
	:h_win(NULL)
{
}

void MyCefApp::OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line)
{
	command_line->AppendArgument("disable-extensions");
}

void MyCefApp::OnContextInitialized() {
  CEF_REQUIRE_UI_THREAD();
}

void MyCefApp::OnRenderProcessThreadCreated(CefRefPtr<CefListValue> extra_info)
{
	CEF_REQUIRE_IO_THREAD(); 

	extra_info->SetString(0, wxGetApp().GetMainFrame()->GetGuiPath()); // gui path	
	extra_info->SetString(1, wxGetApp().GetMainFrame()->GetPathForSaving("")); // path for saving
}

void MyCefApp::StartBrowserOnTab(HWND hwnd, std::string start_url)
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
	  url = start_url;
    //url = "http://www.google.com";
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

void MyCefApp::OnRenderThreadCreated(CefRefPtr<CefListValue> extra_info)
{
	// Read params
	gui_path = extra_info->GetString(0);
	saving_path = extra_info->GetString(1);
}

void MyCefApp::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
{
	// Retrieve the context's window object.
	CefRefPtr<CefV8Value> object = context->GetGlobal();
	
	CefRefPtr<CefV8Handler> handler = new MyV8Handler(saving_path, browser);
	object->SetValue("cef_js_func",
		CefV8Value::CreateFunction("cef_js_func", handler),
		V8_PROPERTY_ATTRIBUTE_NONE);

	object->SetValue("cef_js_track_mouse",
		CefV8Value::CreateFunction("cef_js_track_mouse", handler),
		V8_PROPERTY_ATTRIBUTE_NONE);

	object->SetValue("cef_js_track_focus",
		CefV8Value::CreateFunction("cef_js_track_focus", handler),
		V8_PROPERTY_ATTRIBUTE_NONE);

	// init tracking js mouse script
	static std::string js_init_tracking_code;
	if (js_init_tracking_code.length() == 0)
	{
		std::string js_path = gui_path;
		js_path += "//js//css-selector-generator.min.js";

		std::ifstream js_file(js_path);
		if (js_file.is_open())
		{
			std::stringstream js;
			js << js_file.rdbuf();
			js_init_tracking_code = js.str();
		}
	}

	if (js_init_tracking_code.length() != 0)
		frame->ExecuteJavaScript(js_init_tracking_code, frame->GetURL(), 0);

	// init focus tracking script
	static std::string js_init_tracking_focus;
	if (js_init_tracking_focus.length() == 0)
	{
		std::string js_path = gui_path;
		js_path += "//js//focushandler.js";

		std::ifstream js_file(js_path);
		if (js_file.is_open())
		{
			std::stringstream js;
			js << js_file.rdbuf();
			js_init_tracking_focus = js.str();
		}
	}

	if (js_init_tracking_focus.length() != 0)
		frame->ExecuteJavaScript(js_init_tracking_focus, frame->GetURL(), 0);
}

bool MyV8Handler::Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
{
	if (name == "cef_js_func")
	{
		CefString str;
		if (arguments.size() > 0)
		{
			str = arguments[0]->GetStringValue();

			wxLongLong ts = wxGetLocalTimeMillis();
			std::string file_name = "js_" + ts.ToString() + ".txt";
			file_name = saving_path + file_name;

			std::ofstream out(file_name);
			out << str.ToString();			

			wxString fname = file_name;
			fname.Replace("\\", "\\\\");
					
			// Create the message object.
			CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("cef_js_func");

			// Retrieve the argument list object.
			CefRefPtr<CefListValue> args = msg->GetArgumentList();

			// Populate the argument values.
			args->SetString(0, "{\"callback\": \"javascript\", \"file\": \"" + fname.ToStdString() + "\"}");

			// Send the process message to the browser process.
			browser->SendProcessMessage(PID_BROWSER, msg);
			return true;
		}
	}
	else if (name == "cef_js_track_mouse")
	{
		int client_x = arguments[0]->GetIntValue();
		int client_y = arguments[1]->GetIntValue();

		std::string result = arguments[2]->GetStringValue();

		CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("cef_js_track_mouse");
		CefRefPtr<CefListValue> args = msg->GetArgumentList();
		args->SetInt(0, client_x);
		args->SetInt(1, client_y);
		args->SetString(2, result);
		browser->SendProcessMessage(PID_BROWSER, msg);
		return true;
	}
	else if (name == "cef_js_track_focus")
	{
		std::string result = arguments[0]->GetStringValue();

		CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("js_focus_event");
		CefRefPtr<CefListValue> args = msg->GetArgumentList();
		args->SetString(0, result);
		browser->SendProcessMessage(PID_BROWSER, msg);
		return true;
	}

	// Function does not exist.
	return false;
}