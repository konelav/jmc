#include "stdafx.h"
#include <winsock2.h>
#include "tintin.h"
#include "telnet.h"
#include <time.h>

#include "ttcoreex.h"
#include "JmcObj.h"

#include <map>
#include <vector>

const TelnetOption TelnetOptions[TELNET_OPTIONS_NUM] = {
	{TN_BINARY,    L"BINARY",L"Binary data stream"},
	{TN_EOR_OPT,   L"EOR",   L"End-of-record"},							/* should be controlled by JMC */
	{TN_AYT_OPT,   L"AYT",   L"Are you there?"},						/* should be controlled by JMC */
	{TN_ECHO,      L"ECHO",  L"Server echoes user's input"},			/* should be controlled by JMC */
	{TN_NAWS,      L"NAWS",  L"Negotiate about window size"},			/* should be controlled by JMC */
	{TN_TTYPE,     L"MTTS",  L"MUD Terminal Type Standard"},			/* should be controlled by JMC */
	{TN_MSDP,      L"MSDP",  L"MUD Server Data Protocol"},			
	{TN_MSSP,      L"MSSP",  L"MUD Server Status Protocol"},			
	{TN_COMPRESS2, L"MCCP",  L"MUD Client Compression Protocol v2"},	/* should be controlled by JMC */
	{TN_COMPRESS,  L"MCCP1", L"MUD Client Compression Protocol v1"},	/* should be controlled by JMC */
	{TN_COMPRESS2, L"MCCP2", L"MUD Client Compression Protocol v2"},	/* should be controlled by JMC */
	{TN_MSP,       L"MSP",   L"MUD Sound Protocol"},
	{TN_MXP,       L"MXP",   L"MUD eXtension Protocol"},
	{TN_ATCP,      L"ATCP",  L"Achaea Telnet Client Protocol"},
	{TN_GMCP,      L"GMCP",  L"Generic MUD Communication Protocol"},
	{TN_CHARSET,   L"CHARSET",L"Character set selection"}				/* should be controlled by JMC */
};

std::vector<unsigned int> vEnabledTelnetOptions;
std::vector<char> SubnegotiationBuffer;
std::map<wstring,wstring> mWebsocketOptions;

extern GET_WNDSIZE_FUNC GetWindowSize;
extern CComObject<CJmcObj>* pJmcObj;
extern wchar_t MUDHostName[256];

int get_telnet_option_num(const wchar_t *name)
{
	int i;
	if(is_all_digits(name) && wcslen(name) > 0)
		return _wtoi(name);
	for(i = 0; i < TELNET_OPTIONS_NUM; i++)
		if(is_abrev(name, TelnetOptions[i].Name))
			return TelnetOptions[i].Code;
	return -1;
}
void get_telnet_option_name(unsigned int num, wchar_t *buf)
{
	int i;
	for(i = 0; i < TELNET_OPTIONS_NUM; i++)
		if(TelnetOptions[i].Code == num) {
			wcscpy(buf, TelnetOptions[i].Name);
			return;
		}
	swprintf(buf, L"%d", num);
}
static void get_telnet_option_descr(unsigned int num, wchar_t *buf)
{
	int i;
	for(i = 0; i < TELNET_OPTIONS_NUM; i++)
		if(TelnetOptions[i].Code == num) {
			swprintf(buf, L"%d[%s] [%ls]",
				TelnetOptions[i].Code,
				TelnetOptions[i].Name,
				TelnetOptions[i].Descr);
			return;
		}
	swprintf(buf, L"%d [unknown telnet option]", num);
}

BOOL telnet_option_enabled(unsigned int Option)
{
	int i;
	for(i = 0; i < vEnabledTelnetOptions.size(); i++)
		if(vEnabledTelnetOptions[i] == Option)
			return TRUE;
	return FALSE;
}

BOOL DLLEXPORT bTelnetDebugEnabled = FALSE;
BOOL DLLEXPORT bWebsocketDebugEnabled = FALSE;
BOOL DLLEXPORT bSecureDebugEnabled = FALSE;
BOOL DLLEXPORT bWebsocketEnabled = FALSE;

wchar_t DLLEXPORT strPromptEndSequence[BUFFER_SIZE];
wchar_t DLLEXPORT strPromptEndReplace[BUFFER_SIZE];

unsigned char strPromptEndSeqBytes[BUFFER_SIZE];
unsigned char strPromptEndReplBytes[BUFFER_SIZE];

BOOL DLLEXPORT bPromptEndEnabled = FALSE;

unsigned char State;
unsigned char CurrentSubnegotiation;
bool SentDoCompress2;
bool SentCharsetWILL;
int MttsCounter;
unsigned int SocketFlags;
int LastWidthReported, LastHeightReported;

int PromptEndIndex;

z_stream mccp_stream;
z_stream ws_stream_in, ws_stream_out;

DWORD ws_ping_sent = 0;

void TelnetMsg(wchar_t *type, int cmd, int opt)
{
	static wchar_t cmdstr[32], optstr[256], buf[512];
	
	if (!mesvar[MSG_TELNET] || !bTelnetDebugEnabled)
		return;

	switch ((unsigned char)cmd) {
	case TN_WILL:
		swprintf(cmdstr, L"WILL");
		break;
	case TN_WONT:
		swprintf(cmdstr, L"WONT");
		break;
	case TN_DO:
		swprintf(cmdstr, L"DO");
		break;
	case TN_DONT:
		swprintf(cmdstr, L"DONT");
		break;

	case TN_SB:
		swprintf(cmdstr, L"SB");
		break;
	case TN_SE:
		swprintf(cmdstr, L"SE");
		break;
	

	case TN_EOR:
		swprintf(cmdstr, L"EOR");
		break;
	
	case TN_NOP:
		swprintf(cmdstr, L"NOP");
		break;
	case TN_DATA_MARK:
		swprintf(cmdstr, L"DATA_MARK");
		break;
	case TN_BRK:
		swprintf(cmdstr, L"BRK");
		break;
	case TN_IP:
		swprintf(cmdstr, L"IP");
		break;
	case TN_AO:
		swprintf(cmdstr, L"AO");
		break;
	case TN_AYT:
		swprintf(cmdstr, L"AYT");
		break;
	case TN_EC:
		swprintf(cmdstr, L"EC");
		break;
	case TN_EL:
		swprintf(cmdstr, L"EL");
		break;
	case TN_GA:
		swprintf(cmdstr, L"GA");
		break;

	default:
		swprintf(cmdstr, L"%d", cmd);
	}

	if(opt >= 0)
		get_telnet_option_descr(opt, optstr);
	else
		wcscpy(optstr, L"");
	
	swprintf(buf, L"#TELNET %ls IAC:%ls", type, cmdstr);
	if(wcslen(optstr) > 0) {
		wcscat(buf, L":");
		wcscat(buf, optstr);
	}
	tintin_puts2(buf);
}

void send_telnet_command(unsigned char cmd, int opt)
{
	char szBuf[4], len = 0;
	szBuf[len++] = (char)TN_IAC;
	szBuf[len++] = (char)cmd;
	if (opt >= 0)
		szBuf[len++] = (char)opt;

	tls_send(MUDSocket, szBuf, len);
    
	TelnetMsg(L"send", cmd, opt);

	if (cmd != TN_SE && cmd != TN_SB) {
		pJmcObj->m_pvarEventParams[0] = ((LONG)cmd);
		pJmcObj->m_pvarEventParams[1] = ((LONG)opt);
		pJmcObj->m_pvarEventParams[2].Clear();
		
		BOOL bRet = pJmcObj->Fire_Telnet();
		if ( bRet ) {
		}
	}
}

static void RecvCmd(int cmd, int opt = -1)
{
	TelnetMsg(L"recv", cmd, opt);

	if (cmd != TN_SE && cmd != TN_SB) {
		pJmcObj->m_pvarEventParams[0] = ((LONG)cmd);
		pJmcObj->m_pvarEventParams[1] = ((LONG)opt);
		pJmcObj->m_pvarEventParams[2].Clear();
		
		BOOL bRet = pJmcObj->Fire_Telnet();
		if ( bRet ) {
		}
	}
}


#ifdef _DEBUG_LOG
extern DLLEXPORT HANDLE hExLog;
#endif

static void start_decompressing()
{
	if(!(SocketFlags & SOCKCOMPRESSING)) {
		mccp_stream.zalloc = Z_NULL;
		mccp_stream.zfree = Z_NULL;
		mccp_stream.opaque = Z_NULL;
		mccp_stream.avail_in = 0;
		mccp_stream.next_in = Z_NULL;
		int ret = inflateInit(&mccp_stream);
		if (ret != Z_OK) {
			//what should we do?
			tintin_puts(L"#telnet: error while initializing MCCP decompression");
		}
	}
	SocketFlags |= SOCKCOMPRESSING;
}
static void stop_decompressing()
{
	if(SocketFlags & SOCKCOMPRESSING) {
		inflateEnd(&mccp_stream);
		SocketFlags &= ~SOCKCOMPRESSING;
	}
}
static void do_decompressing(const char *src, int size, int *used, char *dst, int capacity, int *decompressed)
{
	mccp_stream.avail_in = size;
	mccp_stream.next_in = (unsigned char*)src;
	mccp_stream.avail_out = capacity;
    mccp_stream.next_out = (unsigned char*)dst;

    int ret = inflate(&mccp_stream, Z_SYNC_FLUSH);
    switch (ret) {
		case Z_STREAM_ERROR:
        case Z_NEED_DICT:
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
			ret = Z_DATA_ERROR;
			break;
    }

	*used = size - mccp_stream.avail_in;
	*decompressed = capacity - mccp_stream.avail_out;

	if (ret != Z_OK) {
		tintin_puts2(rs::rs(1341));
		stop_decompressing();
	}
}

static void init_ws_compression()
{
	if(!(SocketFlags & SOCKWSCOMPRESS)) {
		ws_stream_in.zalloc = Z_NULL;
		ws_stream_in.zfree = Z_NULL;
		ws_stream_in.opaque = Z_NULL;
		ws_stream_in.avail_in = 0;
		ws_stream_in.next_in = Z_NULL;
		int ret = inflateInit(&ws_stream_in);
		if (ret != Z_OK) {
			tintin_puts(L"websocket: error while initializing decompression");
		}

		ws_stream_out.zalloc = Z_NULL;
		ws_stream_out.zfree = Z_NULL;
		ws_stream_out.opaque = Z_NULL;
		ws_stream_out.avail_in = 0;
		ws_stream_out.next_in = Z_NULL;
		ret = deflateInit(&ws_stream_out, Z_DEFAULT_COMPRESSION);
		if (ret != Z_OK) {
			tintin_puts(L"websocket: error while initializing compression");
		}
	}
	SocketFlags |= SOCKWSCOMPRESS;
}
static void deinit_ws_compression()
{
	if(SocketFlags & SOCKWSCOMPRESS) {
		inflateEnd(&ws_stream_in);
		deflateEnd(&ws_stream_out);
		SocketFlags &= ~SOCKWSCOMPRESS;
	}
}
static void do_ws_decompression(const char *src, int size, int *used, char *dst, int capacity, int *decompressed)
{
	ws_stream_in.avail_in = size;
	ws_stream_in.next_in = (unsigned char*)src;
	ws_stream_in.avail_out = capacity;
    ws_stream_in.next_out = (unsigned char*)dst;

    int ret = inflate(&ws_stream_in, Z_SYNC_FLUSH);
    switch (ret) {
		case Z_STREAM_ERROR:
        case Z_NEED_DICT:
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
			ret = Z_DATA_ERROR;
			break;
    }

	*used = size - ws_stream_in.avail_in;
	*decompressed = capacity - ws_stream_in.avail_out;

	if (ret != Z_OK) {
		tintin_puts2(rs::rs(1341));
		deinit_ws_compression();
	}
}
static void do_ws_compression(const char *src, int size, int *used, char *dst, int capacity, int *compressed)
{
	ws_stream_out.avail_in = size;
	ws_stream_out.next_in = (unsigned char*)src;
	ws_stream_out.avail_out = capacity;
    ws_stream_out.next_out = (unsigned char*)dst;

    int ret = deflate(&ws_stream_out, Z_FULL_FLUSH);
    switch (ret) {
		case Z_STREAM_ERROR:
        case Z_NEED_DICT:
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
			ret = Z_DATA_ERROR;
			break;
    }

	*used = size - ws_stream_out.avail_in;
	*compressed = capacity - ws_stream_out.avail_out;

	if (ret != Z_OK) {
		tintin_puts2(rs::rs(1341));
		deinit_ws_compression();
	}
}


void telnet_command(wchar_t *arg) 
{
	wchar_t option[BUFFER_SIZE], flag[BUFFER_SIZE], tmp[BUFFER_SIZE];

	arg = get_arg_in_braces(arg,option,STOP_SPACES,sizeof(option)/sizeof(wchar_t)-1);
    arg = get_arg_in_braces(arg,flag,STOP_SPACES,sizeof(flag)/sizeof(wchar_t)-1);

	if ( !wcslen(option) ) {
		wchar_t options[BUFFER_SIZE];
		if(vEnabledTelnetOptions.size() == 0) {
			wcscpy(options, L"-");
		} else {
			wcscpy(options, L"");
			for(int i = 0; i < vEnabledTelnetOptions.size(); i++) {
				if(i > 0)
					wcscat(options,L", ");
				get_telnet_option_name(vEnabledTelnetOptions[i], tmp);
				wcscat(options, tmp);
			}
		}
		swprintf(tmp, rs::rs(1266), options);
		tintin_puts2(tmp);
	} else if ( is_abrev(option, L"debug") ) {
		if ( wcslen(flag) ==0 )
			bTelnetDebugEnabled = !bTelnetDebugEnabled;
		else if ( !wcsicmp(flag, L"on") )
			bTelnetDebugEnabled = TRUE;
		else
			bTelnetDebugEnabled = FALSE;

		swprintf(tmp, rs::rs(1267), option,
			bTelnetDebugEnabled ? L"ON" : L"OFF");
			tintin_puts2(tmp);
	} else {
		int opt = get_telnet_option_num(option);
		if(opt < 0) {
			swprintf(tmp, rs::rs(1280), option);
			tintin_puts2(tmp);
		} else {
			if ( wcslen(flag) ) {
				if ( !wcsicmp(flag, L"on") ) {
					if ( !telnet_option_enabled(opt) )
						vEnabledTelnetOptions.push_back(opt);
				} else {
					for ( int i = 0; i < vEnabledTelnetOptions.size(); i++ )
						if ( vEnabledTelnetOptions[i] == opt ) {
							vEnabledTelnetOptions.erase(vEnabledTelnetOptions.begin() + i);
							break;
						}
				}
			}
			get_telnet_option_name(opt, option);
			swprintf(tmp, rs::rs(1267), 
				option,
				telnet_option_enabled(opt) ? L"ON" : L"OFF");
			tintin_puts2(tmp);
		}

	}
}

void websocket_command(wchar_t *arg) 
{
	wchar_t option[BUFFER_SIZE], flag[BUFFER_SIZE], val[BUFFER_SIZE], msg[BUFFER_SIZE];

	arg = get_arg_in_braces(arg,option,STOP_SPACES,sizeof(option)/sizeof(wchar_t)-1);
	arg = get_arg_in_braces(arg,flag,STOP_SPACES,sizeof(flag)/sizeof(wchar_t)-1);
    arg = get_arg_in_braces(arg,val,STOP_SPACES,sizeof(val)/sizeof(wchar_t)-1);

	if ( !wcslen(option) ) {
		wchar_t options[BUFFER_SIZE];
		if(mWebsocketOptions.size() == 0) {
			wcscpy(options, L"-");
		} else {
			wcscpy(options, L"");
			for (std::map<wstring,wstring>::iterator it = mWebsocketOptions.begin(); it != mWebsocketOptions.end(); it++) {
				if (options[0] != L'\0')
					wcscat(options ,L", ");
				wcscat(options, it->first.c_str());
				if (it->second.length()) {
					wcscat(options, L"(");
					wcscat(options, it->second.c_str());
					wcscat(options, L")");
				}
			}
		}
		swprintf(msg, L"#websocket status: %ls, options: %ls",
			bWebsocketEnabled ? L"ON" : L"OFF", options); // TODO rs::rs
	} else if ( is_abrev(option, L"debug") ) {
		if ( wcslen(flag) ==0 )
			bWebsocketDebugEnabled = !bWebsocketDebugEnabled;
		else if ( !wcsicmp(flag, L"on") )
			bWebsocketDebugEnabled = TRUE;
		else
			bWebsocketDebugEnabled = FALSE;

		swprintf(msg, L"#websocket debug: %ls",  //TODO rs::rs
			bWebsocketDebugEnabled ? L"ON" : L"OFF");
	} else if ( is_abrev(option, L"on") || is_abrev(option, L"off") ) {
		bWebsocketEnabled = is_abrev(option, L"on");
		swprintf(msg, L"#websocket protocol: %ls", option,  //TODO rs::rs
			bWebsocketEnabled ? L"ON" : L"OFF");
		
	} else {
		if ( wcslen(flag) ) {
			if ( !wcsicmp(flag, L"disable") ) {
				std::map<wstring,wstring>::iterator it = mWebsocketOptions.find(option);
				if (it != mWebsocketOptions.end())
					mWebsocketOptions.erase(it);
				swprintf(msg, L"#websocket option '%ls' disabled", option);
			} else if ( !wcsicmp(flag, L"enable") ) {
				mWebsocketOptions[option] = val;
				if (!wcslen(val))
					swprintf(msg, L"#websocket option '%ls' enabled", option);
				else
					swprintf(msg, L"#websocket option '%ls' set to '%ls'", option, val);
			} else {
				mWebsocketOptions[option] = flag;
				swprintf(msg, L"#websocket option '%ls' set to '%ls'", option, flag);
			}
		} else {
			std::map<wstring,wstring>::iterator it = mWebsocketOptions.find(option);
			if (it == mWebsocketOptions.end())
				swprintf(msg, L"#websocket option '%ls' disabled", option);
			else
				swprintf(msg, L"#websocket option '%ls' set to '%ls'", option, it->second.c_str());
		}
	}

	tintin_puts2(msg);
}

void promptend_command(wchar_t *arg) 
{
	wchar_t left[BUFFER_SIZE], right[BUFFER_SIZE];
	wchar_t buff[BUFFER_SIZE];

    arg = get_arg_in_braces(arg,left,STOP_SPACES,sizeof(left)/sizeof(wchar_t)-1);

	if ( *left == 0 ) {
		if (!bPromptEndEnabled)
			swprintf(buff, rs::rs(1278));
		else
			swprintf(buff, rs::rs(1279), strPromptEndSequence, strPromptEndReplace);
	} else if (is_abrev(left, L"disable")) {
		bPromptEndEnabled = FALSE;
		swprintf(buff, rs::rs(1278));
	} else {
		if (*left) {
			arg = get_arg_in_braces(arg,right,STOP_SPACES,sizeof(right)/sizeof(wchar_t)-1);
			
			wcscpy(strPromptEndSequence, left);
			wcscpy(strPromptEndReplace, right);

			int len = WideCharToMultiByte(MudCodePageUsed, 0, strPromptEndSequence, wcslen(strPromptEndSequence), 
				(char*)strPromptEndSeqBytes, sizeof(strPromptEndSeqBytes), NULL, NULL);
			strPromptEndSeqBytes[len] = '\0';

			len = WideCharToMultiByte(MudCodePageUsed, 0, strPromptEndReplace, wcslen(strPromptEndReplace), 
				(char*)strPromptEndReplBytes, sizeof(strPromptEndReplBytes), NULL, NULL);
			strPromptEndReplBytes[len] = '\0';
			
			bPromptEndEnabled = TRUE;
			PromptEndIndex = 0;
			swprintf(buff, rs::rs(1279), strPromptEndSequence, strPromptEndReplace);
		}
	}
	tintin_puts2(buff);
}

int send_websocket_frame(int opcode, const char *data, unsigned int count)
{
	bool FIN = true;
	bool RSV1 = false,
		 RSV2 = false,
		 RSV3 = false;
	bool masked = true;
	
    char buf[BUFFER_SIZE + 64], chunk[BUFFER_SIZE + 64];
    char mask[4];
    int bufsize = 0;

	char *tmp_buf = NULL;

	pJmcObj->m_pvarEventParams[0] = 1;
	pJmcObj->m_pvarEventParams[1] = opcode;
	pJmcObj->m_pvarEventParams[2] = (data);
	
    BOOL bRet = pJmcObj->Fire_Websocket();

	if (!bRet)
		return 0;

	//TODO: copy pJmcObj->m_pvarEventParams[2].bstrVal to data

	if (SocketFlags & SOCKWSCOMPRESS) {
		RSV1 = true;
		deflateReset(&ws_stream_out);

		tmp_buf = new char[count+256];
		int used = 0, compressed = 0;
		do_ws_compression(data, count, &used, tmp_buf, count + 256, &compressed);
		if (used < count)
			tintin_puts(L"#websocket: compression error, can't deflate all bytes");

		data = tmp_buf;
		count = compressed;
	}
    
    buf[bufsize++] = (FIN  ? 0x80  : 0) |
                     (RSV1 ? 0x40 : 0) |
                     (RSV2 ? 0x20 : 0) |
                     (RSV3 ? 0x10 : 0) |
                     opcode;

    if (count <= 125)
        buf[bufsize++] = (masked ? 0x80 : 0) | count;
    else if (count < 0x10000) {
        buf[bufsize++] = (masked ? 0x80 : 0) | 126;
        for (int nb = 2; nb > 0; nb--)
            buf[bufsize++] = (count >> ((nb - 1)*8)) & 0xff;
    } else {
        buf[bufsize++] = (masked ? 0x80 : 0) | 127;
        buf[bufsize++] = 0;
        for (int nb = 7; nb > 0; nb--)
            buf[bufsize++] = (count >> ((nb - 1)*8)) & 0xff;
    }

    if (masked) {
        for (int nb = 0; nb < sizeof(mask); nb++)
            buf[bufsize++] = mask[nb] = rand() & 0xff;
    } else
        memset(mask, 0, sizeof(mask));
    
	int total_sent;
    for (total_sent = 0; total_sent < count; ) {
        int to_send = count;
        if (bufsize + to_send > sizeof(buf))
            to_send = sizeof(buf) - bufsize;

		memcpy(chunk, &data[total_sent], to_send);

        for (int i = 0; i < to_send; i++)
            buf[bufsize++] = chunk[i] ^ mask[(total_sent + i) % sizeof(mask)];

        int sent = tls_send(MUDSocket, buf, bufsize);
		if (sent < 0)
			break;
        bufsize = 0;
        total_sent += to_send;
    }

	if (tmp_buf)
		delete[] tmp_buf;

	if (opcode == WS_OPCODE_PING)
		ws_ping_sent = GetTickCount();

	return total_sent;
}

int send_websocket_ping(int timeout_ms) {
	if (!(SocketFlags & SOCKWEBSOCKET))
		return 0;

	if (ws_ping_sent > 0) {
		DWORD now = GetTickCount();
		int elapsed = (int)(now - ws_ping_sent);
		if (elapsed >= 0 && elapsed < timeout_ms)
			return 0;
	}
	send_websocket_frame(WS_OPCODE_PING, "ping", strlen("ping"));
	return 1;
}

int telnet_send_command(const char* output, int length) {
    char buff[BUFFER_SIZE*8 + 32];
    int ret = 0;
	int size = length;

	if (size > sizeof(buff)/2 - 32)
		size = sizeof(buff)/2 - 32;

	BOOL iac_single = bIACSendSingle;
	BOOL add_cr = TRUE;

	if (bWebsocketEnabled) {
		if (mWebsocketOptions.find(L"single-iac") != mWebsocketOptions.end())
			iac_single = TRUE;
		if (mWebsocketOptions.find(L"no-cr") != mWebsocketOptions.end())
			add_cr = FALSE;
	}

	if ( iac_single )
		memcpy (buff, output, size);
	else {
		char* ptr = buff;
		int nIACs = 0;
		for ( int i = 0; i < size; i ++ ) {
			if ( output[i] == (char)0xff ) {
				*ptr++ = (char)0xff;
				nIACs ++;
			}
			*ptr++ = output[i];
		}
		size += nIACs;
	}
	buff[size] = '\0';

	if (add_cr) {
		if (MudCodePageUsed == 1200 || MudCodePageUsed == 1201) {
			if ( *(wchar_t*)(&buff[size-2]) != L'\n' ) {
				*(wchar_t*)(&buff[size]) = L'\n';
				size += 2;
				*(wchar_t*)(&buff[size]) = L'\0';
			}
		} else {
			if ( buff[size-1] != '\n' ) {
				buff[size] = '\n';
				size++;
				buff[size] = '\0';
			}
		}
	}

	int sent;

	if (SocketFlags & SOCKWEBSOCKET) {
		sent = send_websocket_frame(WS_OPCODE_TEXT, buff, size);
	} else {
		sent = tls_send(MUDSocket, buff, size);
	}

	if (sent < 0)
		ret = sent;
	else
		ret = size;

#ifdef _DEBUG_LOG
    // --------   Write external log
    if (hExLog ) {
        char exLogText[128];
        DWORD Written;
        swprintf(exLogText , L"\r\n#SEND got %d bytes sent %d bytes#\r\n" , size, ret );
        WriteFile(hExLog , exLogText , wcslen(exLogText) , &Written, NULL);
        WriteFile(hExLog , buff , size , &Written, NULL);
    }
#endif

	return ret;
}

char *pWSData = NULL;
int WSCapacity = 0;
int WSSize = 0;

char *pInputData = NULL;
int InputCapacity = 0;
int InputSize = 0;

char *pDecompressedData = NULL;
int DecompressedCapacity = 0;
int DecompressedSize = 0;

char *pOutputData = NULL;
int OutputCapacity = 0;
int OutputSize = 0;

void free_telnet_buffer() {
	if (pWSData)
		free(pWSData);
	if (pInputData)
		free(pInputData);
	if (pDecompressedData)
		free(pDecompressedData);
	if (pOutputData)
		free(pOutputData);
	pWSData = pInputData = pDecompressedData = pOutputData = NULL;
	WSCapacity = InputCapacity = DecompressedCapacity = OutputCapacity = 0;
	WSSize = InputSize = DecompressedSize = OutputSize = 0;
}

void increase_capacity(char **ppBuf, int *capacity, int needed) {
	if (needed > *capacity) {
		int newcap = needed + BUFFER_SIZE;
		char *newptr = (char*)realloc(*ppBuf, newcap);
		if (newptr) {
			*capacity = newcap;
			*ppBuf = newptr;
		}
	}
}

void reset_telnet_protocol()
{
	deinit_ws_compression();
	stop_decompressing();
	SocketFlags = 0;
	SentDoCompress2 = false;
	SentCharsetWILL = false;
	MttsCounter = 0;
	LastWidthReported = LastHeightReported = 0;
	CurrentSubnegotiation = 0;
	WSSize = InputSize = OutputSize = DecompressedSize = 0;
	State = '\0';
	SubnegotiationBuffer.clear();
	reset_oob();
	PromptEndIndex = 0;
	MudCodePageUsed = MudCodePage;
}

static unsigned int recv_websocket_frame(char *src, int size)
{
    if (size < 2)
        return 0;
    unsigned char header = src[0];
	bool FIN  = ((header & 0x80) != 0);
	bool RSV1 = ((header & 0x40) != 0);
	bool RSV2 = ((header & 0x20) != 0);
	bool RSV3 = ((header & 0x10) != 0);
	int opcode = (header & 0x0F);
    bool masked = ((src[1] & 0x80) != 0);
    unsigned int count = src[1] & 0x7F;

	if (!FIN) {
		tintin_puts(L"#websocket: fragmented message received, not supported");
	}
    
    char mask[4];

	int ret = 0, nb;
    
    src += 2;
    size -= 2;
	ret += 2;
    if (count == 126) {
        if (size < 2)
            return 0;

        for (nb = 0, count = 0; nb < 2; nb++)
            count = (count << 8) | (unsigned char)src[nb];
        src += 2;
        size -= 2;
		ret += 2;
    } else if (count == 127) {
        if (size < 8)
            return 0;
        for (nb = 1, count = 0; nb < 8; nb++)
            count = (count << 8) | (unsigned char)src[nb];
        src += 8;
        size -= 8;
		ret += 8;
    }
    
    if (masked) {
        if (size < sizeof(mask))
            return 0;
        memcpy(mask, src, sizeof(mask));
        src += sizeof(mask);
        size -= sizeof(mask);
		ret += sizeof(mask);
    } else {
        memset(mask, 0, sizeof(mask));
    }
    
    if (size < count)
        return 0;

	if (masked)
		for (int i = 0; i < count; i++)
			src[i] ^= mask[i % sizeof(mask)];

	ret += count;

	if (bWebsocketDebugEnabled) {
		wchar_t msg[BUFFER_SIZE];
		wsprintf(msg, L"#websocket: received websocket opcode %d, payload size %d", opcode, count);
		tintin_puts(msg);
	}

	pJmcObj->m_pvarEventParams[0] = 0;
	pJmcObj->m_pvarEventParams[1] = opcode;
	pJmcObj->m_pvarEventParams[2] = (src);
	
    BOOL bRet = pJmcObj->Fire_Websocket();
	
	if (!bRet)
		return ret;

	// TODO: copy pJmcObj->m_pvarEventParams[2].bstrVal to src

	if (opcode == WS_OPCODE_TEXT || opcode == WS_OPCODE_BINARY) {
		if ((SocketFlags & SOCKWSCOMPRESS) && RSV1) {
			inflateReset(&ws_stream_in);
			for (int offset = 0;; ) {
				int used = 0, decompressed = 0;
				increase_capacity(&pInputData, &InputCapacity, InputSize + (count - offset)*2 + 256);
				do_ws_decompression(&src[offset], count - offset, &used, pInputData + InputSize, InputCapacity - InputSize, &decompressed);
				InputSize += decompressed;
				offset += used;
				if (!used && !decompressed) {
					if (offset < count)
						tintin_puts(L"#websocket: can't decompress rest of the bytes");
					break;
				}
			}
		} else {
			increase_capacity(&pInputData, &InputCapacity, InputSize + count);
			if (InputSize + count > InputCapacity) {
				//not enough memory!
				count = InputCapacity - InputSize;
			}
			if (count > 0) {
				memcpy(pInputData + InputSize, src, count);
				InputSize += count;
			}
		}

		if (mWebsocketOptions.find(L"message-prompt") != mWebsocketOptions.end()) {
			increase_capacity(&pInputData, &InputCapacity, InputSize + 1);
			if (InputSize + 1 < InputCapacity) {
				pInputData[InputSize++] = END_OF_PROMPT_MARK;
			}
		}
	} else if (opcode == WS_OPCODE_CLOSE) {
		cleanup_session();
		zap_command(L"");
	} else if (opcode == WS_OPCODE_PING) {
		send_websocket_frame(WS_OPCODE_PONG, src, count);
	} else if (opcode == WS_OPCODE_PONG) {
		DWORD now = GetTickCount();
		if (ws_ping_sent > 0) {
			int delay = (int)(now - ws_ping_sent);
			if (delay >= 0 && mWebsocketOptions.find(L"ping") != mWebsocketOptions.end())
				lPingMUD = delay;
		}
	} else {
	}

	return ret;
}

void telnet_push_back(const char *src, int size)
{
	if (SocketFlags & SOCKWEBSOCKET) {
		increase_capacity(&pWSData, &WSCapacity, WSSize + size);
		if (WSSize + size > WSCapacity) {
			//not enough memory!
			size = WSCapacity - WSSize;
		}
		if (size > 0) {
			memcpy(pWSData + WSSize, src, size);
			WSSize += size;
		}

		int ws_processed = 0;
		for (;;) {
			int recvd = recv_websocket_frame(pWSData + ws_processed, WSSize - ws_processed);
			if (recvd <= 0)
				break;
			ws_processed += recvd;
		}
		if (ws_processed < WSSize) {
			memmove(pWSData, pWSData + ws_processed, WSSize - ws_processed);
			WSSize -= ws_processed;
		} else {
			WSSize = 0;
		}
	} else {
		increase_capacity(&pInputData, &InputCapacity, InputSize + size);
		if (InputSize + size > InputCapacity) {
			//not enough memory!
			size = InputCapacity - InputSize;
		}
		if (size > 0) {
			memcpy(pInputData + InputSize, src, size);
			InputSize += size;
		}
	}
}

int telnet_more_coming() {
	if (InputSize > 0 || OutputSize > 0)
		return 1;
	if ((SocketFlags & SOCKCOMPRESSING) && DecompressedSize > 0)
		return 1;
	if ((SocketFlags & SOCKWEBSOCKET) && WSSize > 0)
		return 1;
	return 0;
}

int telnet_pop_front(wchar_t *dst, int maxsize)
{
	int ret = 0;
	increase_capacity(&pOutputData, &OutputCapacity, maxsize);
	while (OutputSize < OutputCapacity && maxsize > 0) {
		int input_processed = 0, output_generated = 0;

		if (SocketFlags & SOCKCOMPRESSING) {
			if (InputSize > 0) {
				increase_capacity(&pDecompressedData, &DecompressedCapacity, DecompressedSize + maxsize);

				int decompressed = 0;
				do_decompressing((const char*)pInputData, InputSize, &input_processed, 
								 (char*)pDecompressedData + DecompressedSize, DecompressedCapacity - DecompressedSize, &decompressed);
				DecompressedSize += decompressed;
			}

			if (DecompressedSize > 0) {
				int processed = 0;
				do_telnet_protecol((const char*)pDecompressedData, DecompressedSize, &processed,
								   (char*)pOutputData + OutputSize, OutputCapacity - OutputSize, &output_generated);

				if (processed < DecompressedSize)
					memmove(pDecompressedData, pDecompressedData + processed, DecompressedSize - processed);
				DecompressedSize -= processed;
			}
		} else if (InputSize > 0) {
			do_telnet_protecol((const char*)pInputData, InputSize, &input_processed,
							   (char*)pOutputData + OutputSize, OutputCapacity - OutputSize, &output_generated);
		}
		
		if (input_processed < InputSize)
			memmove(pInputData, pInputData + input_processed, InputSize - input_processed);
		InputSize -= input_processed;
		
		OutputSize += output_generated;
		
		static DWORD mode = 0;
		/*
		  MultiByteToWideChar(CodePage, 0, pOutputData, OutputSize, dst, maxsize);
		    -- doesn't work correctly with incomplete multibyte sequences
		  memcpy(dst, pOutputData, copy_out); 
		    -- old ANSI line
		  */
		int len = maxsize;
		int processed = min(maxsize, OutputSize);
		int retConv;

		/* Codepages 1200 (UTF-16LE) and 1201 (UTF-16BE) doesn't supported for unmanager applications
		   though they can be very simply implemented for almost(!) all situations
		 */
		if (MudCodePageUsed == 1200) {
			int count = min(len, processed) / sizeof(wchar_t);
			memcpy(dst, pOutputData, count * sizeof(wchar_t));
			len = count;
			processed = count * sizeof(wchar_t);
			retConv = S_OK;
		} else if (MudCodePageUsed == 1201) {
			int count = min(len, processed) / sizeof(wchar_t);
			utf16le_to_utf16be(dst, (const wchar_t*)pOutputData, count);
			len = count;
			processed = count * sizeof(wchar_t);
			retConv = S_OK;
		} else {
			static int last_bytes_rest = 0;
			// there is a bug with ConvertINetMultiByteToUnicode() when converting same incomplete
			// sequence send time in a row
			if (processed == last_bytes_rest) {
				processed = len = 0;
				retConv = S_OK;
			} else {
				int to_conv = processed;
				retConv = fConvertINetMultiByteToUnicode(&mode, MudCodePageUsed, pOutputData, &processed, dst, &len);
				if (retConv == S_OK)
					last_bytes_rest = to_conv - processed;
				else
					last_bytes_rest = 0;
			}
		}
		if (retConv != S_OK) {
			wchar_t buf[BUFFER_SIZE];
			swprintf(buf, rs::rs(1292), processed, retConv);
			tintin_puts2(buf);
		}
		else {
			dst += len;
			maxsize -= len;
		}

		if (processed < OutputSize)
			memmove(pOutputData, pOutputData + processed, OutputSize - processed);
		OutputSize -= processed;
		ret += len;

		if (input_processed <= 0 && processed <= 0)
			break;
	}
	return ret;
}

static void request_charsets()
{
	char cpname[128];
	vector<char> buf;
	char sep = ' ';

	buf.push_back((char)TN_IAC);
	buf.push_back((char)TN_SB);
	buf.push_back((char)TN_CHARSET);
	buf.push_back((char)CHARSET_REQUEST);
	for (map< wstring, UINT >::iterator it = CPIDs.begin(); it != CPIDs.end(); it++) {
		buf.push_back(sep);
		int len = WideCharToMultiByte(CP_UTF8, 0, it->first.c_str(), it->first.length(), 
			cpname, sizeof(cpname), NULL, NULL);
		for (int i = 0; i < len; i++)
			buf.push_back(cpname[i]);
	}
	buf.push_back((char)TN_IAC);
	buf.push_back((char)TN_SE);
	
	tls_send(MUDSocket, buf.begin(), buf.size());
}

static void handle_charsets(const char *data, int length)
{
	if (length <= 0)
		return;

	wchar_t cpname[128];

	char code = data[0];
	data++;
	length--;

	vector<char> confirm;
	UINT confirmed_codepage;
	wstring confirmed_cpname;

	switch (code) {
	case CHARSET_REQUEST:
		{
			char sep = data[0];
			data++;
			length--;
			for (const char *start = data; length >= 0; length--, data++) {
				if ((data[0] == sep || length == 0) && (data > start)) {
					int l = MultiByteToWideChar(CP_UTF8, 0, start, data - start, cpname, sizeof(cpname));
					cpname[l] = L'\0';
					
					for (int i = 0; i < l; i++)
						cpname[i] = towlower(cpname[i]);
					
					map< wstring, UINT>::iterator it;
					if ((it = CPIDs.find(cpname)) != CPIDs.end()) {
						confirmed_codepage = it->second;
						confirmed_cpname = it->first;

						confirm.clear();
						for (const char *src = start; src != data; src++)
							confirm.push_back(*src);
						
						if (it->second == MudCodePage) {
							
							break;
						}
					}
					start = data + 1;
				}
			}
			if (confirm.size() > 0) {
				confirm.insert(confirm.begin(), (char)CHARSET_ACCEPTED);
				MudCodePageUsed = confirmed_codepage;

				if (mesvar[MSG_TELNET]) {
					wchar_t buffer[BUFFER_SIZE];
					swprintf(buffer, rs::rs(1305), confirmed_cpname.c_str());
					tintin_puts2(buffer);
				}
			} else {
				confirm.insert(confirm.begin(), (char)CHARSET_REJECTED);
				tintin_puts2(rs::rs(1304));
			}
			confirm.insert(confirm.begin(), (char)TN_CHARSET);
			confirm.insert(confirm.begin(), (char)TN_SB);
			confirm.insert(confirm.begin(), (char)TN_IAC);

			confirm.push_back((char)TN_IAC);
			confirm.push_back((char)TN_SE);
			
			tls_send(MUDSocket, confirm.begin(), confirm.size());
		}
		break;
	case CHARSET_ACCEPTED:
		{
			int l = MultiByteToWideChar(CP_UTF8, 0, data, length, cpname, sizeof(cpname));
			cpname[l] = L'\0';
			
			for (int i = 0; i < l; i++)
				cpname[i] = towlower(cpname[i]);
				
			map< wstring, UINT>::iterator it;
			if ((it = CPIDs.find(cpname)) != CPIDs.end()) {
				MudCodePageUsed = confirmed_codepage = it->second;
			}
		}
		break;
	case CHARSET_REJECTED:
		{
			tintin_puts2(rs::rs(1304));
		}
		break;
	}
}

void send_telnet_subnegotiation(unsigned char option, const wchar_t *output, int length, bool raw_bytes)
{
	int i;
	vector<char> buf;

	int coded_len;
	char *coded;

	if (raw_bytes) {
		coded_len = length;
		coded = new char[coded_len];
		for (i = 0; i < coded_len; i++) {
			coded[i] = (unsigned char)output[i];
		}
	} else {
		coded_len = WideCharToMultiByte(MudCodePageUsed, 0, output, length, NULL, 0, NULL, NULL);
		coded = new char[coded_len];
		WideCharToMultiByte(MudCodePageUsed, 0, output, length, coded, coded_len, NULL, NULL);
	}

	buf.push_back((char)TN_IAC);
	buf.push_back((char)TN_SB);
	buf.push_back((char)option);
	for (i = 0; i < coded_len; i++) {
		switch (coded[i]) {
		case TN_IAC:
			buf.push_back((char)TN_IAC);
			buf.push_back((char)TN_IAC);
			break;
		case TN_SB:
		case TN_SE:
			break;
		default:
			buf.push_back(coded[i]);
			break;
		}
	}
	buf.push_back((char)TN_IAC);
	buf.push_back((char)TN_SE);

	delete[] coded;
	
	tls_send(MUDSocket, buf.begin(), buf.size());

	if (mesvar[MSG_TELNET]) {
		wchar_t buffer[BUFFER_SIZE], optname[64];
		get_telnet_option_name(option, optname);
		swprintf(buffer, L"#TELNET SB-%ls: send %d byte(s) [%ls]", optname, length, output);
		tintin_puts2(buffer);
	}
}

void recv_telnet_subnegotiation(unsigned char option, const char *input, int length)
{
	if (mesvar[MSG_TELNET]) {
		wchar_t buf[BUFFER_SIZE], optname[64];
		get_telnet_option_name(option, optname);
		swprintf(buf, L"#TELNET SB-%ls: recv %d byte(s)", optname, length);
		tintin_puts2(buf);
	}

	int wide_len = MultiByteToWideChar(MudCodePageUsed, 0, input, length, NULL, 0);
	wchar_t *data = new wchar_t[wide_len + 1];
	MultiByteToWideChar(MudCodePageUsed, 0, input, length, data, wide_len);
	data[wide_len] = L'\0';

	pJmcObj->m_pvarEventParams[0] = ((LONG)TN_SE);
	pJmcObj->m_pvarEventParams[1] = ((LONG)option);
	pJmcObj->m_pvarEventParams[2] = (data);
	
    BOOL bRet = pJmcObj->Fire_Telnet();
	if ( bRet ) {
		switch (option) {
		case TN_GMCP:
			parse_gmcp(data);
			break;
		case TN_MSDP:
			parse_msdp(data, wide_len);
			break;
		case TN_MSSP:
			parse_mssp(data, wide_len);
			break;
		case TN_CHARSET:
			handle_charsets(input, length);
			break;
		}
    }

	delete[] data;
}

void do_telnet_protecol(const char* input, int length, int *used, char* output, int capacity, int *generated)
{
    *used = *generated = 0;
#ifdef _DEBUG_LOG
	if (hExLog ) {
		char exLogText[128];
        DWORD Written;
        sprintf(exLogText , "\r\n#telnet state:%d#\r\n", State);
        WriteFile(hExLog , exLogText , strlen(exLogText) , &Written, NULL);
        //WriteFile(hExLog , buffer , didget , &Written, NULL);
    }
#endif

	if (SocketFlags & SOCKNAWS) {
		wchar_t buf[5];
		int w, h;
		GetWindowSize(-1, w, h);
		if (w != LastWidthReported || h != LastHeightReported) {
			buf[0] = (w >> 8) & 0xFF;
			buf[1] = (w >> 0) & 0xFF;
			buf[2] = (h >> 8) & 0xFF;
			buf[3] = (h >> 0) & 0xFF;
			buf[5] = 0;
			send_telnet_subnegotiation(TN_NAWS, buf, 4, true);
			LastWidthReported = w;
			LastHeightReported = h;
		}
	}

	if (!SentCharsetWILL && telnet_option_enabled(TN_CHARSET)) {
		send_telnet_command(TN_WILL, TN_CHARSET);
		SentCharsetWILL = true;
	}

    for (*used = 0; *used < length && (*generated) < capacity; (*used)++) {
		switch( State ) {
		case TN_IAC: {
				State = input[*used];
 				switch(State) {
					case TN_GA:
						output[(*generated)++] = END_OF_PROMPT_MARK;
						RecvCmd(State);
						State = '\0';
						break;
					case TN_EOR:
						output[(*generated)++] = END_OF_PROMPT_MARK;
						RecvCmd(State);
						State = '\0';
						break;
					case TN_AYT:
						if (telnet_option_enabled(TN_AYT_OPT))
							write_line_mud(L" ");
						RecvCmd(State);
						State = '\0';
						break;

					case TN_SB:
					case TN_WILL:
					case TN_WONT:
					case TN_DO:
					case TN_DONT:
						break;

					case TN_SE:
						RecvCmd(State);
						State = '\0';
						SubnegotiationBuffer.push_back(0);
						recv_telnet_subnegotiation(CurrentSubnegotiation, SubnegotiationBuffer.begin(), SubnegotiationBuffer.size() - 1);
						SubnegotiationBuffer.pop_back();
						switch (CurrentSubnegotiation) {
						case TN_COMPRESS:
							(*used)++;
							CurrentSubnegotiation = 0;
							if (SubnegotiationBuffer.size() == 0) {
								start_decompressing();
								return;
							}
							break;
						case TN_COMPRESS2:
							(*used)++;
							CurrentSubnegotiation = 0;
							if (SubnegotiationBuffer.size() == 0) {
								start_decompressing();
								return;
							}
							break;
						case TN_TTYPE:
							if (SubnegotiationBuffer.size() == 1 && SubnegotiationBuffer[0] == MTTS_SEND) {
								wchar_t mtts_is[128];
								wchar_t *value = &(mtts_is[1]);
								mtts_is[0] = MTTS_IS;

								switch (MttsCounter) {
								case 0:
									swprintf(value, L"%ls", strProductName);
									MttsCounter++;
									break;
								case 1:
									swprintf(value, L"%ls", L"ANSI");
									MttsCounter++;
									break;
								case 2:
									swprintf(value, L"%ls %d", L"MTTS", 
										MTTS_ANSI | MTTS_UTF8);
									break;
								}
								send_telnet_subnegotiation(TN_TTYPE, mtts_is, wcslen(value) + 1, false);
							}
							break;
						case TN_NAWS:
							break;
						}
						CurrentSubnegotiation = 0;
						break;

					case TN_IAC:
						if (CurrentSubnegotiation > 0) {
							SubnegotiationBuffer.push_back(State);
						} else {
							output[(*generated)++] = input[*used];
						}
						State = '\0';
						break;
					default:
						RecvCmd(State);
						State = '\0';
						break;
				}
		    }
			break;
		case TN_WILL: {
				unsigned char opt = input[*used];
				RecvCmd(TN_WILL, opt);
				if (!telnet_option_enabled(opt)) {
					send_telnet_command(TN_DONT, opt);
				} else {
					switch(opt) {
					case TN_ECHO:
						SocketFlags |= SOCKECHO;
						send_telnet_command(TN_DO, TN_ECHO);
						break;
					case TN_TTYPE:
						MttsCounter = 0;
						send_telnet_command(TN_DO, TN_TTYPE);
						break;
					case TN_COMPRESS:
						if (SentDoCompress2)
							send_telnet_command(TN_DONT, TN_COMPRESS);
						else
							send_telnet_command(TN_DO, TN_COMPRESS);
						break;
					case TN_COMPRESS2:
						send_telnet_command(TN_DO, TN_COMPRESS2);
						SentDoCompress2 = true;
						break;
					case TN_GMCP:
						send_telnet_command(TN_DO, TN_GMCP);
						start_gmcp();
						break;
					case TN_MSDP:
						send_telnet_command(TN_DO, TN_MSDP);
						start_msdp();
						break;
					case TN_MSSP:
						send_telnet_command(TN_DO, TN_MSSP);
						start_mssp();
						break;
					case TN_CHARSET:
						send_telnet_command(TN_DO, TN_CHARSET);
						request_charsets();
						break;
					default:
						send_telnet_command(TN_DO, opt);
						break;
					}
				}
				State = '\0';
			}
			break;
		case TN_WONT: {
				unsigned char opt = input[*used];
				RecvCmd(TN_WONT, opt);
				switch(input[*used]) {
				case TN_ECHO:
					SocketFlags &= ~SOCKECHO;
					send_telnet_command(TN_DONT, TN_ECHO);
					break;
				case TN_EOR_OPT:
					SocketFlags &= ~SOCKEOR;
					send_telnet_command(TN_DONT, TN_EOR);
					break;
				default:
					send_telnet_command(TN_DONT, opt);
					break;
				}
				State = '\0';
			}
			break;
		case TN_DO: {
				unsigned char opt = input[*used];
				RecvCmd(TN_DO, opt);
				if (!telnet_option_enabled(opt)) {
					send_telnet_command(TN_WONT, opt);
				} else {
					switch (opt) {
					case TN_ECHO:
						SocketFlags &= ~SOCKECHO;
						send_telnet_command(TN_WILL, TN_ECHO);
						break;
					case TN_NAWS:
						SocketFlags |= SOCKNAWS;
						send_telnet_command(TN_WILL, TN_NAWS);
						break;
					case TN_TTYPE:
						MttsCounter = 0;
						send_telnet_command(TN_WILL, TN_TTYPE);
						break;
					default:
						send_telnet_command(TN_WILL, opt);
						break;
					}
				}
				State = '\0';
			}
			break;
		case TN_DONT: {
				unsigned char opt = input[*used];
				RecvCmd(TN_DONT, opt);
				if (telnet_option_enabled(opt)) {
					switch (opt) {
					case TN_ECHO:
						SocketFlags |= SOCKECHO;
						send_telnet_command(TN_WONT, TN_ECHO);
						break;
					case TN_NAWS:
						SocketFlags &= ~SOCKNAWS;
						send_telnet_command(TN_WONT, TN_NAWS);
						break;
					default:
						send_telnet_command(TN_WONT, opt);
						break;
					}
				}
				State = '\0';
			}
			break;
		case TN_SB: {
				CurrentSubnegotiation = input[*used];
				SubnegotiationBuffer.clear();
				RecvCmd(TN_SB, CurrentSubnegotiation);
				State = '\0';
			}
			break;
		default:
			{
				unsigned char ch = input[*used];
				if (ch == TN_IAC) {
					if ( !(SocketFlags & SOCKTELNET) )
						SocketFlags |= SOCKTELNET;
					State = ch;
				} else if (CurrentSubnegotiation > 0) {
					if (CurrentSubnegotiation == TN_COMPRESS && ch == TN_WILL)
						State = TN_IAC;
					else
						SubnegotiationBuffer.push_back(ch);
				} else if (ch != 0x00) { // why so could happens?
					switch (ch) {
					case 0x9:
						if(capacity - (*generated) >= 4) {
							output[(*generated)++] = ' ';
							output[(*generated)++] = ' ';
							output[(*generated)++] = ' ';
							output[(*generated)++] = ' ';
						} else {
							return;
						}
						break;
					case 0x7:
						MessageBeep(MB_OK);
						break;
					default:
						if (bPromptEndEnabled) {
							if ((unsigned char)input[*used] == strPromptEndSeqBytes[PromptEndIndex]) {
								PromptEndIndex++;
								if (!strPromptEndSeqBytes[PromptEndIndex]) { //match!
									for (char *copy = (char*)strPromptEndReplBytes; *copy; )
										output[(*generated)++] = *(copy++);
									output[(*generated)++] = END_OF_PROMPT_MARK;
									PromptEndIndex = 0;
								}
								continue;
							}
							for (char *copy = (char*)strPromptEndSeqBytes; PromptEndIndex; PromptEndIndex--)
								output[(*generated)++] = *(copy++);
						}

						if (bIACReciveSingle || ch != 255) {
							output[(*generated)++] = ch;
						} else {
							State = ch;
						}
					}
				}
			}
		}
    }
}

static int sock_recv(int sock, char *dst, int nbytes) {
	struct timeval tm = {1,0};
	fd_set set;

	for (;;) {
		FD_ZERO(&set);
		FD_SET(sock, &set);
		if (select(sock + 1, &set, NULL, NULL, &tm) <= 0)
			return -1;
		if (FD_ISSET(sock, &set)) {
			int ret = tls_recv(sock, dst, nbytes);
			if (ret != 0)
				return ret;
		}
	}
}

static int sock_getline(int sock, char *dst, int maxlen) {
	char ch;
	int len = 0;
	for (;;) {
		int ret = sock_recv(sock, &ch, sizeof(ch));
		if (ret < 0)
			return ret;
		dst[len++] = ch;
		if (len + 1 >= maxlen)
			break;
		if (ch == '\n')
			break;
	}
	dst[len] = '\0';
	return len;
}

int telnet_init_session(int sock) {
	USES_CONVERSION;

	if (bWebsocketEnabled) {
		char key[16];
		char buf[BUFFER_SIZE], exts[BUFFER_SIZE], b64key[sizeof(key)*8/6 + 8];
		bool compressed = false;
		wchar_t msg[BUFFER_SIZE];

		srand( time(NULL) );
		for (int i = 0; i < sizeof(key); i++)
			key[i] = rand();
		base64_encode(b64key, sizeof(b64key), key, sizeof(key));
		
		buf[0] = '\0';
		if (mWebsocketOptions.find(L"permessage-deflate") != mWebsocketOptions.end()) {
			strcat(buf, "permessage-deflate");
		}
		if (strlen(buf) == 0)
			exts[0] = '\0';
		else
			sprintf(exts, "Sec-WebSocket-Extensions: %s\r\n", buf);
		
		sprintf(buf,
			"GET / HTTP/1.1\r\n"
            "Host: %s\r\n"
			"Origin: %s\r\n"
            "Upgrade: websocket\r\n"
			"Connection: Upgrade\r\n"
			"Sec-WebSocket-Version: %d\r\n"
			"Sec-WebSocket-Key: %s\r\n"
			"%s"
			"User-Agent: %s\r\n"
			"\r\n",

			W2A(mWebsocketOptions.find(L"Host") != mWebsocketOptions.end() ? mWebsocketOptions[L"Host"].c_str() : MUDHostName),
			W2A(mWebsocketOptions.find(L"Origin") != mWebsocketOptions.end() ? mWebsocketOptions[L"Origin"].c_str() : MUDHostName),
			WEBSOCKET_VERSION,
			b64key,
			exts,
			W2A(mWebsocketOptions.find(L"User-Agent") != mWebsocketOptions.end() ? mWebsocketOptions[L"User-Agent"].c_str() : strProductName)
		);
		tls_send(sock, buf, strlen(buf));

		int len = sock_getline(sock, buf, sizeof(buf));
		if (len <= 0)
			return -1;

		if (strncmp(buf, "HTTP/1.1 101 ", strlen("HTTP/1.1 101 "))) {
			tintin_puts(L"#websocket: unexpected server reply"); //TODO rs::rs
			if (bWebsocketDebugEnabled) {
				tintin_puts(A2W(buf));
			}
			return -1;
		}

		map<string, string> headers;
		for (;;) {
			int len = sock_getline(sock, buf, sizeof(buf));
			if (len <= 0)
				return -1;
			if (!strcmp(buf, "\r\n"))
				break;
			char *sep = strchr(buf, ':');
			if (sep) {
				char *ptr;
				*sep++ = '\0';
				if (*sep == ' ')
					sep++;
				for (ptr = buf; *ptr; ptr++)
					ptr[0] = tolower(ptr[0]);
				for (ptr = sep; *ptr; ptr++)
					ptr[0] = (ptr[0] == '\r' || ptr[0] == '\n') ? '\0' : ptr[0];
				headers[buf] = sep;
			} else if (bWebsocketDebugEnabled) {
				wsprintf(msg, L"#  (websocket non-header line)  %ls", A2W(buf));
				tintin_puts(msg);
			}
		}

		if (bWebsocketDebugEnabled) {
			for (map<string, string>::iterator it = headers.begin(); it != headers.end(); it++) {
				wsprintf(msg, L"#  (websocket header)  %ls: %ls", A2W(it->first.c_str()), A2W(it->second.c_str()));
				tintin_puts(msg);
			}
		}

		if (headers.find("connection") == headers.end() || stricmp(headers["connection"].c_str(), "upgrade") ||
			headers.find("upgrade") == headers.end() || stricmp(headers["upgrade"].c_str(), "websocket")) {
			tintin_puts(L"#websocket: wrong headers in server reply"); //TODO rs::rs
			return -1;
		}
		if (headers.find("sec-websocket-extensions") != headers.end()) {
			compressed = (headers["sec-websocket-extensions"].find("permessage-deflate") != string::npos);
		}

		SocketFlags |= SOCKWEBSOCKET;
		if (compressed) {
			init_ws_compression();
		}

		return 0;
	}

	return 0;
}

void telnet_close_session() {
	if (SocketFlags & SOCKWEBSOCKET) {
		send_websocket_frame(WS_OPCODE_CLOSE, 0, 0);
	}
	deinit_ws_compression();
	ws_ping_sent = 0;
}
