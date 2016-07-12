#include "cef_handler.h"
#include <iostream>
#include <sstream>
#include <string>

#include "include/base/cef_bind.h"
#include "include/cef_app.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

#include <windows.h>
#include <shlobj.h>

#include "web_event_browser.h"

wxDECLARE_APP(WevebApp);

namespace {

SimpleHandler* g_instance = NULL;

}  // namespace

SimpleHandler::SimpleHandler()
    : is_closing_(false), first_init(false) {
  DCHECK(!g_instance);
  g_instance = this;
}

SimpleHandler::~SimpleHandler() {
  g_instance = NULL;
}

// static
SimpleHandler* SimpleHandler::GetInstance() {
  return g_instance;
}

void SimpleHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) 
{
	CEF_REQUIRE_UI_THREAD();

	int sel_tab = wxGetApp().GetMainFrame()->m_BookBrowser->GetSelection();
	// Add to the map of existing browsers.
	browser_map_.insert(std::pair<int, CefRefPtr<CefBrowser> > (sel_tab, browser));
	
	if (!first_init)
	{
		wxGetApp().GetMainFrame()->SetCefThread(GetCurrentThreadId());
		wxThreadEvent evt(wxEVT_THREAD, InitHook);
		wxQueueEvent(wxGetApp().GetMainFrame(), evt.Clone());
		first_init = true;
	}

}

bool SimpleHandler::DoClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  // Closing the main window requires special handling. See the DoClose()
  // documentation in the CEF header for a detailed destription of this
  // process.
  if (browser_map_.size() == 1) {
    // Set a flag to indicate that the window close should be allowed.
    is_closing_ = true;
  }

  // Allow the close. For windowed browsers this will result in the OS close
  // event being sent.
  return false;
}

void SimpleHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) 
{
	CEF_REQUIRE_UI_THREAD();

	// Remove from the list of existing browsers.
	BrowserMap::iterator bit = browser_map_.begin();
	for (; bit != browser_map_.end(); ++bit)
	{
		if ((bit->second)->IsSame(browser)) 
		{
			browser_map_.erase(bit);
			break;
		}
	}

	if (browser_map_.empty()) 
	{
		// All browser windows have closed. Quit the application message loop.
		//CefQuitMessageLoop();
	}
}

bool SimpleHandler::OnBeforePopup(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& target_url, 
								  const CefString& target_frame_name, CefLifeSpanHandler::WindowOpenDisposition target_disposition, bool user_gesture, 
								  const CefPopupFeatures& popupFeatures,  CefWindowInfo& windowInfo, CefRefPtr<CefClient>& client, 
								  CefBrowserSettings& settings, bool* no_javascript_access)
{

	/*
	1) Thread task for new Tab
	2) Wait
	3) Get tab pointer 
	*/
	MainFrame* my_frame = wxGetApp().GetMainFrame();
	assert(my_frame != NULL);

	my_frame->GetTabMutex().Lock();

	wxThreadEvent evt(wxEVT_THREAD, WorkerAddTab);	
	wxQueueEvent(wxGetApp().GetMainFrame(), evt.Clone());

	if (my_frame->GetTabCond().WaitTimeout(10 * 1000) == wxCOND_TIMEOUT)
	{
		my_frame->SetStatusText(_("Error: Timeout of 10s exceeded for load url on new tab"));
		return true;
	}

	wxNotebook* m_BookBrowser = wxGetApp().GetMainFrame()->m_BookBrowser;
	wxPanel* new_tab = (wxPanel*)m_BookBrowser->GetPage(m_BookBrowser->GetPageCount() - 1);	//get new page (last)

	assert(new_tab);
	
	wxSize tab_size = new_tab->GetSize();
	RECT rect;
	rect.left = rect.top = 0;
	rect.bottom = tab_size.GetHeight();
	rect.right = tab_size.GetWidth();
	
	windowInfo.SetAsPopup(NULL, "xyrec_cef");
	windowInfo.SetAsChild(new_tab->GetHandle(), rect);
	
//	browser->GetMainFrame()->LoadURL(target_url);
	return false;
}

void SimpleHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode)
{
	CEF_REQUIRE_UI_THREAD();

	std::cout << "OnLoadEnd(): " << frame->GetURL().ToString() << std::endl;	

//	if (frame->GetURL().ToString() == wxGetApp().GetActionsManager().GetStartUrl())
	{
//		wxMutexLocker lock(wxGetApp().GetActionsManager().GetMutex());
//		wxGetApp().GetActionsManager().GetCondition().Signal(); // same as Signal() here -- one waiter only
	}

	if (frame->IsMain())
	{
		wxTextCtrl* url_ctrl = wxGetApp().GetMainFrame()->url_ctrl;
		if (url_ctrl != NULL)
		{
			wxGetApp().GetMainFrame()->url_ctrl->Clear();
			wxGetApp().GetMainFrame()->url_ctrl->AppendText(frame->GetURL().ToString());
			
			if ( wxGetApp().GetActionsManager().GetState() == ActionsManager::START_RECORD)
				frame->GetSource(new Visitor(browser, frame->GetURL()));			
		}
	}
}

void SimpleHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString& errorText,
                                const CefString& failedUrl) {
  CEF_REQUIRE_UI_THREAD();

  // Don't display an error for downloaded files.
  if (errorCode == ERR_ABORTED)
    return;

  // Display a load error message.
  std::stringstream ss;
  ss << "<html><body bgcolor=\"white\">"
        "<h2>Failed to load URL " << std::string(failedUrl) <<
        " with error " << std::string(errorText) << " (" << errorCode <<
        ").</h2></body></html>";
  frame->LoadString(ss.str(), failedUrl);
}

void SimpleHandler::CloseAllBrowsers(bool force_close) 
{
	if (!CefCurrentlyOn(TID_UI)) 
	{
		// Execute on the UI thread.
		CefPostTask(TID_UI, base::Bind(&SimpleHandler::CloseAllBrowsers, this, force_close));
		return;
	}

	if (browser_map_.empty())
		return;

	BrowserMap::const_iterator it = browser_map_.begin();
	for (; it != browser_map_.end(); ++it)
		(it->second)->GetHost()->CloseBrowser(force_close);
}

void SimpleHandler::OnTitleChange(CefRefPtr<CefBrowser> browser,
                                  const CefString& title) {
	CEF_REQUIRE_UI_THREAD();

	CefWindowHandle hwnd = browser->GetHost()->GetWindowHandle();
	SetWindowText(hwnd, std::wstring(title).c_str());
}

bool SimpleHandler::OnPreKeyEvent(CefRefPtr<CefBrowser> browser,
                             const CefKeyEvent& event,
                             CefEventHandle os_event,
                             bool* is_keyboard_shortcut) 
{
	return false;
}

bool SimpleHandler::OnKeyEvent(CefRefPtr<CefBrowser> browser,
                        const CefKeyEvent& event,
                        CefEventHandle os_event)
{
//	std::cout << "key press:" << event.character << " '" << char(event.character) << "'" << std::endl;
	return false;
}
  
  CefRefPtr<CefBrowser> SimpleHandler::GetBrowserOnTab()
  {
	  // wait first initialize
	  while (!first_init) {};

	  int sel_tab = wxGetApp().GetMainFrame()->m_BookBrowser->GetSelection();
	  assert(sel_tab != wxNOT_FOUND);
	  
	  auto it = browser_map_.find(sel_tab);

	  assert(it != browser_map_.end());
	  return it->second;
  }

void SimpleHandler::OnBeforeDownload(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item,
	const CefString& suggested_name, CefRefPtr<CefBeforeDownloadCallback> callback)
{
	CEF_REQUIRE_UI_THREAD();

	// GetDownloadPath
	std::string file_name = suggested_name;
	TCHAR szFolderPath[MAX_PATH];
	std::string path;

	// Save the file in the user's "My Documents" folder.
	if (SHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, 0, szFolderPath) >= 0)
	{
		path = CefString(szFolderPath);
		path += "\\" + file_name;
	}

	callback->Continue(path, false);
}

void SimpleHandler::OnDownloadUpdated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item,
	CefRefPtr<CefDownloadItemCallback> callback)
{
	CEF_REQUIRE_UI_THREAD();

	if (download_item->IsInProgress())
	{
		wxString str;
		str.Printf("Downloading: %d%%", download_item->GetPercentComplete());
		wxGetApp().GetMainFrame()->SetStatusText(str);
	}
	else if (download_item->IsComplete()) 
	{
		wxGetApp().GetMainFrame()->SetStatusText("File \"" + download_item->GetFullPath().ToString() + "\"downloaded successfully.");
		
		//test_runner::Alert(browser, "File \"" + download_item->GetFullPath().ToString() + "\" downloaded successfully.");
	}
}

CefRequestHandler::ReturnValue SimpleHandler::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, 
																   CefRefPtr<CefRequest> request, CefRefPtr<CefRequestCallback> callback)
{
	return RV_CONTINUE;
}

bool SimpleHandler::OnResourceResponse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, 
									   CefRefPtr<CefRequest> request, CefRefPtr<CefResponse> response) 
{
	if ( wxGetApp().GetActionsManager().GetState() != ActionsManager::START_RECORD)
		return false;

	HTTPResourceEvent* resource = new HTTPResourceEvent;
	// Request
	resource->request_id = request->GetIdentifier();
	resource->request_method = request->GetMethod().ToString();
	resource->request_resource_type = request->GetResourceType();
	resource->request_url = request->GetURL().ToString();

	CefRequest::HeaderMap headerMapReq;
	request->GetHeaderMap(headerMapReq);

	for(auto it :headerMapReq)
	{
		resource->request_header_map.push_back(make_pair(it.first.ToString(), it.second.ToString()));
	}
	
	// Response
	resource->response_mime_type = response->GetMimeType().ToString();
	resource->response_status = response->GetStatus();
	resource->response_status_text = response->GetStatusText().ToString();
	
	CefRefPtr<CefPostData> post_data = request->GetPostData();
	
	CefResponse::HeaderMap headerMapRes;
	response->GetHeaderMap(headerMapRes);
	
	for(auto it :headerMapRes)
	{
		resource->response_header_map.push_back(make_pair(it.first.ToString(), it.second.ToString()));
	}

	/*
	1) Thread task for new Resource Event
	2) Wait
	3) Get tab pointer 
	*/
	
	MainFrame* my_frame = wxGetApp().GetMainFrame();
	assert(my_frame != NULL);

	wxThreadEvent evt(wxEVT_THREAD, WorkerResourceEvent);		
	evt.SetPayload(resource);

	wxQueueEvent(wxGetApp().GetMainFrame(), evt.Clone());

	return false;
}

void Visitor::Visit(const CefString& string)
{
	MainFrame* my_frame = wxGetApp().GetMainFrame();
	assert(my_frame != NULL);

	wxThreadEvent evt(wxEVT_THREAD, WorkerContentEvent);
	ContentEvent* content = new ContentEvent(string.ToString(), url.ToString());
	evt.SetPayload(content);

	wxQueueEvent(wxGetApp().GetMainFrame(), evt.Clone());
}