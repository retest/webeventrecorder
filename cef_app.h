#pragma once

#include "include/cef_app.h"

class MyCefApp : public CefApp,
                 public CefBrowserProcessHandler,
				 public CefRenderProcessHandler
{
public:
	MyCefApp();

	virtual void OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line) OVERRIDE;

	// CefApp methods:
	virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler()
		OVERRIDE { return this; }
	virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler()
		OVERRIDE { return this; }

	// CefBrowserProcessHandler methods:
	virtual void OnContextInitialized() OVERRIDE;

	// Called on the browser process IO thread after the main thread has been
	// created for a new render process. Provides an opportunity to specify extra
	// information that will be passed to
	// CefRenderProcessHandler::OnRenderThreadCreated() in the render process. Do
	// not keep a reference to |extra_info| outside of this method.
	virtual void OnRenderProcessThreadCreated(CefRefPtr<CefListValue> extra_info) OVERRIDE;

	void StartBrowserOnTab(HWND hwnd, std::string start_url);
	void StopBrowserOnTab();

	// CefRenderProcessHandler methods:
	virtual void OnContextCreated( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) OVERRIDE;

	// Called after the render process main thread has been created. |extra_info|
	// is a read-only value originating from
	// CefBrowserProcessHandler::OnRenderProcessThreadCreated(). Do not keep a
	// reference to |extra_info| outside of this method.
	virtual void OnRenderThreadCreated(CefRefPtr<CefListValue> extra_info) OVERRIDE;

private:
	HWND h_win;
	std::string gui_path;
	std::string saving_path;
	// Include the default reference counting implementation.
	IMPLEMENT_REFCOUNTING(MyCefApp);
};

class MyV8Handler : public CefV8Handler
{
public:
	MyV8Handler(const std::string& saving_path, CefRefPtr<CefBrowser> browser) : saving_path(saving_path), browser(browser) {}
	virtual bool Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
		OVERRIDE;
private:
	std::string saving_path;
	CefRefPtr<CefBrowser> browser;
	// Include the default reference counting implementation.
	IMPLEMENT_REFCOUNTING(MyV8Handler);
};