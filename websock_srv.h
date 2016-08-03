#pragma once

#define _VARIADIC_MAX 8
#define _WEBSOCKETPP_CPP11_THREAD_
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_MEMORY_
#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_

#define ASIO_STANDALONE

#include <system_error>

//#include "asio.hpp"

#include <websocketpp/config/asio_no_tls.hpp>

#include <websocketpp/server.hpp>

#include <iostream>

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
typedef server::message_ptr message_ptr;

class WebSocketSrv
{
	// singlton
	WebSocketSrv();
	WebSocketSrv(const WebSocketSrv&);
	WebSocketSrv& operator=(const WebSocketSrv&);
public:	
	static WebSocketSrv& instance() {
        static WebSocketSrv instance;
        return instance;
    }
	enum state_t
	{
		WSOCK_OPEN,
		WSOCK_CLOSE				
	} state;

	state_t GetState() { return state; }
	void SendDate(std::string json_date);
	void StopServer();
	void Run(int port);

private:
	static void on_open(server* s, websocketpp::connection_hdl hdl);
	static void on_close(server* s, websocketpp::connection_hdl hdl);
	static void on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg);

	server ws_server;
	websocketpp::connection_hdl con_hdl;
};
