#include "My-Number-Card_windows.h"

#pragma mark デバイス

static bool _is_response_positive(BYTE SW1,BYTE SW2,
    Json::Value& threadCtx) {

    if ((SW1 == 0x90) && (SW2 == 0x00)) return true;

    std::string hi, lo;

    bytes_to_hex(&SW1, sizeof(uint8_t), hi);
    bytes_to_hex(&SW2, sizeof(uint8_t), lo);

    threadCtx["response"] = hi + lo;

    return false;
}

static bool _transmit_request(SCARDHANDLE hCard, 
    const SCARD_IO_REQUEST* pioSendPci, 
    std::vector<uint8_t>& data, 
    Json::Value& threadCtx) {

    LPCBYTE pbSendBuffer = &data[0];
    DWORD cbSendLength = data.size();
    BYTE pbRecvBuffer[256];
    DWORD cbRecvLength = 256;

    LONG lResult;

    lResult = SCardTransmit(hCard,
        pioSendPci,
        pbSendBuffer,
        cbSendLength,
        NULL,
        pbRecvBuffer,
        &cbRecvLength);

    if (lResult == SCARD_S_SUCCESS) {

        BYTE SW1 = pbRecvBuffer[cbRecvLength - 2];
        BYTE SW2 = pbRecvBuffer[cbRecvLength - 1];

        return _is_response_positive(SW1, SW2, threadCtx);
    }

    return false;
}

static int _get_data_size(std::vector<uint8_t>& size) {

    if (size.size() != 4) {
        return 0;
    }
    else {
        int data_length_length, data_length, offset = 2;
        if ((size[1] >> 7) == 0) {
            data_length_length = 1;
            data_length = size[2];
            offset++;
        }
        else {
            data_length = 0;
            data_length_length = size[1] & 0x7f;
            for (int i = 2; i < data_length_length + 2; ++i) {
                data_length = (data_length << 8) + size[i];
                offset++;
            }
        }
        return offset + data_length;
    }
}

static void _apdu_binary_read_my_number(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    int size = threadCtx["size"].asInt();

    std::vector<uint8_t>data(sizeof(APDU_READ_BINARY));
    memcpy(&data[0],
        APDU_READ_BINARY,
        sizeof(APDU_READ_BINARY));
    data[4] = size;

    LPCBYTE pbSendBuffer = &data[0];
    DWORD cbSendLength = data.size();
    BYTE pbRecvBuffer[256];
    DWORD cbRecvLength = 256;

    LONG lResult;

    lResult = SCardTransmit(hCard,
        pioSendPci,
        pbSendBuffer,
        cbSendLength,
        NULL,
        pbRecvBuffer,
        &cbRecvLength);

    if (lResult == SCARD_S_SUCCESS) {

        BYTE SW1 = pbRecvBuffer[cbRecvLength - 2];
        BYTE SW2 = pbRecvBuffer[cbRecvLength - 1];

        if (_is_response_positive(SW1, SW2, threadCtx)) {
            std::vector<uint8_t>buf(size);
            memcpy(&buf[0],
                &pbRecvBuffer[0],
                size);
            int len = buf[2];
            threadCtx["myNumber"] = std::string((const char*)&buf[3], len);
            threadCtx["success"] = true;
        }
    }
}

static void _apdu_binary_read_my_number_length(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_READ_BINARY_GET_LENGTH));
    memcpy(&data[0],
        APDU_READ_BINARY_GET_LENGTH,
        sizeof(APDU_READ_BINARY_GET_LENGTH));

    LPCBYTE pbSendBuffer = &data[0];
    DWORD cbSendLength = data.size();
    BYTE pbRecvBuffer[256];
    DWORD cbRecvLength = 256;

    LONG lResult;

    lResult = SCardTransmit(hCard,
        pioSendPci,
        pbSendBuffer,
        cbSendLength,
        NULL,
        pbRecvBuffer,
        &cbRecvLength);

    if (lResult == SCARD_S_SUCCESS) {

        BYTE SW1 = pbRecvBuffer[cbRecvLength - 2];
        BYTE SW2 = pbRecvBuffer[cbRecvLength - 1];

        if (_is_response_positive(SW1, SW2, threadCtx)) {

            std::vector<uint8_t>size(4);

            memcpy(&size[0],
                &pbRecvBuffer[0],
                size.size());

            threadCtx["size"] = _get_data_size(size);
            _apdu_binary_read_my_number(hCard, pioSendPci, threadCtx);
        }
    }
}

static void _apdu_select_my_number(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
        APDU_SELECT_EF_UNDER_DF,
        sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_MNUMBER_EF_JPKI_HI;
    data[6] = APDU_SELECT_MNUMBER_EF_JPKI_LO;

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_binary_read_my_number_length(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_verify_app_aux_my_number(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::string pin = threadCtx["pin4"].asString();

    std::vector<uint8_t>data(sizeof(APDU_VERIFY_PIN) + pin.length());
    memcpy(&data[0],
        APDU_VERIFY_PIN,
        sizeof(APDU_VERIFY_PIN));
    data[3] = APDU_VERIFY_PIN_EF_JPKI;
    data[4] = pin.length();
    memcpy(&data[5], pin.data(), pin.length());

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_select_my_number(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_pin_aux_my_number(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
        APDU_SELECT_EF_UNDER_DF,
        sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_PIN_CARD_EF_JPKI_HI;
    data[6] = APDU_SELECT_PIN_CARD_EF_JPKI_LO;

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_verify_app_aux_my_number(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_app_aux_my_number(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {
    
    std::string hex = APDU_SELECT_CARD_AP_JPKI;
    std::vector<uint8_t>buf(0);
    hex_to_bytes(hex, buf);
    std::vector<uint8_t>data(sizeof(APDU_SELECT_FILE_DF) + buf.size() - 1);
    memcpy(&data[0],
        APDU_SELECT_FILE_DF,
        sizeof(APDU_SELECT_FILE_DF));
    data[3] = APDU_SELECT_FILE_DF_P2_RFU_JPKI;
    data[4] = buf.size();
    memcpy(&data[5], &buf[0], buf.size());

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_select_pin_aux_my_number(hCard, pioSendPci, threadCtx);
    } 
}

typedef void (*apdu_api_t)(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx);

static void _connect(Json::Value& threadCtx, apdu_api_t api){

    std::wstring slotName;
    _parse_atr(threadCtx, slotName);
    LPTSTR lpszReaderName = (LPTSTR)slotName.c_str();

    pki_type_t pki_type = (pki_type_t)threadCtx["type"].asInt();

    if (pki_type == pki_type_j) {
        DWORD protocols = SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;
        DWORD mode = SCARD_SHARE_SHARED;
        DWORD scope = SCARD_SCOPE_USER;
        int timeout = 3; //seconds

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
            lResult = SCardGetStatusChange(hContext, 0, &readerState, 1);
            if (lResult == SCARD_S_SUCCESS) {
                int is_card_present = 0;
                time_t startTime = time(0);
                time_t anchorTime = startTime;
                bool isPolling = true;

                while (isPolling) {
                    time_t now = time(0);
                    time_t elapsedTime = abs(startTime - now);
                    if (elapsedTime > 0)
                    {
                        startTime = now;
                    }
                    elapsedTime = abs(anchorTime - now);
                    if (elapsedTime < timeout) {

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

                    }
                    else {
                        /* timeout */
                        isPolling = false;
                    }

                }

                if (is_card_present) {

                    SCARDHANDLE hCard;
                    DWORD dwActiveProtocol;
                    DWORD dwProtocol;
                    DWORD dwAtrSize;
                    DWORD dwState;

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
                        lResult = SCardStatus(hCard, NULL, NULL, &dwState, &dwProtocol, atr, &dwAtrSize);
                        if (lResult == SCARD_S_SUCCESS) {
                            api(hCard, SCARD_PCI_T1, threadCtx);
                            threadCtx["success"] = threadCtx["success"].asBool();
                        }/* SCardStatus */
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
}

void _sign_with_certificate_windows(Json::Value& threadCtx) {}
void _get_my_certificate_windows(Json::Value& threadCtx) {}
void _get_my_number_windows(Json::Value& threadCtx) {
    
    _connect(threadCtx, _apdu_select_app_aux_my_number);

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

static bool _parse_atr(Json::Value& threadCtx, std::wstring& slotName) {
 
    int timeout = 3; //seconds

    std::string u8 = threadCtx["slotName"].asString();

    u8_to_u16(u8, slotName);
    
    LPTSTR lpszReaderName = (LPTSTR)slotName.c_str();
    
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
                                if (dwAtrLen > K) {
                                    std::string historicalBytes;
                                    bytes_to_hex((const uint8_t*)&buf[dwAtrLen - K - 1], K, historicalBytes);
                                    threadCtx["historicalBytes"] = historicalBytes;
                                    threadCtx["type"] = _get_pki_type(threadCtx);
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
    return true;
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

static pki_type_t _get_pki_type(Json::Value& threadCtx) {
    
    if(threadCtx["historicalBytes"].isString()){
        std::string historicalBytes = threadCtx["historicalBytes"].asString();
        if(historicalBytes == ID_HPKI) {
            return pki_type_h;
        }
        if(historicalBytes == ID_JPKI) {
            return pki_type_j;
        }
    }
    
    return pki_type_unknown;
}
