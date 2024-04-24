#include "My-Number-Card_windows.h"

#pragma mark デバイス

void _get_slots_windows(Json::Value& threadCtx) {
    
    Json::Value slotNames(Json::arrayValue);
    
    uint32_t scope = threadCtx["scope"];
    
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
                        slotNames.append(u16);
                        pReader = pReader + wcslen(pReader) + 1;
                    }
                    threadCtx["success"] = true;
                }
            }
        }
        SCardReleaseContext(hContext);
    }
}

static bool _parse_atr(Json::Value& threadCtx) {
 
    std::wstring slotName = threadCtx["slotName"].asString();
    
    LPTSTR lpszReaderName = (LPTSTR)slotName.c_str();
    
    uint32_t scope = threadCtx["scope"];
    
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
                        if (rv == SCARD_S_SUCCESS) {
                            std::vector<uint8_t>buf(dwAtrLen);
                            lResult = SCardGetAttrib(hCard,
                                                     SCARD_ATTR_ATR_STRING,
                                                     &buf[0],
                                                     &dwAtrLen);
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
