#pragma once

#include "include/cef_client.h"

#include <map>

class SimpleHandler : public CefClient,
                      public CefDisplayHandler,
                      public CefLifeSpanHandler,
                      public CefLoadHandler,
					  public CefKeyboardHandler,
					  public CefDownloadHandler,
					  public CefRequestHandler
{
public:
	SimpleHandler();
	~SimpleHandler();

	//  CefRefPtr<CefBrowser> GetActiveBrowser() { return browser_list_.front();}


	// Provide access to the single global instance of this object.
	static SimpleHandler* GetInstance();

	// CefClient methods:
	virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() OVERRIDE {
		return this;
	}
	virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() OVERRIDE {
		return this;
	}
	virtual CefRefPtr<CefLoadHandler> GetLoadHandler() OVERRIDE {
		return this;
	}

	virtual CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() OVERRIDE {
		return this;
	}

	virtual CefRefPtr<CefDownloadHandler> GetDownloadHandler() OVERRIDE {
		return this;
	}
	
	virtual CefRefPtr<CefRequestHandler> GetRequestHandler() OVERRIDE {
		return this;
	}
  
	// CefDisplayHandler methods:
	virtual void OnTitleChange(CefRefPtr<CefBrowser> browser,
								const CefString& title) OVERRIDE;

	// CefLifeSpanHandler methods:
	virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;
	virtual bool DoClose(CefRefPtr<CefBrowser> browser) OVERRIDE;
	virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) OVERRIDE;
	virtual bool OnBeforePopup(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& target_url, const CefString& target_frame_name,
								CefLifeSpanHandler::WindowOpenDisposition target_disposition, bool user_gesture, const CefPopupFeatures& popupFeatures, 
								CefWindowInfo& windowInfo, CefRefPtr<CefClient>& client, CefBrowserSettings& settings, bool* no_javascript_access) OVERRIDE;

	// CefLoadHandler methods:
	virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         int httpStatusCode) OVERRIDE;

	virtual void OnLoadError(CefRefPtr<CefBrowser> browser,
							CefRefPtr<CefFrame> frame,
							ErrorCode errorCode,
							const CefString& errorText,
							const CefString& failedUrl) OVERRIDE;

	// CefKeyboardHandler methods:
  
	// Called before a keyboard event is sent to the renderer. |event| contains
	// information about the keyboard event. |os_event| is the operating system
	// event message, if any. Return true if the event was handled or false
	// otherwise. If the event will be handled in OnKeyEvent() as a keyboard
	// shortcut set |is_keyboard_shortcut| to true and return false.
	///
	virtual bool OnPreKeyEvent(CefRefPtr<CefBrowser> browser,
								const CefKeyEvent& event,
								CefEventHandle os_event,
								bool* is_keyboard_shortcut) OVERRIDE; 

	///
	// Called after the renderer and JavaScript in the page has had a chance to
	// handle the event. |event| contains information about the keyboard event.
	// |os_event| is the operating system event message, if any. Return true if
	// the keyboard event was handled or false otherwise.
	///
	virtual bool OnKeyEvent(CefRefPtr<CefBrowser> browser,
							const CefKeyEvent& event,
							CefEventHandle os_event) OVERRIDE; 
  
	// CefDownloadHandler methods
	virtual void OnBeforeDownload(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item,
		const CefString& suggested_name, CefRefPtr<CefBeforeDownloadCallback> callback) OVERRIDE;

	virtual void OnDownloadUpdated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item,
		CefRefPtr<CefDownloadItemCallback> callback) OVERRIDE;

	// CefRequestHandler methods
	
	// Called on the IO thread before a resource request is loaded. The |request|
	// object may be modified. Return RV_CONTINUE to continue the request
	// immediately. Return RV_CONTINUE_ASYNC and call CefRequestCallback::
	// Continue() at a later time to continue or cancel the request
	// asynchronously. Return RV_CANCEL to cancel the request immediately.
	// 
	virtual ReturnValue OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, 
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefRequest> request,
		CefRefPtr<CefRequestCallback> callback) OVERRIDE;

	///
	// Called on the IO thread when a resource response is received. To allow the
	// resource to load normally return false. To redirect or retry the resource
	// modify |request| (url, headers or post body) and return true. The
	// |response| object cannot be modified in this callback.

	virtual bool OnResourceResponse(CefRefPtr<CefBrowser> browser,
									CefRefPtr<CefFrame> frame,
									CefRefPtr<CefRequest> request,
									CefRefPtr<CefResponse> response) OVERRIDE;

	// Request that all existing browser windows close.
	void CloseAllBrowsers(bool force_close);

	bool IsClosing() const { return is_closing_; }
    
	CefRefPtr<CefBrowser> GetBrowserOnTab();
	bool IsExistingBrowser() const { return !browser_map_.empty(); } 
 private:
	 typedef std::map<int, CefRefPtr<CefBrowser> > BrowserMap;
	// Map of existing browser windows. Only accessed on the CEF UI thread.
	BrowserMap browser_map_;
	bool is_closing_;

	bool first_init;
	
	// Include the default reference counting implementation.
	IMPLEMENT_REFCOUNTING(SimpleHandler);
};

class Visitor : public CefStringVisitor 
{
	CefString url;
public:
	explicit Visitor(CefRefPtr<CefBrowser> browser, CefString url) : browser_(browser), url(url) {}
	virtual void Visit(const CefString& string) OVERRIDE;
private:
	CefRefPtr<CefBrowser> browser_;
	IMPLEMENT_REFCOUNTING(Visitor);
};