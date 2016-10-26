#pragma once

/////////////////////////////////////////////////////////////////////////////
// Name:        xy_recorder.cpp
// Purpose:     Record and Play mouse/key actions on Web sites
// Author:      Alexey Ivanov
// Modified by:
// Created:     17/06/2016
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "wx/treectrl.h"
#include "wx/notebook.h"
#include "wx/cmdline.h"
#include "wx/wfstream.h"
#include "wx/url.h"

#define USE_GENERIC_TREECTRL 0

#if USE_GENERIC_TREECTRL
#include "wx/generic/treectlg.h"
#ifndef wxTreeCtrl
#define wxTreeCtrl wxGenericTreeCtrl
#define sm_classwxTreeCtrl sm_classwxGenericTreeCtrl
#endif
#endif

#include <list>

// Include CEF

#include "cef_app.h"
#include "cef_handler.h"

// Include rapidjson

#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/document.h"
#include "rapidjson/error/error.h"
#include "rapidjson/error/en.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/schema.h"

// ----------------------------------------------------------------------------
// resources
// ----------------------------------------------------------------------------

#include "blue.xpm"
#include "green.xpm"
#include "red.xpm"


// the application icon (under Windows it is in resources and even
// though we could still include the XPM here it would be unused)
#ifndef wxHAS_IMAGES_IN_RESOURCES
    #include "../sample.xpm"
#endif

// Forward classes
class MainFrame;
class ActionsManager;
class WebsocketThread;

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// Define a new application type, each program should derive a class from wxApp

class WevebApp : public wxApp
{
public:
	bool InitCefLibrary();

    // override base class virtuals
    // ----------------------------

    // this one is called on application startup and is a good place for the app
    // initialization (doing it here and not in the ctor allows to have an error
    // return: if OnInit() returns false, the application terminates)
    virtual bool OnInit() wxOVERRIDE;
	virtual void OnInitCmdLine(wxCmdLineParser &parser) wxOVERRIDE;
	virtual bool OnCmdLineParsed(wxCmdLineParser &parser) wxOVERRIDE;
	virtual int OnExit() wxOVERRIDE;

	MainFrame* GetMainFrame() { return main_frame; }
	
	CefRefPtr<MyCefApp> app;
	ActionsManager& GetActionsManager() { return *actions_manager; } 

	wxString GetLoadedJson() { return loaded_json; }
	bool IsHooked() { return is_hooked; }
	void StartHook() { is_hooked = true; }
	void StopHook() { is_hooked = false; }

	int GetPort() { return static_cast<int>(port); }
	wxString GetStartUrl() { return start_url; }
private:
	MainFrame* main_frame;	
	ActionsManager* actions_manager;

	wxString loaded_json;
	bool is_hooked;
	long port;
	wxString start_url;
};

class ContentEvent;

// Define a new frame type: this is going to be our main frame
class MainFrame : public wxFrame
{
public:
    // ctor(s)
    MainFrame(const wxString& title);

    // event handlers (these functions should _not_ be virtual)
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
	void OnChangePath(wxCommandEvent& event);	
	void OnSize(wxSizeEvent& event);
	void OnKeyPress(wxKeyEvent& event);
	void OnClose(wxCloseEvent& event);
	
	void OnRightClick(wxMouseEvent& event);
	void InitHooks(wxThreadEvent& event);

	void AddResourceWorker(wxThreadEvent& event);
	void AddContentWorker(wxThreadEvent& event);
	void ParseWebSockCmd(wxThreadEvent& event);

	// Buttons
	void OnRecordScript(wxCommandEvent& event);
	void OnSaveToFile(wxCommandEvent& event);
	void OnLoadFromFile(wxCommandEvent& event);
	void OnPlay(wxCommandEvent& event);	

	void SetCefThread(DWORD cef_thread);

	std::string GetPathForSaving(const std::string& filename);
	const std::string& GetGuiPath() { return gui_path; }
	void TrackingJSMouse(int xPos, int yPos);

	void InitWebsocketServer();
	void OpenConnect();
	void CloseConnect();

	// Tab Mutex and Cond
	wxMutex& GetTabMutex() { return tab_mutex; }
	wxCondition& GetTabCond() { return tab_cond; }
	
	wxPanel *m_browser_panel;
	
	std::string user_agent;
private:
	wxTreeCtrl *m_Tree;	
	HWND hWndBrowser;

	void PlayEvents();
	bool LoadJsonFile(wxString path);
	void Resize();

	wxMutex tab_mutex;
	wxCondition tab_cond;

	std::string default_path;	// for saving screenshots/contents
	std::string gui_path;		// path to GUI
	
	DWORD cef_thread_id;

	WebsocketThread* web_sock_thread;

	ContentEvent* save_content;

    // any class wishing to process wxWidgets events must use this macro
    wxDECLARE_EVENT_TABLE();
};

// A custom status bar which contains icons, timer
class MyStatusBar : public wxStatusBar
{
public:
    MyStatusBar(wxWindow *parent, long style = wxSTB_DEFAULT_STYLE);
    virtual ~MyStatusBar();

    void UpdateClock();

	void StartPlay();
	void StopPlay();

	void SetGreenIcon();
	void SetRedIcon();

    // event handlers
    void OnTimer(wxTimerEvent& WXUNUSED(event)) { UpdateClock(); }
    void OnSize(wxSizeEvent& event);

private:
	enum
    {
        Field_Text,
        Field_Bitmap_conn,
		Field_Bitmap_play,
        Field_Clock,
		Field_Max
    };
    wxTimer m_timer;
    wxStaticBitmap *m_statbmp_conn;
	wxStaticBitmap *m_statbmp_play;
	wxTimeSpan m_seconds;

    wxDECLARE_EVENT_TABLE();
};

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

static const int BITMAP_SIZE_X = 15;

// IDs for the controls and the menu commands
enum
{
    // menu items
    Minimal_Quit = wxID_EXIT,

    // it is important for the id corresponding to the "About" command to have
    // this standard value as otherwise it won't be handled properly under Mac
    // (where it is special and put into the "Apple" menu)
    Minimal_About = wxID_ABOUT,

	RecordScript = wxID_HIGHEST,
	SaveToFile,
	LoadFromFile,
	Play,
	Screenshot,
	WorkerAddTab,
	CloseTab,
	WorkerResourceEvent,
	WorkerContentEvent,
	ChangePath,
	InitHook,
	WorkerWebSockCmd
};

class Action
{	
protected:
	wxLongLong delay;
	wxLongLong CalculateDelay();
public:
	Action(wxLongLong delay) {
		if (delay == 0)
			this->delay = CalculateDelay();
		else
			this->delay = delay;
	}

	virtual ~Action() {}
	virtual void Execute() = 0;
	virtual void GetJsonObject(rapidjson::Value& item, rapidjson::Document::AllocatorType& alloc) = 0;

	wxLongLong GetDelay() { return delay;}
};

class ClickAction: public Action
{
	int x, y;
	WPARAM wp;

	bool EventToPopup();
public:
	ClickAction(int xPos, int yPos, WPARAM wp, wxLongLong delay): 
		Action(delay), x(xPos), y(yPos), wp(wp) {}
	~ClickAction() {}

	virtual void Execute() override;
	virtual void GetJsonObject(rapidjson::Value& item, rapidjson::Document::AllocatorType& alloc) override;
};

class MouseMoveAction: public Action
{
protected:
	int x, y;
	WPARAM wp;
public:
	MouseMoveAction(int xPos, int yPos, WPARAM wp, wxLongLong delay): 
		Action(delay), x(xPos), y(yPos), wp(wp) {}
	~MouseMoveAction() override {}

	virtual void Execute() override;
	virtual void GetJsonObject(rapidjson::Value& item, rapidjson::Document::AllocatorType& alloc) override;
};

class MouseWheelAction: public MouseMoveAction
{
	int delta;
public:
	MouseWheelAction(MouseMoveAction& m_a, int delta): MouseMoveAction(m_a), delta(delta) {}
	~MouseWheelAction() override {}

	virtual void Execute() override;
	virtual void GetJsonObject(rapidjson::Value& item, rapidjson::Document::AllocatorType& alloc) override;
};

class TypeAction: public Action
{
	WPARAM wp;
	LPARAM lp;
	int ch;
public:
	TypeAction(WPARAM wp, LPARAM lp, int ch, wxLongLong delay): 
		Action(delay), wp(wp), lp(lp), ch(ch) {}
	~TypeAction() override {}

	virtual void Execute() override;
	virtual void GetJsonObject(rapidjson::Value& item, rapidjson::Document::AllocatorType& alloc) override;
};

class ResizeAction: public Action
{
	int width;
	int height;
public:
	ResizeAction(int width, int height, wxLongLong delay): 
		Action(delay), width(width), height(height) {}
	~ResizeAction() override {}

	virtual void Execute() override;
	virtual void GetJsonObject(rapidjson::Value& item, rapidjson::Document::AllocatorType& alloc) override;
};

class Event
{
protected:
	wxLongLong timestamp;		
public:
	Event(wxLongLong ts): timestamp(ts) {}
	virtual ~Event() {}
	virtual void GetJsonObject(rapidjson::Value& item, rapidjson::Document::AllocatorType& alloc) = 0;

	wxLongLong GetTimestamp() { return timestamp;} 
};

class ScreenshotEvent: public Event
{
	std::string filename;
public:
	ScreenshotEvent(std::string fn, wxLongLong ts): 
		Event(ts), filename(fn) {}
	~ScreenshotEvent() override {}

	virtual void GetJsonObject(rapidjson::Value& item, rapidjson::Document::AllocatorType& alloc) override;
};

class HTTPResourceEvent: public Event
{	
public:
	HTTPResourceEvent(wxLongLong ts = wxGetLocalTimeMillis()): Event(ts), request_resource_type(-1) {}

	// Request
	std::string request_url;
	std::string request_method;
	int request_resource_type;
	int request_id;
	std::list<std::pair<std::string, std::string>> request_header_map;

	// Response
	std::string response_mime_type;
	int response_status;
	std::string response_status_text;
	std::list<std::pair<std::string, std::string>> response_header_map;
	
	virtual void GetJsonObject(rapidjson::Value& item, rapidjson::Document::AllocatorType& alloc) override;
	std::string ResourceTypeToStr();
};

class ContentEvent: public Event
{
	std::string content;
	std::string url;
	std::string saved_filename;
public:
	ContentEvent(const std::string &content = "", const std::string& url = "", wxLongLong ts = wxGetLocalTimeMillis())
		: Event(ts), content(content), url(url) {}

	ContentEvent& operator=(ContentEvent& r)
	{
		this->content = r.content;
		this->url = r.url;
		this->saved_filename = r.saved_filename;
		this->timestamp = r.timestamp;		
		return *this;
	}

	bool SaveToFile();
	void Release() { content.clear(); url.clear(); } 
	virtual void GetJsonObject(rapidjson::Value& item, rapidjson::Document::AllocatorType& alloc) override;
};


class StartUrlEvent: public Event
{
	wxString url;
public:
	StartUrlEvent(const std::string& url, wxLongLong ts = wxGetLocalTimeMillis()): Event(ts), url(url)  {}
	~StartUrlEvent() override {}
	virtual void GetJsonObject(rapidjson::Value& item, rapidjson::Document::AllocatorType& alloc) override;
};

class JSClickEvent : public Event
{
	int client_x;
	int client_y;
	std::string result;
public:
	JSClickEvent(int client_x, int client_y, const std::string& result, wxLongLong ts = wxGetLocalTimeMillis()):
		Event(ts), client_x(client_x), client_y(client_y), result(result) {};
	~JSClickEvent() override {}
	virtual void GetJsonObject(rapidjson::Value& item, rapidjson::Document::AllocatorType& alloc) override;
};

class JSFocusEvent : public Event
{
	std::string result;
public:
	JSFocusEvent(const std::string& result, wxLongLong ts = wxGetLocalTimeMillis()) : Event(ts), result(result) { }
	~JSFocusEvent() override {}
	virtual void GetJsonObject(rapidjson::Value& item, rapidjson::Document::AllocatorType& alloc) override;	
};

class ActionsManager
{
public:
	enum state_t
	{
		START_RECORD,
		STOP_RECORD
	};

	ActionsManager(): tree_ctrl(NULL), state(STOP_RECORD), condition(mutex) {}

	state_t GetState() const { return state; }
	wxLongLong GetStartTime() const { return start_timestamp; }
//	const std::list<Action*>& GetActions() const { return actions; }
	const std::string& GetStartUrl() const { return start_url; }


	bool GenerateJsonDocument(rapidjson::Document& doc);
	bool LoadJsonDocument(rapidjson::Document& doc);

	void StartRecord();
	void StopRecord();

	void PauseRecord();
	void ResumeRecord();

	void Play();

	// Action pushers
	void PushClickAction(int xPos, int yPos, WPARAM wp, wxLongLong delay = 0);
	void PushMouseMoveAction(int xPos, int yPos, WPARAM wp, wxLongLong delay = 0);
	void PushMouseWheelAction(int xPos, int yPos, WPARAM wp, int delta, wxLongLong delay = 0);
	void PushTypeAction(WPARAM wp, LPARAM lp, int ch = 0, wxLongLong delay = 0);
	void PushResizeAction(int width, int height, wxLongLong delay = 0);
	void PushStartUrl();
	void PushScreenshot(wxLongLong ts, std::string path);
	void PushResourceEvent(HTTPResourceEvent* resource);
	void PushContentEvent(ContentEvent* content);

	void BindTreeCtrl(wxTreeCtrl* tr);
	
	void SendJson(Action& action);
	void SendJson(Event& event);

	wxMutex& GetMutex() { return mutex; }
	wxCondition& GetCondition() { return condition; }

	void SetStartUrl(const std::string& url) { start_url = url; }
private:
	friend Action;

	void DoDelay(Action* action);
	void DeleteActions();	

	// current list recorded actions
	std::list<Action*> actions;

	// current list recorded actions
	std::list<Event*> events;
	wxTreeCtrl* tree_ctrl;
	state_t state;

	wxLongLong start_timestamp;
	wxLongLong prev_action_timestamp;

	std::string start_url;

	// for wait load start url
	wxMutex mutex;
	wxCondition condition;
};


class ReplayingThread: public wxThread
{
public:
	ReplayingThread(): wxThread(wxTHREAD_DETACHED) {}

protected:
	virtual ExitCode Entry();
};

class WebsocketThread: public wxThread
{
public:
	WebsocketThread() {}
	~WebsocketThread() {}
protected:
	virtual ExitCode Entry();
};
