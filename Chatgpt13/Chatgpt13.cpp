#include "ChatGgtAPI.hpp"

using namespace std;

int _tmain()
{
	DEFAULT_KEY = "your_api_key";
	aichat_open(DEFAULT_KEY, "gpt-3.5-turbo");
	aichat_start_session(API_KEY.c_str());
	
	for (;;)
	{
		STDSTR prompt;
		std::cout << "Enter question:";

		std::getline(std::cin, prompt);

		if (std::size_t pos = prompt.find("%key") != string::npos) 
		{
			string new_api_key;
			stringstream s(prompt);
			string pref_session;
			s >> pref_session;
			if ((s >> std::ws).eof()) 
			{
				cout << "new_api_key empty!" << endl;
			}
			else
			{
				s >> new_api_key;
				if (apikey_validation(new_api_key))
				{
					API_KEY = new_api_key;
					isNewSession = true;
					aichat_start_session(API_KEY.c_str());
					cout << "Your API KEY is valid. You can start a new session with your key. Enjoy!" << endl;
				}
				else
				{
					cout << "Your API KEY is invalide. Please try again with another key. Or you can start a new session with default key now. Enjoy!" << endl;
				}
				continue;
			}
			continue;
		}

		if (std::size_t pos = prompt.find("%session") != string::npos) 
		{
			int session_num;
			stringstream s(prompt);
			string pref_session;
			s >> pref_session;
			// Restore the session
			if (hist_conver.size() > 0) 
			{
				LISTCONVERSATIONS item_conv(hist_conver);
				list_session.insert(list_session.end(), item_conv);
				hist_conver.clear();
				aichat_end_session();
			}

			if ((s >> std::ws).eof()) 
			{
				cout << "Session " << list_session.size() + 1 << ":" << endl;
				session_num = (int)list_session.size();
				isNewSession = true;

				for (int i = 0; i < (int)list_session[session_num - 1].size(); ++i) {
					cout << "Question: " << list_session[session_num - 1][i].question << endl;
					cout << "Answer: " << list_session[session_num - 1][i].answer << endl;
				}
			}
			else 
			{
				s >> session_num;
				if (session_num >(int)list_session.size())
				{
					cout << "The number is invalid. Here are last conversations." << endl;
					session_num = (int)list_session.size();
				}

				isNewSession = true;
				cout << "Session " << session_num << ":" << endl;
				for (int i = 0; i < (int)list_session[session_num - 1].size(); ++i)
				{
					cout << "Question: " << list_session[session_num - 1][i].question << endl;
					cout << "Answer: " << list_session[session_num - 1][i].answer << endl;
				}
				
			}
			continue;
		}

		if (prompt == "%all") {
			// Restore the session
			if (hist_conver.size() > 0)
			{
				LISTCONVERSATIONS item_conv(hist_conver);
				list_session.insert(list_session.end(), item_conv);
				hist_conver.clear();
				aichat_end_session();
			}
			isNewSession = true;
			int session_cnt = 0;
			for (int i = 0; i < (int)list_session.size(); ++i)
			{
				cout << "Session " << ++session_cnt << endl;
				for (int j = 0; j < (int)list_session[i].size(); ++j)
				{
					cout << "Question: " << list_session[i][j].question << endl;
					cout << "Answer: " << list_session[i][j].answer << endl;
				}
				cout << "------------------------------------------" << endl;
			}
			continue;
		}

		if (prompt == "%") {
			// Restore the session
			if (hist_conver.size() > 0) 
			{
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

		if (isNewSession)
		{
			// Restore the session
			if (hist_conver.size() > 0)
			{
				LISTCONVERSATIONS item_conv(hist_conver);
				list_session.insert(list_session.end(), item_conv);
				hist_conver.clear();
				aichat_end_session();
			}
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

bool apikey_validation(STDSTR key)
{
	ChatGPT_OBJ.SetKey(key.c_str());
	ChatGPT_OBJ.SetModel(MODEL.c_str());
	STDSTR testRequest = R"({"role":"user", "content":"Hi"})";
	RESPONSEOBJ res = ChatGPT_OBJ.Text(testRequest.c_str());

	if (!res.has_value()) 
		return false;
	auto& r = res.value();
	
	return r.isvalid;
}

STDSTR REST(STDSTR api_key, char* jsondata)
{
	STDSTR response = "";

	HINTERNET hSession = InternetOpen(
		"REST",
		INTERNET_OPEN_TYPE_DIRECT,
		NULL,
		NULL,
		0);

	if (hSession == NULL)
	{
		std::cout << "InternetOpenA error :" << ::GetLastError() << std::endl;
		return false;
	}

	DWORD dwTimeout = 60000;
	DWORD dwRetries = 10;

	InternetSetOption(hSession, INTERNET_OPTION_CONNECT_TIMEOUT, &dwTimeout, sizeof(DWORD));
	InternetSetOption(hSession, INTERNET_OPTION_CONNECT_RETRIES, &dwRetries, sizeof(DWORD));

	HINTERNET hConnect = InternetConnect(
		hSession,
		"api.openai.com",
		INTERNET_DEFAULT_HTTPS_PORT, // THIS
		"",
		"",
		INTERNET_SERVICE_HTTP,
		0,
		0);
	
	if (hSession == NULL || hConnect == NULL)
		return NETWORK_DISCONNECTED;

	HINTERNET hHttpFile = HttpOpenRequest(
		hConnect,
		"POST",
		"/v1/chat/completions",
		NULL,
		NULL,
		NULL,
		INTERNET_FLAG_SECURE, // THIS
		0);

	STDSTR bearHeader = "Authorization: Bearer " + api_key;

	HttpAddRequestHeaders(
		hHttpFile,
		bearHeader.c_str(),
		(DWORD)-1,
		HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);

	HttpAddRequestHeaders(
		hHttpFile, 
		"Content-Type: application/json", 
		(DWORD)-1, 
		HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);
	
	DWORD startTime = GetTickCount();
	DWORD currentTime = 0;
	while (!HttpSendRequest(hHttpFile, NULL, 0, jsondata, (DWORD)strlen(jsondata))) {
		currentTime = GetTickCount();
//		printf("HttpSendRequest error : (%lu)\n", GetLastError());
		InternetErrorDlg(
			GetDesktopWindow(),
			hHttpFile,
			ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED,
			FLAGS_ERROR_UI_FILTER_FOR_ERRORS |
			FLAGS_ERROR_UI_FLAGS_GENERATE_DATA |
			FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS,
			NULL);
		if (currentTime - startTime > 60000)
			return NETWORK_DISCONNECTED;
	}


	DWORD dwFileSize;
	dwFileSize = BUFSIZ;

	char* buffer;
	buffer = new char[dwFileSize + 1];
	
	for (;;)
	{
		DWORD dwBytesRead;
		BOOL bRead;

		bRead = InternetReadFile(
			hHttpFile,
			buffer,
			dwFileSize + 1,
			&dwBytesRead);

		if (dwBytesRead == 0) break;
		
		if (!bRead) {
			printf("InternetReadFile error : <%lu>\n", GetLastError());
			response.append("");
		}
		else {
			buffer[dwBytesRead] = 0;
			response.append(buffer);
		}
	}

	InternetCloseHandle(hHttpFile);
	InternetCloseHandle(hConnect);
	InternetCloseHandle(hSession);

	return response;
}

STDSTR escape_json(const std::string &s)
{
	std::ostringstream o;
	for (auto c = s.cbegin(); c != s.cend(); c++)
	{
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

void aichat_open(STDSTR _key, STDSTR _model)
{
	API_KEY = _key;
	MODEL = _model;
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
	if (!isNewSession) 
	{
		for (int i = 0; i < (int)hist_conver.size(); ++i)
		{
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

	RESPONSEOBJ res = ChatGPT_OBJ.Text(request_part.c_str());

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

RESPONSEOBJ CHATGPT_API::Text(const char* prompt)
{
	STD_CHARVECTOR data(_BUFFERSIZE);
	char json_data[_BUFFERSIZE];
	sprintf_s(data.data(), _BUFFERSIZE,
		R"({"model": "%s", "messages": [{"role": "system", "content": "You are a helpful assistant."}, %s]})",
		model.c_str(), prompt);
	data.resize(strlen(data.data()));
	sprintf_s(json_data, _BUFFERSIZE,
		R"({"model": "%s", "messages": [{"role": "system", "content": "You are a helpful assistant."}, %s]})",
		model.c_str(), prompt);
	STDSTR str = data.data();

	CHATGPT_RESULT r;

	STDSTR hRESPONSE = REST(APIKEY, json_data);
	
	if (hRESPONSE == NETWORK_DISCONNECTED)
	{
		r.isvalid = false;
		r.t = "HANG FOREVER";
		return r;
	}

	try
	{
		JSONOBJECT jObject;
		jObject.parse(hRESPONSE.data());
		r.o = jObject;

		if (jObject.has<JSONOBJECT>("error")){
			r.t = jObject.get<JSONOBJECT>("error").get<jsonxx::String>("message");
			r.isvalid = false;
		}
		else {
			auto& choices = jObject.get<JSONARRAY>("choices");
			auto& choice0 = choices.get<JSONOBJECT>(0);
			auto& message = choice0.get<JSONOBJECT>("message");

			r.isvalid = true;
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
