#include "websock_srv.h"

#include "web_event_browser.h"

wxDECLARE_APP(WevebApp);

WebSocketSrv::WebSocketSrv() 
	: state(WSOCK_CLOSE)
{
	try 
	{
		// Set logging settings
		ws_server.set_access_channels(websocketpp::log::alevel::all);
		ws_server.clear_access_channels(websocketpp::log::alevel::frame_payload);

		// Initialize 
		ws_server.init_asio();

		// Register our message handler
		ws_server.set_message_handler(bind(&WebSocketSrv::on_message,&ws_server,::_1,::_2));
		
		// Register open/close connection
		ws_server.set_open_handler(bind(&WebSocketSrv::on_open,&ws_server,::_1));
		ws_server.set_close_handler(bind(&WebSocketSrv::on_close,&ws_server,::_1));
	} 
	catch (websocketpp::exception const & e) 
	{
		std::cout << e.what() << std::endl;
	}
	catch (...) 
	{
		std::cout << "other exception" << std::endl;
	}
}

// Define a callback to handle incoming messages
void WebSocketSrv::on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg) 
{
	wxThreadEvent evt(wxEVT_THREAD, WorkerWebSockCmd);
	evt.SetPayload(msg->get_payload());
	wxQueueEvent(wxGetApp().GetMainFrame(), evt.Clone());	
	return;

	/*
    std::cout << "on_message called with hdl: " << hdl.lock().get()
              << " and message: " << msg->get_payload()
              << std::endl;

    // check for a special command to instruct the server to stop listening so
    // it can be cleanly exited.
    if (msg->get_payload() == "stop-listening") {
        s->stop_listening();
        return;
    }

    try {
        s->send(hdl, msg->get_payload(), msg->get_opcode());
    } catch (const websocketpp::lib::error_code& e) {
        std::cout << "Echo failed because: " << e
                  << "(" << e.message() << ")" << std::endl;
    }
	*/
}

void WebSocketSrv::on_open(server* s, websocketpp::connection_hdl hdl)
{
	WebSocketSrv::instance().state = WSOCK_OPEN;
	WebSocketSrv::instance().con_hdl = hdl;

	wxGetApp().GetMainFrame()->OpenConnect();
}

void WebSocketSrv::on_close(server* s, websocketpp::connection_hdl hdl)
{
	WebSocketSrv::instance().state = WSOCK_CLOSE;
	wxGetApp().GetMainFrame()->CloseConnect();
}

void WebSocketSrv::SendDate(std::string json_date)
{	
	server::message_ptr msg;

//	msg->set_payload(json_date);
//	ws_server.send(con_hdl, msg);
	ws_server.send(con_hdl, json_date.c_str(), websocketpp::frame::opcode::text);
}

void WebSocketSrv::StopServer()
{
	ws_server.stop();
}

void WebSocketSrv::Run(int port)
{
	// Listen on port
	ws_server.listen(port);

	// Start the server accept loop
	ws_server.start_accept();

	// Start the ASIO io_service run loop
	ws_server.run();
}