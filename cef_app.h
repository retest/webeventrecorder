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

	void StartBrowserOnTab(HWND hwnd, std::string start_url);
	void StopBrowserOnTab();

	// CefRenderProcessHandler methods:
	virtual void OnContextCreated( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) OVERRIDE;

private:
	HWND h_win;

	// Include the default reference counting implementation.
	IMPLEMENT_REFCOUNTING(MyCefApp);
};

class MyV8Handler : public CefV8Handler
{
public:
	virtual bool Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
		OVERRIDE;
private:
	// Include the default reference counting implementation.
	IMPLEMENT_REFCOUNTING(MyV8Handler);
};