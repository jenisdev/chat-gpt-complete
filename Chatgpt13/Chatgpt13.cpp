#include "stdafx.h"
#include "ChatGgtAPI.hpp"

using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
	aichat_open();
	aichat_start_session(API_KEY.c_str());
	
	for (;;)
	{
		STDSTR prompt;
		std::cout << "Enter question:";

		std::getline(std::cin, prompt);

		if (std::size_t pos = prompt.find("%session") != string::npos) {
			int session_num;
			stringstream s(prompt);
			string pref_session;
			s >> pref_session;
			// Restore the session
			if (hist_conver.size() > 0) {
				LISTCONVERSATIONS item_conv(hist_conver);
				list_session.insert(list_session.end(), item_conv);
				hist_conver.clear();
				aichat_end_session();
			}

			if ((s >> std::ws).eof()) {
				cout << "Session " << list_session.size() + 1 << ":" << endl;
				session_num = (int)list_session.size();

				for (int i = 0; i < list_session[session_num - 1].size(); ++i) {
					cout << "Question: " << list_session[session_num - 1][i].question << endl;
					cout << "Answer: " << list_session[session_num - 1][i].answer << endl;
				}
			}
			else 
			{
				s >> session_num;
				if (session_num > list_session.size() ) {
					cout << "The number is invalid. Here are last conversations." << endl;
					session_num = (int)list_session.size();
				}

				cout << "Session " << session_num << ":" << endl;
				for (int i = 0; i < list_session[session_num - 1].size(); ++i) {
					cout << "Question: " << list_session[session_num - 1][i].question << endl;
					cout << "Answer: " << list_session[session_num - 1][i].answer << endl;
				}
				
			}
			continue;
		}

		if (prompt == "%all") {
			// Restore the session
			if (hist_conver.size() > 0) {
				LISTCONVERSATIONS item_conv(hist_conver);
				list_session.insert(list_session.end(), item_conv);
				hist_conver.clear();
				aichat_end_session();
			}

			int session_cnt = 0;
			for (int i = 0; i < list_session.size(); ++i) {
				cout << "Session " << ++session_cnt << endl;
				for (int j = 0; j < list_session[i].size(); ++j){
					cout << "Question: " << list_session[i][j].question << endl;
					cout << "Answer: " << list_session[i][j].answer << endl;
				}
				cout << "------------------------------------------" << endl;
			}
			continue;
		}

		if (prompt == "%") {
			// Restore the session
			if (hist_conver.size() > 0) {
				LISTCONVERSATIONS item_conv(hist_conver);
				list_session.insert(list_session.end(), item_conv);
				hist_conver.clear();
			}
			aichat_end_session();
			continue;
		}
			
		if (prompt.empty()) {
			aichat_close();
			break;
		}
		RESPONSEOBJ res = aichat_prompts(prompt);

		if (!res.has_value())
			continue;

		auto& r = res.value();

		// Restore the conversations in Queue
		ONE_CONVERSATION conv;
		conv.question = prompt;
		conv.answer = r.t;
		hist_conver.insert(hist_conver.end(), conv);

		cout << r.t << endl;
	}
}

STDSTR escape_json(const std::string &s)
{
	std::ostringstream o;
	for (auto c = s.cbegin(); c != s.cend(); c++) {
		switch (*c) {
		case '\t': o << "\\t"; break;
		case '"': o << "\\\""; break;
		case '\\': o << "\\\\"; break;
		case '\b': o << "\\b"; break;
		case '\f': o << "\\f"; break;
		case '\n': o << "\\n"; break;
		case '\r': o << "\\r"; break;
		default: o << *c;
		}
	}
	return o.str();
}

void aichat_open()
{
	API_KEY = "your-key";
	MODEL = "gpt-3.5-turbo";
	isNewSession = false;
}

void aichat_start_session(const char* api_key)
{
	ChatGPT_OBJ.SetKey(api_key);
	ChatGPT_OBJ.SetModel(MODEL.c_str());
	return;
}

RESPONSEOBJ aichat_prompts(STDSTR prompt)
{
	string request_part = "";
	if (!isNewSession) {
		for (int i = 0; i < hist_conver.size(); ++i) {
			string question = hist_conver[i].question;
			string answer = hist_conver[i].answer;

			request_part += R"({"role":"user", "content":")" + escape_json(question) + R"("},)";
			request_part += R"({"role":"assistant", "content":")" + escape_json(answer) + R"("},)";
		}


		request_part += R"({"role":"user", "content":")" + escape_json(prompt) + R"("})";
	}
	else
	{
		request_part = R"({"role": "user", "content":")" + escape_json(prompt) + R"("})";
		isNewSession = false;
	}

	RESPONSEOBJ res = ChatGPT_OBJ.Text(request_part.c_str(), 0, MX_TOKENS);

	return res;
}

void aichat_end_session()
{
	isNewSession = true;
}

void aichat_close()
{
	isNewSession = true;
	hist_conver.clear();
	list_session.clear();
	ChatGPT_OBJ.~CHATGPT_API();
}

void CHATGPT_API::SetKey(const char* KEY)
{
	APIKEY = KEY;
}

void CHATGPT_API::SetModel(const char* MODEL)
{
	model = MODEL;
}

STDWSTR CHATGPT_API::Bearer()
{
	wchar_t auth[AUTH_BUFF] = {};
	swprintf_s(auth, AUTH_BUFF, L"Authorization: Bearer %S", APIKEY.c_str());
	return auth;
}

RESPONSEOBJ CHATGPT_API::Text(const char* prompt, int Temperature = 0, int max_tokens = MX_TOKENS)
{
	STD_CHARVECTOR data(_BUFFERSIZE);
	sprintf_s(data.data(), _BUFFERSIZE,
		R"({"model": "%s", "messages": [{"role": "system", "content": "You are a helpful assistant."}, %s]})",
		model.c_str(), prompt);
	data.resize(strlen(data.data()));

	STDSTR str = data.data();

	RESTAPI::REST hREST;
	hREST.Connect(L"api.openai.com", true, 0, 0, 0, 0);
	std::initializer_list<STDWSTR> hdrs = {
		Bearer(),
		L"Content-Type: application/json",
	};
	auto hInternetConnection = hREST.RequestWithBuffer(L"/v1/chat/completions", L"POST", hdrs, data.data(), data.size());

	STD_CHARVECTOR hRESPONSE;
	hREST.ReadToMemory(hInternetConnection, hRESPONSE);
	hRESPONSE.resize(hRESPONSE.size() + 1);

	try
	{
		JSONOBJECT jObject;
		jObject.parse(hRESPONSE.data());
		CHATGPT_RESULT r;
		r.o = jObject;

		if (jObject.has<JSONOBJECT>("error"))
			r.t = jObject.get<JSONOBJECT>("error").get<jsonxx::String>("message");
		else {
			auto& choices = jObject.get<JSONARRAY>("choices");
			auto& choice0 = choices.get<JSONOBJECT>(0);
			auto& message = choice0.get<JSONOBJECT>("message");
			r.t = message.get<JSONSTRING>("content");
		}

		return r;
	}
	catch (int Err)
	{
		std::cout << "An exception occurred. Exception Nr." << Err << std::endl;
	}
	return {};
}

CHATGPT_API::~CHATGPT_API()
{
	cout << "Closed all connections. Bye!" << endl;
}