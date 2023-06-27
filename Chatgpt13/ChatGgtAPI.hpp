#include <iostream>
#include <sstream>
#include <Windows.h>
#include <WinInet.h>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <map>
#include <memory>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wininet.lib")
#include "jsonxx.h"
#include "rest.h"
#include "boost\optional.hpp"

#define _BUFFERSIZE 10000
#define REQ_BUFFSIZE 100010
#define AUTH_BUFF 200
#define MX_TOKENS 4096
#define UNSIGLONG unsigned long
#define STD_CHARVECTOR std::vector<char>
#define STDSTR std::string
#define JSONOBJECT jsonxx::Object
#define JSONARRAY jsonxx::Array
#define JSONSTRING jsonxx::String
#define LISTCONVERSATIONS std::vector<ONE_CONVERSATION>
#define LISTSESSIONS std::vector<LISTCONVERSATIONS>
#define RESPONSEOBJ boost::optional<CHATGPT_RESULT>
#define STDWSTR std::wstring

struct ONE_CONVERSATION
{
	STDSTR question;
	STDSTR answer;
};

struct CHATGPT_RESULT
{
	JSONOBJECT o;
	STDSTR t;
	STD_CHARVECTOR data;
};

STDSTR escape_json(const std::string &s);

//STD_CHARVECTOR Fetch(const char*);

class CHATGPT_API
{
	STDSTR APIKEY;
	STDSTR model;

public:
	void SetKey(const char*);
	void SetModel(const char*);
	STDWSTR Bearer();
	RESPONSEOBJ Text(const char*, int, int);
	~CHATGPT_API();
};

// Global
CHATGPT_API ChatGPT_OBJ;
STDSTR API_KEY, MODEL;
LISTCONVERSATIONS hist_conver;
LISTSESSIONS list_session;
bool isNewSession;

/*
	sets up any globals need by the rest of the calls
	(This might need the API key passed to it.. not sure)
*/
void aichat_open(STDSTR, STDSTR);

/*
	ends any current session and starts a new session using the given api_key for the session
	(this might need to be in aichat_open)
	returns the session# that was started
*/
void aichat_start_session(const char*);

/*
	sends the prompt and session history tokens to openai
	stores the session history data
	response is the response from openai or a %all or %session
	returns a status -- 0 = we got  response
	   -1 = they want to end the session
*/
RESPONSEOBJ aichat_prompts(STDSTR);

/*
	ends the session
*/
void aichat_end_session();

/*
	closes and connections to openai and deallocates any session history, etc.
*/
void aichat_close();