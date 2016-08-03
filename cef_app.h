#pragma once

#include "include/cef_app.h"

class MyCefApp : public CefApp,
                 public CefBrowserProcessHandler 
{
public:
	MyCefApp();

	virtual void OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line) OVERRIDE;

	// CefApp methods:
	virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler()
		OVERRIDE { return this; }

	// CefBrowserProcessHandler methods:
	virtual void OnContextInitialized() OVERRIDE;

	void StartBrowserOnTab(HWND hwnd, std::string start_url);
	void StopBrowserOnTab();

private:
	HWND h_win;

	// Include the default reference counting implementation.
	IMPLEMENT_REFCOUNTING(MyCefApp);
};