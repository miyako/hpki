#include "My-Number-Card.h"

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
    BYTE pbRecvBuffer[258];
    DWORD cbRecvLength = sizeof(pbRecvBuffer);

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
    BYTE pbRecvBuffer[258];
    DWORD cbRecvLength = sizeof(pbRecvBuffer);

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
    BYTE pbRecvBuffer[258];
    DWORD cbRecvLength = sizeof(pbRecvBuffer);

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
    if (pin.length()) {
        memcpy(&data[5], pin.data(), pin.length());
    }

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

static void _read_block_basic4i(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value *threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_READ_BINARY));
    memcpy(&data[0],
        APDU_READ_BINARY,
        sizeof(APDU_READ_BINARY));

    int block = threadCtx->operator[]("block").asInt();
    int size = threadCtx->operator[]("size").asInt();

    data[2] = block;

    if (size < 0x100) {
        data[4] = size;
    }

    LPCBYTE pbSendBuffer = &data[0];
    DWORD cbSendLength = data.size();
    BYTE pbRecvBuffer[258];
    DWORD cbRecvLength = sizeof(pbRecvBuffer);

    LONG lResult;

    lResult = SCardTransmit(hCard,
        pioSendPci,
        pbSendBuffer,
        cbSendLength,
        NULL,
        pbRecvBuffer,
        &cbRecvLength);

    if (lResult == SCARD_S_SUCCESS) {

        int block = threadCtx->operator[]("block").asInt();
        int size = threadCtx->operator[]("size").asInt();

        std::string hex;
        if (size > 0xFF) {
            bytes_to_hex((const uint8_t*)pbRecvBuffer, 0x0100, hex);
        }
        else {
            bytes_to_hex((const uint8_t*)pbRecvBuffer, size, hex);
        }

        if (threadCtx->isMember("data")) {
            std::string _hex = threadCtx->operator[]("data").asString();
            threadCtx->operator[]("data") = _hex + hex;
        }
        else {
            threadCtx->operator[]("data") = hex;
        }

        size -= 0x100;
        block++;

        threadCtx->operator[]("size").operator=(size);
        threadCtx->operator[]("block").operator=(block);

        if (size > 0) {
            _read_block_basic4i(hCard, pioSendPci, threadCtx);
        }
        else {

            if (threadCtx->isMember("data")) {

                std::string hex = threadCtx->operator[]("data").asString();
                std::vector<uint8_t>buf(0);
                hex_to_bytes(hex, buf);
                bool boundary = false;
                bool field_id = false;
                uint8_t field_type = 0;

                for (size_t i = 0; i < buf.size(); ++i) {
                    if (buf[i] == 0xDF) {
                        boundary = true;
                        continue;
                    }
                    if (boundary) {
                        boundary = false;
                        field_id = true;
                        field_type = buf[i];
                        continue;
                    }
                    if (field_id) {
                        field_id = false;
                        uint8_t field_length = buf[i];
                        switch (field_type) {
                        case 0x21:
                            //header
//                            threadCtx["header"] = std::string((const char *)&buf[i+1], field_length);
                            break;
                        case 0x22:
                            //name
//                                    threadCtx["commonName"] = /*std::string((const char *)&buf[i+1], field_length)*/;
                            threadCtx->operator[]("commonName").operator=(std::string((const char*)&buf[i + 1], field_length));
                            break;
                        case 0x23:
                            //address
//                                    threadCtx["address"] = std::string((const char *)&buf[i+1], field_length);
                            threadCtx->operator[]("address").operator=(std::string((const char*)&buf[i + 1], field_length));
                            break;
                        case 0x24:
                            //dateOfBirth
//                                    threadCtx["dateOfBirth"] = /*std::string((const char *)&buf[i+1], field_length)*/;
                            threadCtx->operator[]("dateOfBirth").operator=(std::string((const char*)&buf[i + 1], field_length));
                            break;
                        case 0x25:
                            //sex
//                                    threadCtx["gender"] = std::string((const char *)&buf[i+1], field_length);
                            threadCtx->operator[]("gender").operator=(std::string((const char*)&buf[i + 1], field_length));
                            break;
                        default:
                            break;
                        }
                    }
                }
                //                    threadCtx["success"] = true;
                //                    threadCtx->operator[]("certificate").operator=(certificate);
                threadCtx->operator[]("success").operator=(true);
            }

        }
       
    }
}

static void _apdu_binary_read_basic4i(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    threadCtx["block"] = 0x00;

    _read_block_basic4i(hCard, pioSendPci, &threadCtx);

}

static void _apdu_binary_read_basic4i_length(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_READ_BINARY_GET_LENGTH));
    memcpy(&data[0],
        APDU_READ_BINARY_GET_LENGTH,
        sizeof(APDU_READ_BINARY_GET_LENGTH));

    LPCBYTE pbSendBuffer = &data[0];
    DWORD cbSendLength = data.size();
    BYTE pbRecvBuffer[258];
    DWORD cbRecvLength = sizeof(pbRecvBuffer);

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
            _apdu_binary_read_basic4i(hCard, pioSendPci, threadCtx);
        }
    }
}

static void _apdu_select_basic4i(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
        APDU_SELECT_EF_UNDER_DF,
        sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_BASIC4I_EF_JPKI_HI;
    data[6] = APDU_SELECT_BASIC4I_EF_JPKI_LO;

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_binary_read_basic4i_length(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_verify_app_aux_basic4i(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::string pin = threadCtx["pin4"].asString();

    std::vector<uint8_t>data(sizeof(APDU_VERIFY_PIN) + pin.length());
    memcpy(&data[0],
        APDU_VERIFY_PIN,
        sizeof(APDU_VERIFY_PIN));
    data[3] = APDU_VERIFY_PIN_EF_JPKI;
    data[4] = pin.length();
    if (pin.length()) {
        memcpy(&data[5], pin.data(), pin.length());
    }

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_select_basic4i(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_pin_aux_basic4i(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
        APDU_SELECT_EF_UNDER_DF,
        sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_PIN_CARD_EF_JPKI_HI;
    data[6] = APDU_SELECT_PIN_CARD_EF_JPKI_LO;

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_verify_app_aux_basic4i(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_app_aux_basic4i(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

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
        _apdu_select_pin_aux_basic4i(hCard, pioSendPci, threadCtx);
    }
}

static void _read_block_cert_jpki(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value* threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_READ_BINARY));
    memcpy(&data[0],
        APDU_READ_BINARY,
        sizeof(APDU_READ_BINARY));

    int block = threadCtx->operator[]("block").asInt();
    int size = threadCtx->operator[]("size").asInt();

    data[2] = block;

    if (size < 0x100) {
        data[4] = size;
    }

    LPCBYTE pbSendBuffer = &data[0];
    DWORD cbSendLength = data.size();
    BYTE pbRecvBuffer[258];
    DWORD cbRecvLength = sizeof(pbRecvBuffer);

    LONG lResult;

    lResult = SCardTransmit(hCard,
        pioSendPci,
        pbSendBuffer,
        cbSendLength,
        NULL,
        pbRecvBuffer,
        &cbRecvLength);

    if (lResult == SCARD_S_SUCCESS) {

        int block = threadCtx->operator[]("block").asInt();
        int size = threadCtx->operator[]("size").asInt();

        std::string hex;
        if (size > 0xFF) {
            bytes_to_hex((const uint8_t*)pbRecvBuffer, 0x0100, hex);
        }
        else {
            bytes_to_hex((const uint8_t*)pbRecvBuffer, size, hex);
        }

        if (threadCtx->isMember("data")) {
            std::string _hex = threadCtx->operator[]("data").asString();
            threadCtx->operator[]("data") = _hex + hex;
        }
        else {
            threadCtx->operator[]("data") = hex;
        }

        size -= 0x100;
        block++;

        threadCtx->operator[]("size").operator=(size);
        threadCtx->operator[]("block").operator=(block);

        if (size > 0) {
            _read_block_cert_jpki(hCard, pioSendPci, threadCtx);
        }
        else {

            if (threadCtx->isMember("data")) {
                Json::Value certificate(Json::objectValue);
                std::string hex = threadCtx->operator[]("data").asString();
                certificate["der"] = hex;

                std::vector<uint8_t>buf(0);
                hex_to_bytes(hex, buf);
                const uint8_t* p = &buf[0];
                X509* x509 = d2i_X509(NULL, &p, buf.size());
                if (x509) {
                    BIO* bio = BIO_new(BIO_s_mem());
                    if (PEM_write_bio_X509(bio, x509)) {
                        uint64_t len = BIO_number_written(bio) + 1;
                        std::vector<uint8_t>_buf(len);
                        if (BIO_read(bio, &_buf[0], (int)len)) {
                            certificate["pem"] = std::string((const char*)&_buf[0], len);
                        }
                    }
                    BIO_free(bio);

                   // X509_NAME* subject = X509_get_subject_name(x509);
                    certificate["subject"] = X509_NAME_oneline(X509_get_subject_name(x509), NULL, 0);

                   // X509_NAME* issuer = X509_get_issuer_name(x509);
                    certificate["issuer"] = X509_NAME_oneline(X509_get_issuer_name(x509), NULL, 0);

                    long serialNumber = ASN1_INTEGER_get(X509_get_serialNumber(x509));
                    long version = X509_get_version(x509);
                    certificate["serialNumber"] = (int)serialNumber;
                    certificate["version"] = (int)version;
                    ASN1_TIME* notBefore = X509_get_notBefore(x509);
                    ASN1_TIME* notAfter = X509_get_notAfter(x509);

                    std::string textValue;
                    asn_time_to_iso(notBefore, textValue);
                    certificate["notBefore"] = textValue;

                    asn_time_to_iso(notAfter, textValue);
                    certificate["notAfter"] = textValue;

                    X509_free(x509);
                }

                threadCtx->operator[]("certificate").operator=(certificate);
                threadCtx->operator[]("success").operator=(true);
            }

        }

    }
}

static void _apdu_binary_read_jpki_cert_identity(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    threadCtx["block"] = 0x00;

    _read_block_cert_jpki(hCard, pioSendPci, &threadCtx);

}

static void _apdu_binary_read_jpki_cert_identity_length(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_READ_BINARY_GET_LENGTH));
    memcpy(&data[0],
        APDU_READ_BINARY_GET_LENGTH,
        sizeof(APDU_READ_BINARY_GET_LENGTH));

    LPCBYTE pbSendBuffer = &data[0];
    DWORD cbSendLength = data.size();
    BYTE pbRecvBuffer[258];
    DWORD cbRecvLength = sizeof(pbRecvBuffer);

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
            _apdu_binary_read_jpki_cert_identity(hCard, pioSendPci, threadCtx);
        }
    }
}

static void _apdu_select_jpki_cert_identity(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
        APDU_SELECT_EF_UNDER_DF,
        sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_CRT_IDENTITY_EF_JPKI_HI;
    data[6] = APDU_SELECT_CRT_IDENTITY_EF_JPKI_LO;

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_binary_read_jpki_cert_identity_length(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_app_jpki_cert_identity(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::string hex = APDU_SELECT_IDENTITY_AP_JPKI;
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
        _apdu_select_jpki_cert_identity(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_hpki_cert_identity(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
        APDU_SELECT_EF_UNDER_DF,
        sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_CRT_EF_HPKI_HI;
    data[6] = APDU_SELECT_CRT_EF_HPKI_LO;

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_binary_read_jpki_cert_identity_length(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_app_hpki_cert_identity(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::string hex = APDU_SELECT_IDENTITY_AP_HPKI;
    std::vector<uint8_t>buf(0);
    hex_to_bytes(hex, buf);

    std::vector<uint8_t>data(sizeof(APDU_SELECT_FILE_DF) + buf.size());
    memcpy(&data[0],
        APDU_SELECT_FILE_DF,
        sizeof(APDU_SELECT_FILE_DF));

    data[4] = buf.size();
    memcpy(&data[5], &buf[0], buf.size());

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_select_hpki_cert_identity(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_hpki_cert_signature(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
        APDU_SELECT_EF_UNDER_DF,
        sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_CRT_EF_HPKI_HI;
    data[6] = APDU_SELECT_CRT_EF_HPKI_LO;

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_binary_read_jpki_cert_identity_length(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_app_hpki_cert_signature(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::string hex = APDU_SELECT_SIGNATURE_AP_HPKI;
    std::vector<uint8_t>buf(0);
    hex_to_bytes(hex, buf);

    std::vector<uint8_t>data(sizeof(APDU_SELECT_FILE_DF) + buf.size());
    memcpy(&data[0],
        APDU_SELECT_FILE_DF,
        sizeof(APDU_SELECT_FILE_DF));

    data[4] = buf.size();
    memcpy(&data[5], &buf[0], buf.size());

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_select_hpki_cert_signature(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_binary_read_jpki_cert_signature(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    threadCtx["block"] = 0x00;

    _read_block_cert_jpki(hCard, pioSendPci, &threadCtx);

}

static void _apdu_binary_read_jpki_cert_signature_length(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_READ_BINARY_GET_LENGTH));
    memcpy(&data[0],
        APDU_READ_BINARY_GET_LENGTH,
        sizeof(APDU_READ_BINARY_GET_LENGTH));

    LPCBYTE pbSendBuffer = &data[0];
    DWORD cbSendLength = data.size();
    BYTE pbRecvBuffer[258];
    DWORD cbRecvLength = sizeof(pbRecvBuffer);

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
            _apdu_binary_read_jpki_cert_signature(hCard, pioSendPci, threadCtx);
        }
    }
}

static void _apdu_select_jpki_cert_signature(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
        APDU_SELECT_EF_UNDER_DF,
        sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_CRT_SIGNATURE_EF_JPKI_HI;
    data[6] = APDU_SELECT_CRT_SIGNATURE_EF_JPKI_LO;

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_binary_read_jpki_cert_signature_length(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_verify_app_jpki_cert_signature(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::string pin = threadCtx["pin6"].asString();

    std::vector<uint8_t>data(sizeof(APDU_VERIFY_PIN) + pin.length());
    memcpy(&data[0],
        APDU_VERIFY_PIN,
        sizeof(APDU_VERIFY_PIN));
    data[3] = APDU_VERIFY_PIN_EF_JPKI;
    data[4] = pin.length();
    if (pin.length()) {
        memcpy(&data[5], pin.data(), pin.length());
    }

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_select_jpki_cert_signature(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_pin_jpki_cert_signature(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
        APDU_SELECT_EF_UNDER_DF,
        sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_PIN_SIGNATURE_EF_JPKI_HI;
    data[6] = APDU_SELECT_PIN_SIGNATURE_EF_JPKI_LO;

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_verify_app_jpki_cert_signature(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_app_jpki_cert_signature(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::string hex = APDU_SELECT_IDENTITY_AP_JPKI;
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
        _apdu_select_pin_jpki_cert_signature(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_compute_digital_signature_jpki(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    hash_algorithm algorithm = (hash_algorithm)threadCtx["algorithm"].asInt();
    size_t APDU_size;
    switch (algorithm) {
        case hash_algorithm_sha512:
            APDU_size = sizeof(APDU_COMPUTE_DIGITAL_SIGNATURE_KEY_JPKI);
            break;
        case hash_algorithm_sha384:
            APDU_size = sizeof(APDU_COMPUTE_DIGITAL_SIGNATURE_KEY_JPKI) - 0x10;
            break;
        case hash_algorithm_sha1:
            APDU_size = sizeof(APDU_COMPUTE_DIGITAL_SIGNATURE_KEY_JPKI) - 0x30;
            break;
        default:
            APDU_size = sizeof(APDU_COMPUTE_DIGITAL_SIGNATURE_KEY_JPKI) - 0x20;
            break;
    }
    
    std::vector<uint8_t>data(sizeof(APDU_COMPUTE_DIGITAL_SIGNATURE_KEY_JPKI));
    memcpy(&data[0],
        APDU_COMPUTE_DIGITAL_SIGNATURE_KEY_JPKI,
        sizeof(APDU_COMPUTE_DIGITAL_SIGNATURE_KEY_JPKI));
    std::string digestInfo = threadCtx["digestInfo"].asString();
    std::vector<uint8_t>buf(0);
    hex_to_bytes(digestInfo, buf);
    data[4] = buf.size();
    memcpy(&data[5], &buf[0], buf.size());

    LPCBYTE pbSendBuffer = &data[0];
    DWORD cbSendLength = APDU_size;
    BYTE pbRecvBuffer[258];
    DWORD cbRecvLength = sizeof(pbRecvBuffer);

    LONG lResult;

    lResult = SCardTransmit(hCard,
        pioSendPci,
        pbSendBuffer,
        cbSendLength,
        NULL,
        pbRecvBuffer,
        &cbRecvLength);

    if (lResult == SCARD_S_SUCCESS) {
    
        size_t signature_length = cbRecvLength - 2;
        std::vector<uint8_t>buf(signature_length);

        memcpy(&buf[0], &pbRecvBuffer[0], signature_length);

        std::string hex;
        bytes_to_hex(&buf[0], signature_length, hex);
        threadCtx["signature"] = hex;
        std::string b64;
        bytes_to_b64(&buf[0], signature_length, b64);
        threadCtx["signature_base64"] = b64;
        threadCtx["success"] = true;
    }

}

static void _apdu_select_jpki_key_identity(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
        APDU_SELECT_EF_UNDER_DF,
        sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_KEY_IDENTITY_EF_JPKI_HI;
    data[6] = APDU_SELECT_KEY_IDENTITY_EF_JPKI_LO;

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_compute_digital_signature_jpki(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_verify_app_jpki_compute_digital_signature_identity(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::string pin = threadCtx["pin4"].asString();

    std::vector<uint8_t>data(sizeof(APDU_VERIFY_PIN) + pin.length());
    memcpy(&data[0],
        APDU_VERIFY_PIN,
        sizeof(APDU_VERIFY_PIN));
    data[3] = APDU_VERIFY_PIN_EF_JPKI;
    data[4] = pin.length();
    if (pin.length()) {
        memcpy(&data[5], pin.data(), pin.length());
    }

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_select_jpki_key_identity(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_pin_jpki_compute_digital_signature_identity(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
        APDU_SELECT_EF_UNDER_DF,
        sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_PIN_IDENTITY_EF_JPKI_HI;
    data[6] = APDU_SELECT_PIN_IDENTITY_EF_JPKI_LO;

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_verify_app_jpki_compute_digital_signature_identity(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_app_jpki_compute_digital_signature_identity(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::string hex = APDU_SELECT_IDENTITY_AP_JPKI;
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
        _apdu_select_pin_jpki_compute_digital_signature_identity(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_compute_digital_signature_hpki(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    size_t key_length_bits = 2048;
    size_t key_length = key_length_bits / 8;
    std::vector<uint8_t>pkcs(key_length);
        
    pkcs[0] = 0x00;
    pkcs[1] = 0x01;
    
    //padding string
    memset(&pkcs[2],
           0xFF,
           pkcs.size() - 2);

    std::string digestInfo = threadCtx["digestInfo"].asString();
    std::vector<uint8_t>buf(0);
    hex_to_bytes(digestInfo, buf);
    
    size_t p = pkcs.size() - buf.size();
    memcpy(&pkcs[p], &buf[0], buf.size());
    pkcs[p - 1] = 0x00;
    
    std::vector<uint8_t>data(7 + pkcs.size()+ 2);
    
    data[0] = 0x00;
    data[1] = 0x2A;
    data[2] = 0x9E;
    data[3] = 0x9A;
        
    data[4] = 0x00;
    data[5] = key_length >> 8;
    data[6] = key_length & 0x00FF;
    
    memcpy(&data[7], &pkcs[0], pkcs.size());
    
    data[data.size()-2] = 0x00;
    data[data.size()-1] = 0x00;
    
    std::string apdu;
    bytes_to_hex(&data[0], data.size(), apdu);
    threadCtx["apdu_pkcs"] = apdu;

    LPCBYTE pbSendBuffer = &data[0];
    DWORD cbSendLength = APDU_size;
    BYTE pbRecvBuffer[258];
    DWORD cbRecvLength = sizeof(pbRecvBuffer);

    LONG lResult;

    lResult = SCardTransmit(hCard,
        pioSendPci,
        pbSendBuffer,
        cbSendLength,
        NULL,
        pbRecvBuffer,
        &cbRecvLength);

    if (lResult == SCARD_S_SUCCESS) {

        size_t signature_length = cbRecvLength - 2;
        std::vector<uint8_t>buf(signature_length);

        memcpy(&buf[0], &pbRecvBuffer[0], signature_length);

        std::string hex;
        bytes_to_hex(&buf[0], signature_length, hex);
        threadCtx["signature"] = hex;
        std::string b64;
        bytes_to_b64(&buf[0], signature_length, b64);
        threadCtx["signature_base64"] = b64;
        threadCtx["success"] = true;
    }

}

static void _apdu_select_hpki_key(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
        APDU_SELECT_EF_UNDER_DF,
        sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_KEY_EF_HPKI_HI;
    data[6] = APDU_SELECT_KEY_EF_HPKI_LO;

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_compute_digital_signature_hpki(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_verify_app_hpki_compute_digital_signature_identity(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::string pin = threadCtx["pin4"].asString();

    std::vector<uint8_t>data(sizeof(APDU_VERIFY_PIN) + pin.length());
    memcpy(&data[0],
        APDU_VERIFY_PIN,
        sizeof(APDU_VERIFY_PIN));
    data[3] = APDU_VERIFY_PIN_EF_HPKI;
    data[4] = pin.length();
    if (pin.length()) {
        memcpy(&data[5], pin.data(), pin.length());
    }

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_compute_digital_signature_hpki(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_hpki_key_identity(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(MANAGE_SECURITY_ENVIRONMENT));
    memcpy(&data[0],
           MANAGE_SECURITY_ENVIRONMENT,
           sizeof(MANAGE_SECURITY_ENVIRONMENT));

    std::string mse;
    bytes_to_hex(&data[0], data.size(), mse);
    threadCtx["mse"] = mse;
    
    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_verify_app_hpki_compute_digital_signature_identity(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_hpki_key_identity(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
           APDU_SELECT_EF_UNDER_DF,
           sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_KEY_EF_HPKI_HI;
    data[6] = APDU_SELECT_KEY_EF_HPKI_LO;

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_set_security_environment_hpki_identity(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_set_security_environment_hpki_signature(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(MANAGE_SECURITY_ENVIRONMENT));
    memcpy(&data[0],
           MANAGE_SECURITY_ENVIRONMENT,
           sizeof(MANAGE_SECURITY_ENVIRONMENT));

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_verify_app_hpki_compute_digital_signature(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_hpki_key_signature(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
           APDU_SELECT_EF_UNDER_DF,
           sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_KEY_EF_HPKI_HI;
    data[6] = APDU_SELECT_KEY_EF_HPKI_LO;

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_set_security_environment_hpki_signature(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_pin_hpki_compute_digital_signature(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
           APDU_SELECT_EF_UNDER_DF,
           sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_PIN_EF_HPKI_HI;
    data[6] = APDU_SELECT_PIN_EF_HPKI_LO;

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_select_hpki_key_signature(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_pin_hpki_compute_digital_signature_identity(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
        APDU_SELECT_EF_UNDER_DF,
        sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_PIN_EF_HPKI_HI;
    data[6] = APDU_SELECT_PIN_EF_HPKI_LO;

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_select_hpki_key_identity
        /*_apdu_verify_app_hpki_compute_digital_signature_identity*/(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_app_hpki_compute_digital_signature_identity(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::string hex = APDU_SELECT_IDENTITY_AP_HPKI;
    std::vector<uint8_t>buf(0);
    hex_to_bytes(hex, buf);

    std::vector<uint8_t>data(sizeof(APDU_SELECT_FILE_DF) + buf.size());
    memcpy(&data[0],
        APDU_SELECT_FILE_DF,
        sizeof(APDU_SELECT_FILE_DF));

    data[4] = buf.size();
    memcpy(&data[5], &buf[0], buf.size());

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_select_pin_hpki_compute_digital_signature_identity(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_jpki_key_signature(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
        APDU_SELECT_EF_UNDER_DF,
        sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_KEY_SIGNATURE_EF_JPKI_HI;
    data[6] = APDU_SELECT_KEY_SIGNATURE_EF_JPKI_LO;

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_compute_digital_signature_jpki(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_verify_app_jpki_compute_digital_signature(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::string pin = threadCtx["pin6"].asString();

    std::vector<uint8_t>data(sizeof(APDU_VERIFY_PIN) + pin.length());
    memcpy(&data[0],
        APDU_VERIFY_PIN,
        sizeof(APDU_VERIFY_PIN));
    data[3] = APDU_VERIFY_PIN_EF_JPKI;
    data[4] = pin.length();
    if (pin.length()) {
        memcpy(&data[5], pin.data(), pin.length());
    }

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_select_jpki_key_signature(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_pin_jpki_compute_digital_signature(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
        APDU_SELECT_EF_UNDER_DF,
        sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_PIN_SIGNATURE_EF_JPKI_HI;
    data[6] = APDU_SELECT_PIN_SIGNATURE_EF_JPKI_LO;

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_verify_app_jpki_compute_digital_signature(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_app_jpki_compute_digital_signature(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::string hex = APDU_SELECT_IDENTITY_AP_JPKI;
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
        _apdu_select_pin_jpki_compute_digital_signature(hCard, pioSendPci, threadCtx);
    }
}

static void _apdu_select_app_hpki_compute_digital_signature(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx) {

    std::string hex = APDU_SELECT_IDENTITY_AP_HPKI;
    std::vector<uint8_t>buf(0);
    hex_to_bytes(hex, buf);

    std::vector<uint8_t>data(sizeof(APDU_SELECT_FILE_DF) + buf.size());
    memcpy(&data[0],
        APDU_SELECT_FILE_DF,
        sizeof(APDU_SELECT_FILE_DF));

    data[4] = buf.size();
    memcpy(&data[5], &buf[0], buf.size());

    if (_transmit_request(hCard, pioSendPci, data, threadCtx)) {
        _apdu_select_pin_hpki_compute_digital_signature(hCard, pioSendPci, threadCtx);
    }
}

typedef void (*apdu_api_t)(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pioSendPci, Json::Value& threadCtx);

static void _connect(Json::Value& threadCtx, apdu_api_t api){

    std::string u8 = threadCtx["slotName"].asString();

    std::wstring slotName;
    u8_to_u16(u8, slotName);

    LPTSTR lpszReaderName = (LPTSTR)slotName.c_str();

    DWORD protocols = SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;
    DWORD mode = SCARD_SHARE_SHARED;
    DWORD scope = SCARD_SCOPE_USER;
    int timeout = 1; //seconds

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

void _sign_with_certificate_windows(Json::Value& threadCtx) {

    _parse_atr(threadCtx);

    pki_type_t pki_type = (pki_type_t)threadCtx["type"].asInt();

    if (threadCtx["certificateType"].asInt() == certificate_type_identity) {
        switch (pki_type) {
        case pki_type_j:
            _connect(threadCtx, _apdu_select_app_jpki_compute_digital_signature_identity);
            break;
        case pki_type_h:
            _connect(threadCtx, _apdu_select_app_hpki_compute_digital_signature_identity);
            break;
        default:
            break;
        }
    }
    else {
        switch (pki_type) {
        case pki_type_j:
            _connect(threadCtx, _apdu_select_app_jpki_compute_digital_signature);
            break;
        case pki_type_h:
            _connect(threadCtx, _apdu_select_app_hpki_compute_digital_signature);
            break;
        default:
            break;
        }
    }
}

void _get_my_certificate_windows(Json::Value& threadCtx) {

    _parse_atr(threadCtx);

    pki_type_t pki_type = (pki_type_t)threadCtx["type"].asInt();

    if (threadCtx["certificateType"].asInt() == certificate_type_identity) {
        switch (pki_type) {
        case pki_type_j:
            _connect(threadCtx, _apdu_select_app_jpki_cert_identity);
            break;
        case pki_type_h:
            _connect(threadCtx, _apdu_select_app_hpki_cert_identity);
            break;
        default:
            break;
        }
    }
    else {
        switch (pki_type) {
        case pki_type_j:
            _connect(threadCtx, _apdu_select_app_jpki_cert_signature);
            break;
        case pki_type_h:
            _connect(threadCtx, _apdu_select_app_hpki_cert_signature);
            break;
        default:
            break;
        }
    }
}

void _get_my_number_windows(Json::Value& threadCtx) {
    
    _parse_atr(threadCtx);

    pki_type_t pki_type = (pki_type_t)threadCtx["type"].asInt();

    if (pki_type == pki_type_j) {
        _connect(threadCtx, _apdu_select_app_aux_my_number);
    }
}

void _get_my_information_windows(Json::Value& threadCtx) {

    _parse_atr(threadCtx);

    pki_type_t pki_type = (pki_type_t)threadCtx["type"].asInt();

    if (pki_type == pki_type_j) {
        _connect(threadCtx, _apdu_select_app_aux_basic4i);
    }
}

void _get_slots_windows(Json::Value& threadCtx) {
    
    Json::Value slotNames(Json::arrayValue);
    
    DWORD scope = SCARD_SCOPE_USER;
    
    SCARDCONTEXT hContext;
    LONG lResult = SCardEstablishContext(scope, NULL, NULL, &hContext);
    if (lResult == SCARD_E_NO_SERVICE) {
        HANDLE hEvent = SCardAccessStartedEvent();
        DWORD dwResult = WaitForSingleObject(hEvent, DEFAULT_TIMEOUT_MS_FOR_RESOURCE_MANAGER_1ST);
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
 
    int timeout = 1; //seconds

    std::string u8 = threadCtx["slotName"].asString();

    std::wstring slotName;
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
        if (historicalBytes == ID_JGID) {
            return pki_type_j;
        }
    }
    
    return pki_type_unknown;
}
