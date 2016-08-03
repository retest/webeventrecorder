
#include "websock_srv.h"
#include "web_event_browser.h"
#include <iostream>
#include <shlobj.h>

// ----------------------------------------------------------------------------
// event tables and other macros for wxWidgets
// ----------------------------------------------------------------------------

// the event tables connect the wxWidgets events with the functions (event
// handlers) which process them. It can be also done at run-time, but for the
// simple menu events like this the static method is much simpler.
wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
	EVT_SIZE(MainFrame::OnSize)
    EVT_MENU(Minimal_Quit,  MainFrame::OnQuit)
    EVT_MENU(Minimal_About, MainFrame::OnAbout)
	EVT_MENU(ChangePath, MainFrame::OnChangePath)
	EVT_BUTTON(RecordScript, MainFrame::OnRecordScript)
	EVT_BUTTON(SaveToFile, MainFrame::OnSaveToFile)
	EVT_BUTTON(LoadFromFile, MainFrame::OnLoadFromFile)
	EVT_BUTTON(Play, MainFrame::OnPlay)
	EVT_KEY_DOWN(MainFrame::OnKeyPress)
	EVT_THREAD(InitHook, MainFrame::InitHooks)
	EVT_THREAD(WorkerResourceEvent, MainFrame::AddResourceWorker)
	EVT_THREAD(WorkerContentEvent, MainFrame::AddContentWorker)	
	EVT_THREAD(WorkerWebSockCmd, MainFrame::ParseWebSockCmd)
	EVT_CLOSE(MainFrame::OnClose)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(MyStatusBar, wxStatusBar)
    EVT_SIZE(MyStatusBar::OnSize)
    EVT_TIMER(wxID_ANY, MyStatusBar::OnTimer)
wxEND_EVENT_TABLE()

// Create a new application object: this macro will allow wxWidgets to create
// the application object during program execution (it's better than using a
// static object for many reasons) and also implements the accessor function
// wxGetApp() which will return the reference of the right type (i.e. MyApp and
// not wxApp)
wxIMPLEMENT_APP(WevebApp);

extern std::string json_schema;
int CaptureAnImage(HWND hWnd, const char* filename);
void TakeScreenshot();

// ============================================================================
// implementation
// ============================================================================

void CreateConsole()
{
	if (AllocConsole()) 
	{ 
#if 1 /* for Windows 10 and VS 2015*/
		HANDLE stdHandle;
		int hConsole;
		FILE* fp;
		stdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		hConsole = _open_osfhandle((long)stdHandle, _O_TEXT);
		fp = _fdopen(hConsole, "w");

		freopen_s(&fp, "CONOUT$", "w", stdout);
#else
		/* for other OS and VS vers*/
		int hCrt = _open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), 4);
		*stdout = *(::_fdopen(hCrt, "w"));
		::setvbuf(stdout, NULL, _IONBF, 0);
		*stderr = *(::_fdopen(hCrt, "w"));
		::setvbuf(stderr, NULL, _IONBF, 0);
		std::ios::sync_with_stdio();
#endif // _WIN32_WINNT_WIN8 
	}
}

LRESULT CALLBACK MyMouseHook(int nCode, WPARAM wp, LPARAM lp)
{
    MOUSEHOOKSTRUCT *pmh = (MOUSEHOOKSTRUCT *) lp;
		
	if (nCode == 0 && wxGetApp().IsHooked())
	{
		POINT pt = pmh->pt;
		//wxPoint ScreenPos = wxGetApp().GetMainFrame()->GetScreenPosition();
		wxPoint ScreenPos = wxGetApp().GetMainFrame()->m_browser_panel->GetScreenPosition();
//		std::cout << "nCode = " << nCode << ", wParam = " << wp << " ";
		
			
		int xPos = pt.x - ScreenPos.x;
		int yPos = pt.y - ScreenPos.y;

//		std::cout << "Mouse: X = " << xPos << " Y= " << yPos << std::endl;

		//if (wp != WM_MOUSEMOVE && nCode == 3)
		if ((wp == WM_LBUTTONDOWN || wp == WM_LBUTTONUP) && nCode == 0)
		{
			wxGetApp().GetActionsManager().PushClickAction(xPos, yPos, wp);
		}
		else if (wp == WM_MOUSEMOVE && nCode == 0)
		{
			wxGetApp().GetActionsManager().PushMouseMoveAction(xPos, yPos, wp);
		}
		else if (wp == WM_MOUSEWHEEL)
		{
			MOUSEHOOKSTRUCTEX *pmhe = (MOUSEHOOKSTRUCTEX *) lp;
			int delta = GET_WHEEL_DELTA_WPARAM(pmhe->mouseData);

			wxGetApp().GetActionsManager().PushMouseWheelAction(xPos, yPos, wp, delta);
		}
		else
		{
			std::cout << "Unknown event !!!, wp = " << wp << std::endl;
		}
    }
	
    return CallNextHookEx(NULL, nCode, wp, lp);   
}

LRESULT CALLBACK MyKeyboardHook(int nCode, WPARAM wp, LPARAM lp)
{
	if (nCode == 0 && wxGetApp().IsHooked()) 
	{
		std::cout << "nCode = " << nCode << ", wParam = " << wp << ", lParam = " << lp << std::endl;
/*		
		UINT ch = MapVirtualKey(wp, 2);
		if (wxIsascii(ch))
			std::cout << " char = " << ch;
		std::cout << std::endl;
		*/
		
		/*
		if (lp & 0x80000000)
			std::cout << " WM_KEYUP\n";
		else
			std::cout << " WM_DOWN\n";
			*/
		/*
		int WINAPI ToAscii(
  _In_           UINT   uVirtKey,
  _In_           UINT   uScanCode,
  _In_opt_ const BYTE   *lpKeyState,
  _Out_          LPWORD lpChar,
  _In_           UINT   uFlags

);*/

		wxGetApp().GetActionsManager().PushTypeAction(wp, lp);

		if (!(lp & 0x80000000))
		{
			//WM_KEYDOWN -> WM_CHAR			
			// Translate to char

			BYTE lpKeyState[256] ;
			GetKeyboardState(lpKeyState);

			UINT uScanCode = (lp >> 16 ) & 0xFF;
	//		wchar_t Char[256];
			WORD Char;
			int res = ToAscii(wp, uScanCode, lpKeyState, &Char, 0);
			//int res = ToUnicode(wp, uScanCode, lpKeyState, Char, 256, 0);
			if (res < 0)
				std::cout << "is Dead key\n";
			else
			{
				if (res == 0)
					std::cout << "no translated\n";
				else if (res == 1)
				{
					std::cout << "One character was copied to the buffer: "  << char(Char) << std::endl;
					wxGetApp().GetActionsManager().PushTypeAction(wp, lp, char(Char));
				}
				else
					std::cout << "Two characters were copied to the buffer: " << std::endl;
			}		
		}
    }

	return CallNextHookEx(NULL, nCode, wp, lp);   
}


// ----------------------------------------------------------------------------
// the application class
// ----------------------------------------------------------------------------

// 'Main program' equivalent: the program execution "starts" here
bool WevebApp::OnInit()
{
	if (!InitCefLibrary())
		return false;

	is_hooked = false;

	actions_manager = new ActionsManager;

    // call the base class initialization method, currently it only parses a
    // few common command-line options but it could be do more in the future
    if ( !wxApp::OnInit() )
        return false;
		
//	CreateConsole();

    // create the main application window
    main_frame = new MainFrame("WebEventBrowser");
	
    // and show it (the frames, unlike simple controls, are not shown when
    // created initially)
    main_frame->Show(true);	

    // success: wxApp::OnRun() will be called which will enter the main message
    // loop and the application will run. If we returned false here, the
    // application would exit immediately.
    return true;
}

void WevebApp::OnInitCmdLine(wxCmdLineParser &parser)
{
	parser.AddParam(wxEmptyString, wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddOption("p", "port", wxEmptyString, wxCMD_LINE_VAL_NUMBER);
	parser.AddLongOption("url", wxEmptyString, wxCMD_LINE_VAL_STRING);
	parser.Parse();
}

bool WevebApp::OnCmdLineParsed(wxCmdLineParser &parser) 
{
	port = 8000;
	start_url = wxT("google.com");

	parser.Found(wxT("port"), &port);
	parser.Found(wxT("url"), &start_url);

	if (parser.GetParamCount() > 0)
	{
		loaded_json = parser.GetParam(0);
	}
	return true;
}

int WevebApp::OnExit()
{	
	CefShutdown();
//	CefQuitMessageLoop();
	return 0;
}

bool WevebApp::InitCefLibrary()
{
    CefMainArgs main_args;	
    int exit_code = CefExecuteProcess(main_args, NULL, NULL);
    if (exit_code >= 0) 
    {
        // The sub-process has completed so return here.
        return false;
    }

    CefSettings settings;
    settings.multi_threaded_message_loop = true;
    app = new MyCefApp;

    return CefInitialize(main_args, settings, app.get(), NULL);
}

// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

// frame constructor
MainFrame::MainFrame(const wxString& title)
       : wxFrame(NULL, wxID_ANY, title)
	    ,m_Tree(NULL) , m_browser_panel(NULL)
		,hWndBrowser(NULL)		
		,tab_cond(tab_mutex)
		,cef_thread_id(0)
		,web_sock_thread(NULL)
{
    // set the frame icon
    SetIcon(wxICON(sample));

    // create a menu bar
    wxMenu *fileMenu = new wxMenu;

    // the "About" item should be in the help menu
    wxMenu *helpMenu = new wxMenu;
    helpMenu->Append(Minimal_About, "&About\tF1", "Show about dialog");

    fileMenu->Append(ChangePath, "&Set default path\tAlt-G", "Set default path for saving screenshots/contents");
	fileMenu->Append(Minimal_Quit, "E&xit\tAlt-X", "Quit this program");

    // now append the freshly created menu to the menu bar...
	/*
    wxMenuBar *menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, "&File");
    menuBar->Append(helpMenu, "&Help");
	*/
    // ... and attach this menu bar to the frame
    //SetMenuBar(menuBar);

    wxStatusBar *statbarNew = new MyStatusBar(this);
    SetStatusBar(statbarNew);
    SetStatusText("Welcome to WebEventBrowser!");

	Maximize();
	
	wxPanel *m_Panel = new wxPanel(this, wxID_ANY);
	m_Tree = new wxTreeCtrl(m_Panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT );

	m_Tree->AddRoot("Root");

	m_Tree->ExpandAll();

	wxGetApp().GetActionsManager().BindTreeCtrl(m_Tree);
	m_Tree->Hide();
	
	wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);

	m_browser_panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(300, 300));
	sizer->Add(m_browser_panel, 1, wxGROW, 0);
	m_browser_panel->SetMinClientSize(wxSize(750, 500));

	SetSizerAndFit(sizer);

	wxGetApp().app->StartBrowserOnTab(m_browser_panel->GetHandle(), wxGetApp().GetStartUrl().ToStdString());

	//
	wxString loaded_json = wxGetApp().GetLoadedJson();
	if (loaded_json != _(""))
	{
		if (LoadJsonFile(loaded_json))
		{ 
			wxCommandEvent ev(wxEVT_BUTTON, Play);
			AddPendingEvent(ev);
		}	
	}

	CloseConnect();
	InitWebsocketServer();
}

void MainFrame::InitHooks(wxThreadEvent& event)
{
	assert(cef_thread_id != 0);

	HHOOK hook = ::SetWindowsHookEx(WH_MOUSE, MyMouseHook, NULL, cef_thread_id);
	if (hook == NULL)
	{
		wxMessageBox(wxT("Can not set system hook on mouse!"), "Error", wxOK|wxCENTRE);
		wxGetApp().Exit();
	}

	hook = ::SetWindowsHookEx(WH_KEYBOARD, MyKeyboardHook, NULL, cef_thread_id);
	if (hook == NULL)
	{
		wxMessageBox(wxT("Can not set system hook on keyboard!"), "Error", wxOK|wxCENTRE);
		wxGetApp().Exit();
	}
}

// event handlers

void MainFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    // true is to force the frame to close
    Close(true);
}

void MainFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox(wxString::Format
                 (
                    "Welcome to %s!\n"
                    "\n"
                    "This is the minimal sample for testing WebEventBrowser\n"
                    "running under %s.",
                    "WebEventBrowser",
                    wxGetOsDescription()
                 ),
                 "About WebEventBrowser",
                 wxOK | wxICON_INFORMATION,
                 this);
}

void MainFrame::OnChangePath(wxCommandEvent& event)
{
	wxTextEntryDialog dialog(wxGetApp().GetMainFrame(),
							wxT("Please enter a path for saving screenshots/contents\n"),
							wxT("Default saving path"),
							GetPathForSaving(""),
							wxOK | wxCANCEL);

	if (dialog.ShowModal() != wxID_OK)
		return;
	
	default_path = dialog.GetValue().ToStdString();
}

void MainFrame::OnRightClick(wxMouseEvent& event)
{
	wxMenu *menu = new wxMenu();
	menu->Append(CloseTab, "Close tab");
	int x, y;
	x = event.GetX();
	y = event.GetY();

	PopupMenu(menu);
}

void MainFrame::OnSize(wxSizeEvent& event)
{
	if (m_Tree)
		Resize();
	
	if (m_browser_panel != NULL)
	{
		wxSize size = m_browser_panel->GetSize();
		HWND hwnd = FindWindowEx(m_browser_panel->GetHWND(), NULL, NULL, NULL);
		BOOL res = ::SetWindowPos(hwnd, 0, 0, 0, size.x, size.y, SWP_NOZORDER);
	}
	
	wxGetApp().GetActionsManager().PushResizeAction(event.GetSize().GetWidth(), event.GetSize().GetHeight());

	event.Skip();
}

void MainFrame::Resize()
{
	wxSize size = m_Tree->GetParent()->GetClientSize();
    m_Tree->SetSize(0, 0, size.x, size.y);
}

void MainFrame::OnKeyPress(wxKeyEvent& event)
{	
	std::cout << "Key: " << event.GetKeyCode() << " " << char(event.GetKeyCode()) << std::endl;
}

void MainFrame::OnClose(wxCloseEvent& event)
{
	WebSocketSrv::instance().StopServer();

	SimpleHandler::GetInstance()->CloseAllBrowsers(true);
		
	event.Skip();
}

void MainFrame::AddResourceWorker(wxThreadEvent& event)
{
	HTTPResourceEvent* resource = event.GetPayload<HTTPResourceEvent*>();

	wxGetApp().GetActionsManager().PushResourceEvent(resource);
}

void MainFrame::AddContentWorker(wxThreadEvent& event)
{
	ContentEvent* content = event.GetPayload<ContentEvent*>();
	wxGetApp().GetActionsManager().PushContentEvent(content);
}

void MainFrame::ParseWebSockCmd(wxThreadEvent& event)
{
	std::string json_cmd = event.GetPayload<std::string>();
//	std::cout << json_cmd << std::endl;

	ActionsManager& actions_manager = wxGetApp().GetActionsManager();
	
	using namespace rapidjson;

	Document document;
	document.Parse(json_cmd);

	assert(document.IsObject());
	assert(document.HasMember("action"));
	assert(document["action"].IsString());

	std::string action = document["action"].GetString();
	if (action == "set_path")
	{
		default_path = document["dir"].GetString();
	}
	else if (action == "record")
	{
		std::string state = document["state"].GetString();
		if (actions_manager.GetState() == ActionsManager::STOP_RECORD && state == "begin")
		{
			wxGetApp().GetMainFrame()->SetSize(document["width"].GetInt(), document["height"].GetInt());

			SimpleHandler::GetInstance()->GetBrowserOnTab()->GetMainFrame()->LoadURL(document["url"].GetString());

			actions_manager.SetStartUrl(document["url"].GetString());

			SetStatusText(wxT("Start record events..."));

			actions_manager.StartRecord();
		}
		else if (state == "finished")
		{
			SetStatusText(wxT(""));
			actions_manager.StopRecord();
		}
	}
	else if (action == "play")
	{
		if (!LoadJsonFile(document["json"].GetString()))
			return;		

		wxGetApp().GetMainFrame()->SetTitle(_("WebEventBrowser (playing...)"));

		MyStatusBar* status_bar = (MyStatusBar*)GetStatusBar();
		status_bar->StartPlay();

		ActionsManager& actions_manager = wxGetApp().GetActionsManager();

		ReplayingThread *thread = new ReplayingThread();
		thread->Run();
	}
}

void MainFrame::OnRecordScript(wxCommandEvent& event)
{
	wxButton* btn = (wxButton*)event.GetEventObject();
	
	ActionsManager& actions_manager = wxGetApp().GetActionsManager();

	if (actions_manager.GetState() == ActionsManager::STOP_RECORD)
	{

		wxTextEntryDialog dialog(wxGetApp().GetMainFrame(),
								 wxT("Please enter a initial URL\n"),
								 wxT("Initial URL"),
								 wxT("google.com"),
								 wxOK | wxCANCEL);

		if (dialog.ShowModal() != wxID_OK)
			return;
		SimpleHandler::GetInstance()->GetBrowserOnTab()->GetMainFrame()->LoadURL(dialog.GetValue().ToStdString());

		actions_manager.SetStartUrl(dialog.GetValue().ToStdString());

		SetStatusText(wxT("Start record events..."));
		btn->SetLabelText(wxT("Stop Record"));

		actions_manager.StartRecord();	
	}
	else
	{		
		SetStatusText(wxT(""));
		btn->SetLabelText(wxT("Record Script"));
		actions_manager.StopRecord();
	}
}

void MainFrame::OnSaveToFile(wxCommandEvent& event)
{
	// open filedialog to save

	wxFileDialog saveFileDialog(this, wxT("Save WebEventBrowser script"), "", "", "JSON files (*.json)|*.json", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (saveFileDialog.ShowModal() == wxID_CANCEL)        
		return;     // the user changed idea...       

	if (wxGetApp().GetActionsManager().GetState() != ActionsManager::STOP_RECORD)
	{
		SetStatusText(_("Failed to save, please stop play actions"));
		return;
	}
		
	using namespace rapidjson;
	Document doc;
	
	if (!wxGetApp().GetActionsManager().GenerateJsonDocument(doc))
	{
		SetStatusText(_("ERROR: Failed generate json-file..."));
		return;
	}

	FILE* fp = fopen(saveFileDialog.GetPath(), "wb");
	assert(fp != NULL);

	char writeBuffer[65536];
	FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
	Writer<FileWriteStream> writer(os);
	doc.Accept(writer);
	fclose(fp);

	SetStatusText(saveFileDialog.GetPath());
}

void MainFrame::OnLoadFromFile(wxCommandEvent& event)
{
	// open filedialog to load
	
	wxFileDialog openFileDialog(this, wxT("Open WebEventBrowser script"), "", "", "JSON files (*.json)|*.json", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (openFileDialog.ShowModal() == wxID_CANCEL)        
		return;     // the user changed idea...
		
	LoadJsonFile(openFileDialog.GetPath());
}

bool MainFrame::LoadJsonFile(wxString path)
{
	if (wxGetApp().GetActionsManager().GetState() != ActionsManager::STOP_RECORD)
	{
		SetStatusText(_("Error: Failed to open, please stop play actions"));
		return false;
	}
	
	// Validation JSON
	using namespace rapidjson;

	Document sd;
	sd.Parse(json_schema);
	if (sd.HasParseError()) 
	{
		SetStatusText(_("Error: The schema is not a valid JSON"));
		return false;
	}

	SchemaDocument schema(sd);
	
	FILE* fp = fopen(path, "rb");
	assert(fp);
	char readBuffer[65536];
	FileReadStream is(fp, readBuffer, sizeof(readBuffer));
	
	Document doc;
	doc.ParseStream(is);
	if (doc.HasParseError())
	{
		SetStatusText(_("Error: The input is not a valid JSON"));
		return false;
	}
	fclose(fp);

	SchemaValidator validator(schema);
	if (!doc.Accept(validator)) 
	{
		// Input JSON is invalid according to the schema
		// Output diagnostic information
		StringBuffer sb1, sb2;
		validator.GetInvalidSchemaPointer().StringifyUriFragment(sb1);
		validator.GetInvalidDocumentPointer().StringifyUriFragment(sb2);

		wxString str;
		str.Printf("Invalid schema: %s\nInvalid keyword: %s\nInvalid document: %s\n", 
			sb1.GetString(), validator.GetInvalidSchemaKeyword(), sb2.GetString());
		wxMessageBox(str);
	}

	// Parsing json...

	if (!wxGetApp().GetActionsManager().LoadJsonDocument(doc))
	{
		SetStatusText(_("Error: The input is not a valid JSON"));
		return false;
	}

	return true;
}

void MainFrame::OnPlay(wxCommandEvent& event)
{
	wxGetApp().GetMainFrame()->SetTitle(_("WebEventBrowser (playing...)"));

	MyStatusBar* status_bar = (MyStatusBar*)GetStatusBar();
	status_bar->StartPlay();

	ActionsManager& actions_manager = wxGetApp().GetActionsManager();

	ReplayingThread *thread = new ReplayingThread();
	thread->Run();
}

void MainFrame::SetCefThread(DWORD cef_thread)
{
	cef_thread_id = cef_thread;
}

std::string MainFrame::GetPathForSaving(const std::string& filename)
{
	TCHAR szFolderPath[MAX_PATH];
	std::string path = default_path;

	// Save the html content in the user's "My Documents" folder.
	if (SHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, 0, szFolderPath) >= 0)
	{
		if (default_path == "")
			path = CefString(szFolderPath);
		else
			path = default_path;

		path += "\\" + filename;		
	}

	return path;
}

void MainFrame::InitWebsocketServer()
{
	web_sock_thread = new WebsocketThread;
	web_sock_thread->Run();
}

void MainFrame::OpenConnect()
{
	((MyStatusBar*)GetStatusBar())->SetGreenIcon();
}

void MainFrame::CloseConnect()
{
	((MyStatusBar*)GetStatusBar())->SetRedIcon();
}

MyStatusBar::MyStatusBar(wxWindow *parent, long style /* = wxSTB_DEFAULT_STYLE */)
        : wxStatusBar(parent, wxID_ANY, style, "MyStatusBar"), m_timer(this)
{
    // compute the size needed for num lock indicator pane
    wxClientDC dc(this);

    int widths[Field_Max];
    widths[Field_Text] = -1; // growable
	widths[Field_Bitmap_conn] = BITMAP_SIZE_X;
	widths[Field_Bitmap_play] = BITMAP_SIZE_X;
    widths[Field_Clock] = 100;

    SetFieldsCount(Field_Max);
    SetStatusWidths(Field_Max, widths);

	m_statbmp_conn = new wxStaticBitmap(this, wxID_ANY, wxIcon());
	m_statbmp_play = new wxStaticBitmap(this, wxID_ANY, wxIcon());

    SetMinHeight(m_statbmp_conn->GetBestSize().GetHeight());
	SetMinHeight(m_statbmp_play->GetBestSize().GetHeight());

    UpdateClock();
}

MyStatusBar::~MyStatusBar()
{
    if (m_timer.IsRunning())
    {
        m_timer.Stop();
    }
}

void MyStatusBar::OnSize(wxSizeEvent& event)
{
	wxRect rect;
	GetFieldRect(Field_Bitmap_play, rect);
    wxSize size = m_statbmp_play->GetSize();

	m_statbmp_play->Move(rect.x + (rect.width - size.x) / 2,
                    rect.y + (rect.height - size.y) / 2);

	GetFieldRect(Field_Bitmap_conn, rect);
	size = m_statbmp_conn->GetSize();

	m_statbmp_conn->Move(rect.x + (rect.width - size.x) / 2,
                    rect.y + (rect.height - size.y) / 2);

    event.Skip();
}

void MyStatusBar::UpdateClock()
{
	// TODO: timer on seconds
	SetStatusText(m_seconds.Format(), Field_Clock);	
	m_seconds.Add(wxTimeSpan::Second());
}

void MyStatusBar::StartPlay()
{
	SetStatusText("Start Play events!");
	m_statbmp_play->SetIcon(wxIcon(blue_xpm));
	m_seconds = wxTimeSpan::Second();
	m_timer.Start(1000);
}

void MyStatusBar::StopPlay()
{
	m_statbmp_play->SetIcon(wxIcon());
	m_timer.Stop();
}


void MyStatusBar::SetGreenIcon()
{
	m_statbmp_conn->SetIcon(wxIcon(green_xpm));
}

void MyStatusBar::SetRedIcon()
{
	m_statbmp_conn->SetIcon(wxIcon(red_xpm));
}


wxThread::ExitCode ReplayingThread::Entry()
{
	ActionsManager& actions_manager = wxGetApp().GetActionsManager();

	// play actions
	actions_manager.Play();

	MyStatusBar* status_bar = (MyStatusBar*)wxGetApp().GetMainFrame()->GetStatusBar();
	status_bar->StopPlay();

	WebSocketSrv::instance().SendDate("{\"callback\": \"play\", \"state\": \"finished\"}");

	return (wxThread::ExitCode)0;     // success
}

// ----------------------------------------------------------------------------
// WebsocketThread
// ----------------------------------------------------------------------------

wxThread::ExitCode WebsocketThread::Entry()
{
	// init websocket server and run it	
	WebSocketSrv::instance().Run(wxGetApp().GetPort());

	return (wxThread::ExitCode)0;     // success
}


// ----------------------------------------------------------------------------
// ActionManager
// ----------------------------------------------------------------------------

void ActionsManager::StartRecord()
{
	state = START_RECORD;

	DeleteActions();
	
	start_timestamp = wxGetLocalTimeMillis();

	wxGetApp().GetMainFrame()->SendSizeEvent();

	PushStartUrl();

	TakeScreenshot();

	wxGetApp().StartHook();
}

void ActionsManager::StopRecord()
{
	state = STOP_RECORD;
	wxGetApp().StopHook();
	TakeScreenshot();
}

void ActionsManager::BindTreeCtrl(wxTreeCtrl* tr)
{
	assert(tree_ctrl == NULL);
	assert(tr != NULL);
	
	tree_ctrl = tr;
}

void ActionsManager::SendJson(Action& action)
{
	rapidjson::Document doc;
	doc.SetObject();
	rapidjson::Value item(rapidjson::kObjectType);
	action.GetJsonObject(item, doc.GetAllocator());

	doc.AddMember("action", item, doc.GetAllocator());
		
	rapidjson::StringBuffer buffer;
	buffer.Clear();
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);
	
	assert(WebSocketSrv::instance().GetState() == WebSocketSrv::WSOCK_OPEN);
	WebSocketSrv::instance().SendDate(buffer.GetString());
}

void ActionsManager::SendJson(Event& event)
{
	rapidjson::Document doc;
	doc.SetObject();
	rapidjson::Value item(rapidjson::kObjectType);
	event.GetJsonObject(item, doc.GetAllocator());

	doc.AddMember("event", item, doc.GetAllocator());
		
	rapidjson::StringBuffer buffer;
	buffer.Clear();
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);
	
	assert(WebSocketSrv::instance().GetState() == WebSocketSrv::WSOCK_OPEN);
	WebSocketSrv::instance().SendDate(buffer.GetString());
}

void ActionsManager::PushClickAction(int xPos, int yPos, WPARAM wp, wxLongLong ts)
{
	if (state != START_RECORD)
		return;

	if (WebSocketSrv::instance().GetState() == WebSocketSrv::WSOCK_OPEN)
	{
		ClickAction click_action(xPos, yPos, wp, ts);
		SendJson(click_action);	
	}

	actions.push_back(new ClickAction(xPos, yPos, wp, ts));

	// Add in TreeCtrl
	wxTreeItemId root_id = tree_ctrl->GetRootItem();
	assert(root_id.IsOk());

	wxTreeItemId click_id = tree_ctrl->AppendItem(root_id, wxT("CLICK"));	
	wxString txt;
	txt.Printf(wxT("x=%d"), xPos);
	tree_ctrl->AppendItem(click_id, txt);

	txt.Printf(wxT("y=%d"), yPos);
	tree_ctrl->AppendItem(click_id, txt);

	txt.Printf(wxT("wp=%d"), wp);
	tree_ctrl->AppendItem(click_id, txt);
	
	tree_ctrl->Expand(click_id);
	tree_ctrl->EnsureVisible(click_id);	
}

void ActionsManager::PushMouseMoveAction(int xPos, int yPos, WPARAM wp, wxLongLong ts)
{
	if (state != START_RECORD)
		return;

	if (WebSocketSrv::instance().GetState() == WebSocketSrv::WSOCK_OPEN)
	{
		MouseMoveAction move_action(xPos, yPos, wp, ts);
		SendJson(move_action);	
	}

	actions.push_back(new MouseMoveAction(xPos, yPos, wp, ts));
	
	// Add in TreeCtrl
	wxTreeItemId root_id = tree_ctrl->GetRootItem();
	assert(root_id.IsOk());

	//virtual wxTreeItemId wxTreeCtrl::GetLastChild  ( const wxTreeItemId &  item ) const 

	wxTreeItemId last_child = tree_ctrl->GetLastChild(root_id);
	wxTreeItemId item_id;

	wxString str;

	if (last_child.IsOk())
		str = tree_ctrl->GetItemText(last_child);

	if (str == wxT("MOVE"))
	{
		item_id = tree_ctrl->AppendItem(last_child, wxT("item"));
	}
	else
	{
		wxTreeItemId move_id = tree_ctrl->AppendItem(root_id, wxT("MOVE"));
		item_id = tree_ctrl->AppendItem(move_id, wxT("item"));
	}		
			
	wxString txt;
	txt.Printf(wxT("x=%d"), xPos);
	tree_ctrl->AppendItem(item_id, txt);

	txt.Printf(wxT("y=%d"), yPos);
	tree_ctrl->AppendItem(item_id, txt);

	txt.Printf(wxT("wp=%d"), wp);
	tree_ctrl->AppendItem(item_id, txt);
	
//	tree_ctrl->Expand(item_id);
//	tree_ctrl->EnsureVisible(item_id);	
	
}

void ActionsManager::PushMouseWheelAction(int xPos, int yPos, WPARAM wp, int delta, wxLongLong ts)
{
	if (state != START_RECORD)
		return;

	if (WebSocketSrv::instance().GetState() == WebSocketSrv::WSOCK_OPEN)
	{
		MouseWheelAction wheel_action(MouseMoveAction(xPos, yPos, wp, ts), delta);
		SendJson(wheel_action);
	}

	actions.push_back(new MouseWheelAction(MouseMoveAction(xPos, yPos, wp, ts), delta));

	// Add in TreeCtrl
	wxTreeItemId root_id = tree_ctrl->GetRootItem();
	assert(root_id.IsOk());

	wxTreeItemId wheel_id = tree_ctrl->AppendItem(root_id, wxT("WHEEL"));
	wxString txt;
	txt.Printf(wxT("x=%d"), xPos);
	tree_ctrl->AppendItem(wheel_id, txt);

	txt.Printf(wxT("y=%d"), yPos);
	tree_ctrl->AppendItem(wheel_id, txt);

	txt.Printf(wxT("wp=%d"), wp);
	tree_ctrl->AppendItem(wheel_id, txt);

	txt.Printf(wxT("delta=%d"), delta);
	tree_ctrl->AppendItem(wheel_id, txt);

	tree_ctrl->Expand(wheel_id);
	tree_ctrl->EnsureVisible(wheel_id);
}

void ActionsManager::PushTypeAction(WPARAM wp, LPARAM lp, char ch, wxLongLong ts)
{
	if (state != START_RECORD)
		return;

	if (WebSocketSrv::instance().GetState() == WebSocketSrv::WSOCK_OPEN)
	{
		TypeAction type_action(wp, lp, ch, ts);
		SendJson(type_action);
	}

	actions.push_back(new TypeAction(wp, lp, ch, ts));

	// Add in TreeCtrl
	wxTreeItemId root_id = tree_ctrl->GetRootItem();
	assert(root_id.IsOk());

	wxTreeItemId type_id = tree_ctrl->AppendItem(root_id, wxT("TYPE"));
	wxString txt;

	if (ch != 0)
	{		
		txt.Printf(wxT("char=%c"), ch);
		tree_ctrl->AppendItem(type_id, txt);
	}

	txt.Printf(wxT("wp=%lu"), wp);
	tree_ctrl->AppendItem(type_id, txt);

	txt.Printf(wxT("lp=%lu"), lp);
	tree_ctrl->AppendItem(type_id, txt);

	tree_ctrl->Expand(type_id);
	tree_ctrl->EnsureVisible(type_id);	
}

void ActionsManager::PushResizeAction(int width, int height, wxLongLong ts /*= wxGetLocalTimeMillis()*/)
{
	if (state != START_RECORD)
		return;

	if (WebSocketSrv::instance().GetState() == WebSocketSrv::WSOCK_OPEN)
	{
		ResizeAction resize_action(width, height, ts);
		SendJson(resize_action);
	}

	actions.push_back(new ResizeAction(width, height, ts));

	// Add in TreeCtrl
	wxTreeItemId root_id = tree_ctrl->GetRootItem();
	assert(root_id.IsOk());

	wxTreeItemId resize_id = tree_ctrl->AppendItem(root_id, wxT("RESIZE"));
	wxString txt;

	txt.Printf(wxT("width=%lu"), width);
	tree_ctrl->AppendItem(resize_id, txt);

	txt.Printf(wxT("height=%lu"), height);
	tree_ctrl->AppendItem(resize_id, txt);

	tree_ctrl->Expand(resize_id);
	tree_ctrl->EnsureVisible(resize_id);	
}

void ActionsManager::PushScreenshot(wxLongLong ts, std::string path)
{
	if (WebSocketSrv::instance().GetState() == WebSocketSrv::WSOCK_OPEN)
	{
		ScreenshotEvent screenshot_event(path, ts);
		SendJson(screenshot_event);
	}

	events.push_back(new ScreenshotEvent(path, ts));

	// Add in TreeCtrl
	wxTreeItemId root_id = tree_ctrl->GetRootItem();
	assert(root_id.IsOk());

	wxTreeItemId s_shot = tree_ctrl->AppendItem(root_id, wxT("SCREENSHOT"));
	wxString txt;

	txt.Printf(wxT("filename=%s"), path);
	tree_ctrl->AppendItem(s_shot, txt);

//		tree_ctrl->Expand(type_id);
//		tree_ctrl->EnsureVisible(type_id);	
}

void ActionsManager::PushStartUrl()
{
	if (WebSocketSrv::instance().GetState() == WebSocketSrv::WSOCK_OPEN)
	{
		StartUrlEvent start_url_event(start_url, start_timestamp);
		SendJson(start_url_event);
	}

	// Add start URL in TreeCtrl

	wxTreeItemId root_id = tree_ctrl->GetRootItem();
	assert(root_id.IsOk());

	wxTreeItemId url_id = tree_ctrl->AppendItem(root_id, wxT("URL"));	
	tree_ctrl->AppendItem(url_id, start_url);
}

void ActionsManager::PushResourceEvent(HTTPResourceEvent* resource)
{
	if (state != START_RECORD)
	{
		delete resource;
		return;
	}

	if (WebSocketSrv::instance().GetState() == WebSocketSrv::WSOCK_OPEN)
	{
		SendJson(*resource);
	}

	events.push_back(resource);
	
	// Add Resource in TreeCtrl

	wxTreeItemId root_id = tree_ctrl->GetRootItem();
	assert(root_id.IsOk());

	wxTreeItemId resource_id = tree_ctrl->AppendItem(root_id, wxT("RESOURCE"));	

	// Request
	wxTreeItemId req_id = tree_ctrl->AppendItem(resource_id, wxT("REQUEST"));
	
	wxTreeItemId url_id = tree_ctrl->AppendItem(req_id, wxT("URL"));
	tree_ctrl->AppendItem(url_id, resource->request_url);

	wxTreeItemId method_id = tree_ctrl->AppendItem(req_id, wxT("METHOD"));
	tree_ctrl->AppendItem(method_id, resource->request_method);
	
	wxTreeItemId type_id = tree_ctrl->AppendItem(req_id, wxT("TYPE"));
	tree_ctrl->AppendItem(type_id, resource->ResourceTypeToStr());

	wxTreeItemId header_id = tree_ctrl->AppendItem(req_id, wxT("HEADER"));

	for(auto it :resource->request_header_map)
	{
		wxString str;
		str.Printf(wxT("%s: %s"), it.first, it.second);
		wxTreeItemId item_id = tree_ctrl->AppendItem(header_id, str);
	}

	// Response
	wxTreeItemId res_id = tree_ctrl->AppendItem(resource_id, wxT("RESPONSE"));	
	tree_ctrl->AppendItem(res_id, start_url);

	wxTreeItemId mime_id = tree_ctrl->AppendItem(res_id, wxT("MIME"));
	tree_ctrl->AppendItem(mime_id, resource->response_mime_type);
	
	wxTreeItemId status_id = tree_ctrl->AppendItem(res_id, wxT("STATUS"));
	wxString status;
	status.Printf(wxT("%d: %s"), resource->response_status, resource->response_status_text);
	tree_ctrl->AppendItem(status_id, status);

	header_id = tree_ctrl->AppendItem(res_id, wxT("HEADER"));

	for(auto it :resource->response_header_map)
	{
		wxString str;
		str.Printf(wxT("%s: %s"), it.first, it.second);
		wxTreeItemId item_id = tree_ctrl->AppendItem(header_id, str);
	}
}

void ActionsManager::PushContentEvent(ContentEvent* content)
{
	if (state != START_RECORD)
	{
		delete content;
		return;
	}

	if (WebSocketSrv::instance().GetState() == WebSocketSrv::WSOCK_OPEN)
	{
		SendJson(*content);
	}

	if (content->SaveToFile())
		events.push_back(content);

	content->Release();	
}

void ActionsManager::Play()
{
	if (actions.empty())
		return;
	
//	mutex.Lock();

	SimpleHandler::GetInstance()->GetBrowserOnTab()->GetMainFrame()->LoadURL(start_url);
	/*
	if (condition.WaitTimeout(30 * 1000) == wxCOND_TIMEOUT)
	{
		wxGetApp().GetMainFrame()->SetStatusText(_("Error: Timeout of 30s exceeded for load start url"));
		return;
	}
	*/
	time_cursor = start_timestamp;

	for(auto action :actions)
	{
		SimpleHandler::GetInstance()->GetBrowserOnTab()->GetHost()->SetFocus(true);
		action->Execute();
		DoDelay(action);
	}

	wxGetApp().GetMainFrame()->SetTitle(_("WebEventBrowser"));
	wxGetApp().GetMainFrame()->SetStatusText(_(""));
}

void ActionsManager::DoDelay(Action* action)
{
	wxLongLong t = action->GetTimestamp();
	assert(t >= time_cursor);
	wxLongLong msec = t - time_cursor;
	std::cout << (unsigned long)msec.ToLong() << std::endl;	
	wxMilliSleep((unsigned long)msec.ToLong());
	
	time_cursor = t;
}

void ActionsManager::DeleteActions()
{
	for(auto action :actions)
		delete action;
	actions.clear();
	
	wxTreeItemId root = tree_ctrl->GetRootItem();
	if (root.IsOk())
	{
		tree_ctrl->DeleteChildren(root);
	}
}

bool ActionsManager::GenerateJsonDocument(rapidjson::Document& doc)
{
	using namespace rapidjson;
	Document::AllocatorType& alloc = doc.GetAllocator();

	doc.SetObject();

	Value start_time(kNumberType);
	start_time.SetInt64(start_timestamp.GetValue());
	doc.AddMember("start_time", start_time, alloc);	
	doc.AddMember("start_url", start_url, alloc);

	Value json_events(kArrayType);
	
	for(auto ev :events)
	{
		Value item(kObjectType);
		ev->GetJsonObject(item, alloc);
		json_events.PushBack(item, alloc);
	}

	doc.AddMember("events", json_events, doc.GetAllocator());

	Value json_actions(kArrayType);
	
	for(auto action :actions)
	{
		Value item(kObjectType);
		action->GetJsonObject(item, alloc);
		json_actions.PushBack(item, alloc);
	}

	doc.AddMember("actions", json_actions, doc.GetAllocator());

	return true;
}

bool ActionsManager::LoadJsonDocument(rapidjson::Document& doc)
{
	state = START_RECORD;

	DeleteActions();

	using namespace rapidjson;

	Value& start_time = doc["start_time"];
	Value& url = doc["start_url"];
	Value& actions = doc["actions"];

	start_timestamp = start_time.GetInt64();

	SetStartUrl(url.GetString());
	PushStartUrl();

	for(SizeType i = 0; i < actions.Size(); i++)
	{
		Value& action = actions[i];
		assert(action.IsObject());

		if (action["type"] == "move")
		{
			PushMouseMoveAction(action["x"].GetInt64(), action["y"].GetInt64(), action["wp"].GetInt64(), action["time"].GetInt64());
		}
		else if (action["type"] == "click")
		{
			PushClickAction(action["x"].GetInt64(), action["y"].GetInt64(), action["wp"].GetInt64(), action["time"].GetInt64());
		}
		else if (action["type"] == "wheel")
		{
			PushMouseWheelAction(action["x"].GetInt64(), action["y"].GetInt64(), action["wp"].GetInt64(), action["delta"].GetInt64(), action["time"].GetInt64());
		}
		else if (action["type"] == "type")
		{
			PushTypeAction(action["wp"].GetInt64(), action["lp"].GetInt64(), action["ch"].GetInt64(), action["time"].GetInt64());
		}
		else if(action["type"] == "screenshot")
		{
			PushScreenshot(action["time"].GetInt64(), action["file"].GetString());
		}
		else if(action["type"] == "resize")
		{
			PushResizeAction(action["width"].GetInt64(), action["height"].GetInt64(), action["time"].GetInt64());
		}
		else 
			return false;
	}

	state = STOP_RECORD;

	return true;
}

bool isKeyDown(WPARAM wparam) 
{
	return (GetKeyState(wparam) & 0x8000) != 0;
}

int GetCefMouseModifiers(WPARAM wparam) 
{
	int modifiers = 0;
	if (wparam & MK_CONTROL)
		modifiers |= EVENTFLAG_CONTROL_DOWN;
	if (wparam & MK_SHIFT)
		modifiers |= EVENTFLAG_SHIFT_DOWN;
	if (isKeyDown(VK_MENU))
		modifiers |= EVENTFLAG_ALT_DOWN;
	if (wparam & MK_LBUTTON)
		modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
	if (wparam & MK_MBUTTON)
		modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
	if (wparam & MK_RBUTTON)
		modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;

	// Low bit set from GetKeyState indicates "toggled".
	if (::GetKeyState(VK_NUMLOCK) & 1)
		modifiers |= EVENTFLAG_NUM_LOCK_ON;
	if (::GetKeyState(VK_CAPITAL) & 1)
		modifiers |= EVENTFLAG_CAPS_LOCK_ON;
	return modifiers;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	/*
	int WINAPI GetClassName(
  _In_  HWND   hWnd,
  _Out_ LPTSTR lpClassName,
  _In_  int    nMaxCount
);
*/
	TCHAR class_name[256];
	if (GetClassName(hwnd, class_name, 256) != 0)
	{
		if (std::basic_string<TCHAR>(class_name) != _T("Chrome_WidgetWin_0"))
			return TRUE;

		LONG style = GetWindowLong(hwnd, GWL_STYLE);
		if (style & WS_DISABLED)
			return TRUE;
			
		DWORD proc_id;
		::GetWindowThreadProcessId(hwnd, &proc_id);
		std::cout << "ProcID = " << proc_id << std::endl;
		std::cout << "Current ProcID = " << GetCurrentProcessId() << std::endl;

		if (proc_id != GetCurrentProcessId())
			return TRUE;

		HWND child_hwnd = FindWindowEx(hwnd, NULL, NULL, NULL);
		std::cout << "FindWindowEx() = " << child_hwnd << std::endl;
		if (!child_hwnd)
			return TRUE;

		
		RECT rc;
		::GetClientRect(child_hwnd,&rc);
		::MapWindowPoints(child_hwnd, SimpleHandler::GetInstance()->GetBrowserOnTab()->GetHost()->GetWindowHandle(),(LPPOINT)&rc,2);

		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		int _x = x - rc.left;
		int _y = y - rc.top;

		std::cout << "rect.x = " << rc.left << ", rect.y = " << rc.top << std::endl;
		::SendMessage(child_hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(_x, _y));
		::SendMessage(child_hwnd, WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM(_x, _y));

		SetLastError(ERROR_SUCCESS);
		return FALSE;

	}	
	return TRUE;
}

bool ClickAction::EventToPopup()
{		
	BOOL res = EnumWindows(EnumWindowsProc, MAKELPARAM(x,y));
	if (res == FALSE && GetLastError() == ERROR_SUCCESS)
		return true;
	else 
		return false;
}

void ClickAction::Execute()
{
	CefRefPtr<CefBrowser> current_browser = SimpleHandler::GetInstance()->GetBrowserOnTab();

	if (wp == WM_LBUTTONDOWN)
	{
		// the case when the user clicks on a selectbox, 
		// cef creates a window Popup, you need to find it and click on it.
		if (EventToPopup())
			return;		
	}

	// fill Cef arguments
	CefMouseEvent mouse_event;
	mouse_event.x = x;
	mouse_event.y = y;
	mouse_event.modifiers = GetCefMouseModifiers(wp);

	CefBrowserHost::MouseButtonType type;

	if (wp == WM_LBUTTONDOWN || wp == WM_LBUTTONUP)
		type = MBT_LEFT;
	else if (wp == WM_RBUTTONDOWN || wp == WM_RBUTTONUP)
		type = MBT_RIGHT;
	else
		type = MBT_MIDDLE;

	bool mouseUp;
	if (wp == WM_LBUTTONDOWN || wp == WM_RBUTTONDOWN)
		mouseUp = false;
	else
		mouseUp = true;

	int clickCount = 1;

	current_browser->GetHost()->SendMouseClickEvent(mouse_event, type, mouseUp, clickCount);
}

void ClickAction::GetJsonObject(rapidjson::Value& item, rapidjson::Document::AllocatorType& alloc)
{
	rapidjson::Value type(rapidjson::kStringType);
	type.SetString("click", alloc);

	item.AddMember("type", type, alloc);
	item.AddMember("x", x, alloc);
	item.AddMember("y", y, alloc);
	item.AddMember("wp", wp, alloc);
	item.AddMember("time", timestamp.GetValue(), alloc);
}

void MouseMoveAction::Execute()
{
	CefRefPtr<CefBrowser> current_browser = SimpleHandler::GetInstance()->GetBrowserOnTab();

	// fill Cef arguments
	CefMouseEvent mouse_event;
	mouse_event.x = x;
	mouse_event.y = y;
	mouse_event.modifiers = GetCefMouseModifiers(wp);

	current_browser->GetHost()->SendMouseMoveEvent(mouse_event, false);
}

void MouseMoveAction::GetJsonObject(rapidjson::Value& item, rapidjson::Document::AllocatorType& alloc)
{
	rapidjson::Value type(rapidjson::kStringType);
	type.SetString("move", alloc);
	
	item.AddMember("type", type, alloc);
	item.AddMember("x", x, alloc);
	item.AddMember("y", y, alloc);
	item.AddMember("wp", wp, alloc);
	item.AddMember("time", timestamp.GetValue(), alloc);
}

void MouseWheelAction::Execute()
{
	CefRefPtr<CefBrowser> current_browser = SimpleHandler::GetInstance()->GetBrowserOnTab();

	// fill Cef arguments
	CefMouseEvent mouse_event;
	mouse_event.x = x;
	mouse_event.y = y;
	mouse_event.modifiers = GetCefMouseModifiers(wp);

	current_browser->GetHost()->SendMouseWheelEvent(mouse_event, isKeyDown(VK_SHIFT)?delta:0, !isKeyDown(VK_SHIFT)?delta:0);
}

void MouseWheelAction::GetJsonObject(rapidjson::Value& item, rapidjson::Document::AllocatorType& alloc)
{
	rapidjson::Value type(rapidjson::kStringType);
	type.SetString("wheel", alloc);
	
	item.AddMember("type", type, alloc);
	item.AddMember("x", x, alloc);
	item.AddMember("y", y, alloc);
	item.AddMember("wp", wp, alloc);
	item.AddMember("delta", delta, alloc);
	item.AddMember("time", timestamp.GetValue(), alloc);
}

void TypeAction::Execute()
{
	CefRefPtr<CefBrowser> current_browser = SimpleHandler::GetInstance()->GetBrowserOnTab();

	CefKeyEvent key_event;
	key_event.windows_key_code = wp;
	key_event.native_key_code = lp;
	key_event.focus_on_editable_field = true;

	if (ch == 0)
	{
		if (!(lp & 0x80000000))		
			key_event.type = KEYEVENT_KEYUP;			
		else
			key_event.type = KEYEVENT_RAWKEYDOWN;				
	}
	else
	{		
		key_event.type = KEYEVENT_CHAR;
		key_event.windows_key_code = ch;
	}

	current_browser->GetHost()->SendKeyEvent(key_event);
}

void TypeAction::GetJsonObject(rapidjson::Value& item, rapidjson::Document::AllocatorType& alloc)
{
	rapidjson::Value type(rapidjson::kStringType);
	type.SetString("type", alloc);

	item.AddMember("type", type, alloc);
	item.AddMember("wp", wp, alloc);
	item.AddMember("lp", lp, alloc);
	item.AddMember("ch", ch, alloc);
	item.AddMember("time", timestamp.GetValue(), alloc);
}

void ResizeAction::Execute()
{
	wxGetApp().GetMainFrame()->SetSize(width, height);
	wxGetApp().GetMainFrame()->SendSizeEvent();
}

void ResizeAction::GetJsonObject(rapidjson::Value& item, rapidjson::Document::AllocatorType& alloc)
{
	rapidjson::Value type(rapidjson::kStringType);
	type.SetString("resize", alloc);

	item.AddMember("type", type, alloc);
	item.AddMember("width", width, alloc);
	item.AddMember("height", height, alloc);
	item.AddMember("time", timestamp.GetValue(), alloc);
}

void ScreenshotEvent::GetJsonObject(rapidjson::Value& item, rapidjson::Document::AllocatorType& alloc)
{
	rapidjson::Value type(rapidjson::kStringType);
	type.SetString("screenshot", alloc);

	item.AddMember("type", type, alloc);
	item.AddMember("file", filename, alloc);
	item.AddMember("time", timestamp.GetValue(), alloc);
}
/*
void UrlAction::Execute()
{
}*/

std::string HTTPResourceEvent::ResourceTypeToStr() 
{ 
	assert(request_resource_type != -1);
	
	// stringizing resource type
	static char* resource_type[] = { "RT_MAIN_FRAME", "RT_SUB_FRAME", "RT_STYLESHEET", "RT_SCRIPT", "RT_IMAGE", "RT_FONT_RESOURCE", "RT_SUB_RESOURCE", "RT_OBJECT", "RT_MEDIA", "RT_WORKER", "RT_SHARED_WORKER", "RT_PREFETCH", "RT_FAVICON", "RT_XHR", "RT_PING", "RT_SERVICE_WORKER", "RT_CSP_REPORT", "RT_PLUGIN_RESOURCE" };
	
	assert(request_resource_type < sizeof(resource_type)/sizeof(resource_type[0]));

	return resource_type[request_resource_type];
}

void HTTPResourceEvent::GetJsonObject(rapidjson::Value& item, rapidjson::Document::AllocatorType& alloc)
{
	rapidjson::Value type(rapidjson::kStringType);
	type.SetString("resource", alloc);

	rapidjson::Value request(rapidjson::kObjectType);
	rapidjson::Value response(rapidjson::kObjectType);

	// Request
	request.AddMember("url", request_url, alloc);
	request.AddMember("method", request_method, alloc);
	request.AddMember("type", ResourceTypeToStr(), alloc);
	request.AddMember("id", request_id, alloc);

	rapidjson::Value header_req(rapidjson::kObjectType);
	for(auto it :request_header_map)
	{
		rapidjson::Value name(rapidjson::kStringType);
		name.SetString(it.first, alloc);
		header_req.AddMember(name, it.second, alloc);
	}
	request.AddMember("header", header_req, alloc);

	// Response

	response.AddMember("mime", response_mime_type, alloc);
	response.AddMember("status", response_status, alloc);
	response.AddMember("status_text", response_status_text, alloc);
	response.AddMember("status_text", response_status_text, alloc);

	rapidjson::Value header_res(rapidjson::kObjectType);
	for(auto it :response_header_map)
	{
		rapidjson::Value name(rapidjson::kStringType);
		name.SetString(it.first, alloc);
		header_res.AddMember(name, it.second, alloc);
	}
	request.AddMember("header", header_res, alloc);

	item.AddMember("type", type, alloc);
	item.AddMember("request", request, alloc);
	item.AddMember("response", response, alloc);
}

bool ContentEvent::SaveToFile()
{
	wxString filename;	

	wxURL u(url);
	filename.Printf(wxT("%s_%llu"), u.GetServer(), GetTimestamp());

	saved_filename = wxGetApp().GetMainFrame()->GetPathForSaving(filename.ToStdString() + ".html");

	wxFFile f(saved_filename, "w");
	if (f.IsOpened())
	{
		f.Write(content.c_str(), content.length());
		return true;
	}
	else
		wxGetApp().GetMainFrame()->GetStatusBar()->SetStatusText(wxT("Failed open: ") + saved_filename);		
	
	return false;
}

void ContentEvent::GetJsonObject(rapidjson::Value& item, rapidjson::Document::AllocatorType& alloc)
{
	rapidjson::Value type(rapidjson::kStringType);
	type.SetString("content", alloc);

	item.AddMember("type", type, alloc);
	item.AddMember("file", saved_filename, alloc);
}

void StartUrlEvent::GetJsonObject(rapidjson::Value& item, rapidjson::Document::AllocatorType& alloc)
{
	rapidjson::Value type(rapidjson::kStringType);
	type.SetString("init", alloc);

	item.AddMember("type", type, alloc);
	item.AddMember("start_url", url.ToStdString(), alloc);
	item.AddMember("start_time", GetTimestamp().GetValue(), alloc);
}


void TakeScreenshot()
{
	wxLongLong ts = wxGetLocalTimeMillis();

	HWND hWnd = SimpleHandler::GetInstance()->GetBrowserOnTab()->GetHost()->GetWindowHandle();
	
	std::string file_name = ts.ToString() + ".jpeg";	
	std::string path = wxGetApp().GetMainFrame()->GetPathForSaving(file_name);
	
	if (path != "")
	{
		CaptureAnImage(hWnd, path.c_str());
		wxGetApp().GetActionsManager().PushScreenshot(ts, path);
	}		
}

int CaptureAnImage(HWND hWnd, const char* filename)
{
	HDC hdcScreen;
	HDC hdcWindow;
	HDC hdcMemDC = NULL;
	HBITMAP hbmScreen = NULL;
	BITMAP bmpScreen;

	// Retrieve the handle to a display device context for the client 
	// area of the window. 
	hdcScreen = GetDC(NULL);
	hdcWindow = GetDC(hWnd);

	// Create a compatible DC which is used in a BitBlt from the window DC
	hdcMemDC = CreateCompatibleDC(hdcWindow); 

	if(!hdcMemDC)
	{
		MessageBox(hWnd, L"CreateCompatibleDC has failed",L"Failed", MB_OK);
		goto done;
	}

	// Get the client area for size calculation
	RECT rcClient;
	GetClientRect(hWnd, &rcClient);

	//This is the best stretch mode
	SetStretchBltMode(hdcWindow,HALFTONE);

	//The source DC is the entire screen and the destination DC is the current window (HWND)
	if(!StretchBlt(hdcWindow, 
			   0,0, 
			   rcClient.right, rcClient.bottom, 
			   hdcScreen, 
			   0,0,
			   GetSystemMetrics (SM_CXSCREEN),
			   GetSystemMetrics (SM_CYSCREEN),
			   SRCCOPY))
	{
		MessageBox(hWnd, L"StretchBlt has failed",L"Failed", MB_OK);
		goto done;
	}
	
	// Create a compatible bitmap from the Window DC
	hbmScreen = CreateCompatibleBitmap(hdcWindow, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top);
	
	if(!hbmScreen)
	{
		MessageBox(hWnd, L"CreateCompatibleBitmap Failed",L"Failed", MB_OK);
		goto done;
	}

	// Select the compatible bitmap into the compatible memory DC.
	SelectObject(hdcMemDC,hbmScreen);
	
	// Bit block transfer into our compatible memory DC.
	if(!BitBlt(hdcMemDC, 
			   0,0, 
			   rcClient.right-rcClient.left, rcClient.bottom-rcClient.top, 
			   hdcWindow, 
			   0,0,
			   SRCCOPY))
	{
		MessageBox(hWnd, L"BitBlt has failed", L"Failed", MB_OK);
		goto done;
	}

	// Get the BITMAP from the HBITMAP
	GetObject(hbmScreen,sizeof(BITMAP),&bmpScreen);
	 
	BITMAPFILEHEADER   bmfHeader;    
	BITMAPINFOHEADER   bi;
	 
	bi.biSize = sizeof(BITMAPINFOHEADER);    
	bi.biWidth = bmpScreen.bmWidth;    
	bi.biHeight = bmpScreen.bmHeight;  
	bi.biPlanes = 1;    
	bi.biBitCount = 32;    
	bi.biCompression = BI_RGB;    
	bi.biSizeImage = 0;  
	bi.biXPelsPerMeter = 0;    
	bi.biYPelsPerMeter = 0;    
	bi.biClrUsed = 0;    
	bi.biClrImportant = 0;

	DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;

	// Starting with 32-bit Windows, GlobalAlloc and LocalAlloc are implemented as wrapper functions that 
	// call HeapAlloc using a handle to the process's default heap. Therefore, GlobalAlloc and LocalAlloc 
	// have greater overhead than HeapAlloc.
	HANDLE hDIB = GlobalAlloc(GHND,dwBmpSize); 
	char *lpbitmap = (char *)GlobalLock(hDIB);    

	// Gets the "bits" from the bitmap and copies them into a buffer 
	// which is pointed to by lpbitmap.
	GetDIBits(hdcWindow, hbmScreen, 0,
		(UINT)bmpScreen.bmHeight,
		lpbitmap,
		(BITMAPINFO *)&bi, DIB_RGB_COLORS);

	// A file is created, this is where we will save the screen capture.
	HANDLE hFile = CreateFileA(filename,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);
	
	// Add the size of the headers to the size of the bitmap to get the total file size
	DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
 
	//Offset to where the actual bitmap bits start.
	bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER); 
	
	//Size of the file
	bmfHeader.bfSize = dwSizeofDIB; 
	
	//bfType must always be BM for Bitmaps
	bmfHeader.bfType = 0x4D42; //BM   
 
	DWORD dwBytesWritten = 0;
	WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
	WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
	WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);
	
	//Unlock and Free the DIB from the heap
	GlobalUnlock(hDIB);    
	GlobalFree(hDIB);

	//Close the handle for the file that was created
	CloseHandle(hFile);
	   
	//Clean up
done:
	DeleteObject(hbmScreen);
	DeleteObject(hdcMemDC);
	ReleaseDC(NULL,hdcScreen);
	ReleaseDC(hWnd,hdcWindow);

	return 0;
}

