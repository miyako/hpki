#include "My-Number-Card_windows.h"

#pragma mark デバイス

void _sign_with_certificate_windows(Json::Value& threadCtx) {}
void _get_my_certificate_windows(Json::Value& threadCtx) {}
void _get_my_number_windows(Json::Value& threadCtx) {
    
    _parse_atr(threadCtx);
    
}
void _get_my_information_windows(Json::Value& threadCtx) {}

void _get_slots_windows(Json::Value& threadCtx) {
    
    Json::Value slotNames(Json::arrayValue);
    
    DWORD scope = SCARD_SCOPE_USER;
    
    SCARDCONTEXT hContext;
    LONG lResult = SCardEstablishContext(scope, NULL, NULL, &hContext);
    if (lResult == SCARD_E_NO_SERVICE) {
        HANDLE hEvent = SCardAccessStartedEvent();
        DWORD dwResult = WaitForSingleObject(hEvent, DEFAULT_TIMEOUT_MS_FOR_RESOURCE_MANAGER);
        if (dwResult == WAIT_OBJECT_0) {
            lResult = SCardEstablishContext(scope, NULL, NULL, &hContext);
        }
        SCardReleaseStartedEvent();
    }
    if (lResult == SCARD_S_SUCCESS) {
        DWORD len;
        lResult = SCardListReaders(hContext, SCARD_ALL_READERS, NULL, &len);
        if (lResult == SCARD_S_SUCCESS) {
            std::vector<TCHAR>buf(len);
            lResult = SCardListReaders(hContext, SCARD_ALL_READERS, &buf[0], &len);
            if (lResult == SCARD_S_SUCCESS) {
                LPTSTR pReader = (LPTSTR)&buf[0];
                if (pReader) {
                    while ('\0' != *pReader) {
                        std::wstring u16 = (const wchar_t *)pReader;
                        std::string u8;
                        u16_to_u8(u16, u8);
                        slotNames.append(u8);
                        pReader = pReader + wcslen(pReader) + 1;
                    }
                    threadCtx["success"] = true;
                }
            }
        }
        SCardReleaseContext(hContext);
    }


    threadCtx["readers"] = slotNames;
}

static bool _parse_atr(Json::Value& threadCtx) {
 
    int timeout = 3; //seconds

    std::string u8 = threadCtx["slotName"].asString();
    std::wstring u16;
    u8_to_u16(u8, u16);
    
    LPTSTR lpszReaderName = (LPTSTR)u16.c_str();
    
    DWORD mode = SCARD_SHARE_SHARED;
    DWORD scope = SCARD_SCOPE_USER;
    DWORD protocols = SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;

    SCARDCONTEXT hContext;
    LONG lResult = SCardEstablishContext(scope, NULL, NULL, &hContext);
    if (lResult == SCARD_E_NO_SERVICE) {
        HANDLE hEvent = SCardAccessStartedEvent();
        DWORD dwResult = WaitForSingleObject(hEvent, DEFAULT_TIMEOUT_MS_FOR_RESOURCE_MANAGER);
        if (dwResult == WAIT_OBJECT_0) {
            lResult = SCardEstablishContext(scope, NULL, NULL, &hContext);
        }
        SCardReleaseStartedEvent();
    }
    
    if (lResult == SCARD_S_SUCCESS) {
        SCARD_READERSTATE readerState;
        readerState.szReader = lpszReaderName;
        readerState.dwCurrentState = SCARD_STATE_UNAWARE;
        /* return immediately; check state */
        lResult = SCardGetStatusChange(hContext, 0, &readerState, 1);
        if (lResult == SCARD_S_SUCCESS) {
        
            int is_card_present = 0;
            
            time_t startTime = time(0);
            time_t anchorTime = startTime;
            
            bool isPolling = true;

            while (isPolling) {
                
                time_t now = time(0);
                time_t elapsedTime = abs(startTime - now);
                
                if(elapsedTime > 0)
                {
                    startTime = now;
                }
                
                elapsedTime = abs(anchorTime - now);
                
                if(elapsedTime < timeout) {

                    if (readerState.dwEventState & SCARD_STATE_EMPTY) {
                        lResult = SCardGetStatusChange(hContext, LIBPCSC_API_TIMEOUT, &readerState, 1);
                    }
                    
                    if (readerState.dwEventState & SCARD_STATE_UNAVAILABLE) {
                        isPolling = false;
                    }
                    
                    if (readerState.dwEventState & SCARD_STATE_PRESENT) {
                        is_card_present = 1;
                        isPolling = false;
                    }
                     
                }else{
                    /* timeout */
                    isPolling = false;
                }
                   
            }
            
            if(is_card_present) {
         
                SCARDHANDLE hCard;
                DWORD dwActiveProtocol;
                DWORD dwProtocol;
                DWORD dwAtrSize;
                DWORD dwState;
                DWORD dwAtrLen = 0;
                
                BYTE atr[256];
                
                lResult = SCardConnect(hContext,
                                       lpszReaderName,
                                       mode,
                                       protocols,
                                       &hCard,
                                       &dwActiveProtocol);
                switch (lResult)
                {
                    case (LONG)SCARD_W_REMOVED_CARD:
                        /* SCARD_W_REMOVED_CARD */
                        break;
                    case SCARD_S_SUCCESS:
                        lResult = SCardGetAttrib(hCard,
                                                 SCARD_ATTR_ATR_STRING, 
                                                 NULL,
                                                 &dwAtrLen);
                        if (lResult == SCARD_S_SUCCESS) {
                            std::vector<uint8_t>buf(dwAtrLen);
                            lResult = SCardGetAttrib(hCard,
                                                     SCARD_ATTR_ATR_STRING,
                                                     &buf[0],
                                                     &dwAtrLen);
                            if(lResult == SCARD_S_SUCCESS) {
                                
                                unsigned char T0 = buf[1];
                                unsigned char K = T0 & 0x0F;
                                if(dwAtrLen > K + 6) {
                                    std::string historicalBytes;
                                    bytes_to_hex((const uint8_t *)&buf[6], K, historicalBytes);
                                    threadCtx["historicalBytes"] = historicalBytes;
                                }
                            }
                        }
                        SCardDisconnect(hCard, SCARD_LEAVE_CARD);
                        break;
                    default:
                        break;
                }
            }
        }
        SCardReleaseContext(hContext);
    }
}

#ifdef __WINDOWS__
void u8_to_u16(std::string& u8, std::wstring& u16) {

    int len = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)u8.c_str(), u8.length(), NULL, 0);

    if (len) {
        std::vector<uint8_t> buf((len + 1) * sizeof(wchar_t));
        if (MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)u8.c_str(), u8.length(), (LPWSTR)&buf[0], len)) {
            u16 = std::wstring((const wchar_t*)&buf[0]);
        }
    }
    else {
        u16 = std::wstring((const wchar_t*)L"");
    }
}
void u16_to_u8(std::wstring& u16, std::string& u8) {

    int len = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)u16.c_str(), u16.length(), NULL, 0, NULL, NULL);

    if (len) {
        std::vector<uint8_t> buf(len + 1);
        if (WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)u16.c_str(), u16.length(), (LPSTR)&buf[0], len, NULL, NULL)) {
            u8 = std::string((const char*)&buf[0]);
        }
    }
    else {
        u8 = std::string((const char*)"");
    }
}
#endif
