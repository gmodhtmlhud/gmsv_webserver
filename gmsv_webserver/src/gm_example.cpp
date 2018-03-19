#include "GarrysMod/Lua/Interface.h"

#include <thread>
#include "mongoose.h"
#include <string>

using namespace GarrysMod::Lua;

static std::thread* http_request_handler = nullptr;
static struct mg_serve_http_opts s_http_server_opts;

static bool running = true;

static void ev_handler(struct mg_connection* nc, const int ev, void* p)
{
	if (ev == MG_EV_HTTP_REQUEST)
	{
		mg_serve_http(nc, static_cast<struct http_message *>(p), s_http_server_opts);
	}
}

void startHttpServer(struct mg_mgr* mgr)
{
	while (running)
	{
		mg_mgr_poll(mgr, 1000);
	}

	mg_mgr_free(mgr);

	delete mgr;
}

LUA_FUNCTION( MyExampleFunction )
{
	if (http_request_handler != nullptr)
	{
		LUA->PushBool(false);
	}
	if (LUA->IsType(1, Type::NUMBER))
	{
		const auto f_number = LUA->GetNumber(1);
		auto str_port = std::to_string(static_cast<int>(f_number));

		struct mg_mgr* mgr = new struct mg_mgr();
		mg_mgr_init(mgr, nullptr);

		const auto nc = mg_bind(mgr, str_port.c_str(), ev_handler);
		if (nc == nullptr)
		{
			LUA->PushBool(false);
			return 1;
		}

		mg_set_protocol_http_websocket(nc);
		s_http_server_opts.document_root = "garrysmod/htdocs/";
		s_http_server_opts.enable_directory_listing = "yes";

		http_request_handler = new std::thread(startHttpServer, mgr);

		LUA->PushBool(true);

		return 1;
	}

	return 1;
}

//
// Called when you module is opened
//
GMOD_MODULE_OPEN()
{
	LUA->PushSpecial(SPECIAL_GLOB);
	LUA->CreateTable();
	{
		LUA->PushCFunction(MyExampleFunction);
		LUA->SetField(-2, "StartServer");
	}
	LUA->SetField(-2, "GmodWebServer");
	LUA->Pop();

	return 0;
}

//
// Called when your module is closed
//
GMOD_MODULE_CLOSE()
{
	if (http_request_handler)
	{
		running = false;

		http_request_handler->join();
		http_request_handler = nullptr;
	}

	return 0;
}
