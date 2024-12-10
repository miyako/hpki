#include "My-Number-Card.h"

static bool _parse_atr(Json::Value& threadCtx) {
    
    std::string slotName = threadCtx["slotName"].asString();
    
    TKSmartCardSlotManager *manager = [TKSmartCardSlotManager defaultManager];

    if(manager) {
        NSString *name = [[NSString alloc]initWithUTF8String:slotName.c_str()];
        TKSmartCardSlot *slot = [manager slotNamed:name];
        [name release];
        if(slot) {
            TKSmartCardATR *ATR = [slot ATR];
            if(ATR) {
                NSData *_historicalBytes = [ATR historicalBytes];
                std::string historicalBytes;
                bytes_to_hex((const uint8_t *)[_historicalBytes bytes], [_historicalBytes length], historicalBytes);
                threadCtx["historicalBytes"] = historicalBytes;
                return true;
            }else{
                threadCtx["error"] = "TKSmartCardSlot::ATR() failed";
            }
        }else{
            threadCtx["error"] = "TKSmartCardSlotManager::slotNamed() failed";
        }
    }else{
        threadCtx["error"] = "TKSmartCardSlotManager::defaultManager() failed";
    }
    
    return false;
}

static TKSmartCard *_select_slot(Json::Value& threadCtx) {
    
    std::string slotName = threadCtx["slotName"].asString();
    
    TKSmartCardSlotManager *manager = [TKSmartCardSlotManager defaultManager];

    if(manager) {
        NSString *name = [[NSString alloc]initWithUTF8String:slotName.c_str()];
        TKSmartCardSlot *slot = [manager slotNamed:name];
        [name release];
        if(slot) {
            _parse_atr(threadCtx);
            threadCtx["type"] = _get_pki_type(threadCtx);
            return [slot makeSmartCard];
        }else{
            threadCtx["error"] = "TKSmartCardSlotManager::slotNamed() failed";
        }
    }else{
        threadCtx["error"] = "TKSmartCardSlotManager::defaultManager() failed";
    }
    
    return NULL;
}

static bool _check_access(Json::Value& threadCtx) {
        
    bool entitlement = false;
    
    /*
     
     https://developer.apple.com/documentation/bundleresources/entitlements/com_apple_security_smartcard
     
     */

#ifndef _CONSOLE_
    
    SecTaskRef sec = SecTaskCreateFromSelf(kCFAllocatorMalloc);
    CFErrorRef err = nil;
    CFBooleanRef boolValue = (CFBooleanRef)SecTaskCopyValueForEntitlement(sec,
                                                                          CFSTR("com.apple.security.smartcard"),
                                                                          &err);
    if(!err) {
        if(boolValue) {
            entitlement = CFBooleanGetValue(boolValue);
        }
    }
    
    CFRelease(sec);
    
    if(!entitlement) {
        threadCtx["error"] = "com.apple.security.smartcard is missing in app entitlement";
    }
    
    return entitlement;
#else
    return true;
#endif
}

static bool _is_response_positive(NSData *response, 
                                  Json::Value& threadCtx) {
    
    uint8_t SW1 = 0;
    uint8_t SW2 = 0;
    [response getBytes:&SW1 range:NSMakeRange([response length] - 2, 1)];
    [response getBytes:&SW2 range:NSMakeRange([response length] - 1, 1)];
        
    if ((SW1 == 0x90) && (SW2 == 0x00)) return true;
    
    std::string hi, lo;
    
    bytes_to_hex(&SW1, sizeof(uint8_t), hi);
    bytes_to_hex(&SW2, sizeof(uint8_t), lo);
        
    threadCtx["response"] = hi + lo;
    
    return false;
}

static void _end_session(dispatch_semaphore_t sem, 
                         TKSmartCard *smartCard,
                         Json::Value& threadCtx) {

    [smartCard endSession];
    dispatch_semaphore_signal(sem);

}

#pragma mark デバイス

void _get_slots_macos(Json::Value& threadCtx) {
    
    Json::Value slotNames(Json::arrayValue);
    
    if(_check_access(threadCtx)) {
                
        TKSmartCardSlotManager *manager = [TKSmartCardSlotManager defaultManager];
        if(manager) {
            NSArray<NSString *> *_slotNames = [manager slotNames];
            for (NSString *slotName in _slotNames) {
                std::string u8 = (const char *)[slotName UTF8String];
                slotNames.append(u8);
            }
            threadCtx["readers"] = slotNames;
            threadCtx["success"] = true;
        }else{
            threadCtx["error"] = "TKSmartCardSlotManager::defaultManager() failed";
        }
    }
}
#pragma mark 署名

static void _apdu_compute_digital_signature_jpki(dispatch_semaphore_t sem,
                                                 TKSmartCard *smartCard,
                                                 Json::Value& threadCtx) {
    
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
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:APDU_size]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            size_t signature_length = [response length] -2;
            std::vector<uint8_t>buf(signature_length);
            [response getBytes:&buf[0] range:NSMakeRange(0, signature_length)];
            std::string hex;
            bytes_to_hex(&buf[0], signature_length, hex);
            threadCtx["signature"] = hex;
            std::string b64;
            bytes_to_b64(&buf[0], signature_length, b64);
            threadCtx["signature_base64"] = b64;
            threadCtx["success"] = true;
            _end_session(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_compute_digital_signature_hpki(dispatch_semaphore_t sem,
                                                 TKSmartCard *smartCard,
                                                 Json::Value& threadCtx) {
    
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
    
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            size_t signature_length = [response length] -2;
            std::vector<uint8_t>buf(signature_length);
            [response getBytes:&buf[0] range:NSMakeRange(0, signature_length)];
            std::string hex;
            bytes_to_hex(&buf[0], signature_length, hex);
            threadCtx["signature"] = hex;
            std::string b64;
            bytes_to_b64(&buf[0], signature_length, b64);
            threadCtx["signature_base64"] = b64;
            threadCtx["success"] = true;
            _end_session(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_verify_app_hpki_compute_digital_signature(dispatch_semaphore_t sem,
                                                            TKSmartCard *smartCard,
                                                            Json::Value& threadCtx) {
    
    std::string pin = threadCtx["pin4"].asString();
    
    std::vector<uint8_t>data(sizeof(APDU_VERIFY_PIN) + pin.length());
    memcpy(&data[0],
           APDU_VERIFY_PIN,
           sizeof(APDU_VERIFY_PIN));
    data[3] = APDU_VERIFY_PIN_EF_HPKI;
    data[4] = pin.length();
    if(pin.length()) {
        memcpy(&data[5], pin.data(), pin.length());
    }

    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_compute_digital_signature_hpki(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_set_security_environment_hpki_signature(dispatch_semaphore_t sem,
                                                          TKSmartCard *smartCard,
                                                          Json::Value& threadCtx) {
    
    std::vector<uint8_t>data(sizeof(MANAGE_SECURITY_ENVIRONMENT));
    memcpy(&data[0],
           MANAGE_SECURITY_ENVIRONMENT,
           sizeof(MANAGE_SECURITY_ENVIRONMENT));
    
    std::string mse;
    bytes_to_hex(&data[0], data.size(), mse);
    threadCtx["mse"] = mse;
    
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_verify_app_hpki_compute_digital_signature(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_select_hpki_key_signature(dispatch_semaphore_t sem,
                                            TKSmartCard *smartCard,
                                            Json::Value& threadCtx) {
    
    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
           APDU_SELECT_EF_UNDER_DF,
           sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_KEY_EF_HPKI_HI;
    data[6] = APDU_SELECT_KEY_EF_HPKI_LO;
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_set_security_environment_hpki_signature(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_verify_app_hpki_compute_digital_signature_identity(dispatch_semaphore_t sem,
                                                                     TKSmartCard *smartCard,
                                                                     Json::Value& threadCtx) {
    
    std::string pin = threadCtx["pin4"].asString();
    
    std::vector<uint8_t>data(sizeof(APDU_VERIFY_PIN) + pin.length());
    memcpy(&data[0],
           APDU_VERIFY_PIN,
           sizeof(APDU_VERIFY_PIN));
    data[3] = APDU_VERIFY_PIN_EF_HPKI;
    data[4] = pin.length();
    if(pin.length()) {
        memcpy(&data[5], pin.data(), pin.length());
    }

    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_compute_digital_signature_hpki(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_set_security_environment_hpki_identity(dispatch_semaphore_t sem,
                                                         TKSmartCard *smartCard,
                                                         Json::Value& threadCtx) {
    
    std::vector<uint8_t>data(sizeof(MANAGE_SECURITY_ENVIRONMENT));
    memcpy(&data[0],
           MANAGE_SECURITY_ENVIRONMENT,
           sizeof(MANAGE_SECURITY_ENVIRONMENT));

    std::string mse;
    bytes_to_hex(&data[0], data.size(), mse);
    threadCtx["mse"] = mse;
    
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_verify_app_hpki_compute_digital_signature_identity(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_select_hpki_key_identity(dispatch_semaphore_t sem,
                                           TKSmartCard *smartCard,
                                           Json::Value& threadCtx) {
    
    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
           APDU_SELECT_EF_UNDER_DF,
           sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_KEY_EF_HPKI_HI;
    data[6] = APDU_SELECT_KEY_EF_HPKI_LO;
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
             _apdu_set_security_environment_hpki_identity(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_select_pin_hpki_compute_digital_signature_identity(dispatch_semaphore_t sem,
                                                                     TKSmartCard *smartCard,
                                                                     Json::Value& threadCtx) {
    
    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
           APDU_SELECT_EF_UNDER_DF,
           sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_PIN_EF_HPKI_HI;
    data[6] = APDU_SELECT_PIN_EF_HPKI_LO;
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_select_hpki_key_identity(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}
#pragma mark 認証用秘密鍵

static void _apdu_select_jpki_key_identity(dispatch_semaphore_t sem,
                                           TKSmartCard *smartCard,
                                           Json::Value& threadCtx) {
    
    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
           APDU_SELECT_EF_UNDER_DF,
           sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_KEY_IDENTITY_EF_JPKI_HI;
    data[6] = APDU_SELECT_KEY_IDENTITY_EF_JPKI_LO;
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_compute_digital_signature_jpki(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_verify_app_jpki_compute_digital_signature_identity(dispatch_semaphore_t sem,
                                                                     TKSmartCard *smartCard,
                                                                     Json::Value& threadCtx) {
    
    std::string pin = threadCtx["pin4"].asString();
    
    std::vector<uint8_t>data(sizeof(APDU_VERIFY_PIN) + pin.length());
    memcpy(&data[0],
           APDU_VERIFY_PIN,
           sizeof(APDU_VERIFY_PIN));
    data[3] = APDU_VERIFY_PIN_EF_JPKI;
    data[4] = pin.length();
    if(pin.length()) {
        memcpy(&data[5], pin.data(), pin.length());
    }

    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_select_jpki_key_identity(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_select_pin_jpki_compute_digital_signature_identity(dispatch_semaphore_t sem,
                                                                     TKSmartCard *smartCard,
                                                                     Json::Value& threadCtx) {
    
    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
           APDU_SELECT_EF_UNDER_DF,
           sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_PIN_IDENTITY_EF_JPKI_HI;
    data[6] = APDU_SELECT_PIN_IDENTITY_EF_JPKI_LO;
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_verify_app_jpki_compute_digital_signature_identity(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_select_app_jpki_compute_digital_signature_identity(dispatch_semaphore_t sem,
                                                                     TKSmartCard *smartCard,
                                                                     Json::Value& threadCtx) {
    
    [smartCard beginSessionWithReply:^(BOOL _success, NSError *error) {
        if (_success) {
            std::string hex = APDU_SELECT_IDENTITY_AP_JPKI;
            std::vector<uint8_t>buf(0);
            hex_to_bytes(hex, buf);
            std::vector<uint8_t>data(sizeof(APDU_SELECT_FILE_DF) + buf.size() -1);
            memcpy(&data[0],
                   APDU_SELECT_FILE_DF,
                   sizeof(APDU_SELECT_FILE_DF));
            data[3] = APDU_SELECT_FILE_DF_P2_RFU_JPKI;
            data[4] = buf.size();
            memcpy(&data[5], &buf[0], buf.size());
            [smartCard
             transmitRequest:[NSData dataWithBytes:&data[0]
                                            length:data.size()]
             reply:^(NSData *response, NSError *error) {
                if ((error == nil) && (_is_response_positive(response, threadCtx))) {
                    _apdu_select_pin_jpki_compute_digital_signature_identity(sem, smartCard, threadCtx);
                }else{
                    _end_session(sem, smartCard, threadCtx);
                }
            }];
        }else{
            //beginSessionWithReply() failed
            dispatch_semaphore_signal(sem);
        }
    }];
    
    // wait for the asynchronous blocks to finish
    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
    
}
#pragma mark 署名用秘密鍵

static void _apdu_select_jpki_key_signature(dispatch_semaphore_t sem,
                                            TKSmartCard *smartCard,
                                            Json::Value& threadCtx) {
    
    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
           APDU_SELECT_EF_UNDER_DF,
           sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_KEY_SIGNATURE_EF_JPKI_HI;
    data[6] = APDU_SELECT_KEY_SIGNATURE_EF_JPKI_LO;
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_compute_digital_signature_jpki(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_verify_app_jpki_compute_digital_signature(dispatch_semaphore_t sem,
                                                            TKSmartCard *smartCard,
                                                            Json::Value& threadCtx) {
    
    std::string pin = threadCtx["pin6"].asString();
    
    std::vector<uint8_t>data(sizeof(APDU_VERIFY_PIN) + pin.length());
    memcpy(&data[0],
           APDU_VERIFY_PIN,
           sizeof(APDU_VERIFY_PIN));
    data[3] = APDU_VERIFY_PIN_EF_JPKI;
    data[4] = pin.length();
    if(pin.length()) {
        memcpy(&data[5], pin.data(), pin.length());
    }

    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_select_jpki_key_signature(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_select_pin_jpki_compute_digital_signature(dispatch_semaphore_t sem,
                                                            TKSmartCard *smartCard,
                                                            Json::Value& threadCtx) {
    
    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
           APDU_SELECT_EF_UNDER_DF,
           sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_PIN_SIGNATURE_EF_JPKI_HI;
    data[6] = APDU_SELECT_PIN_SIGNATURE_EF_JPKI_LO;
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_verify_app_jpki_compute_digital_signature(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_select_app_jpki_compute_digital_signature(dispatch_semaphore_t sem,
                                                            TKSmartCard *smartCard,
                                                            Json::Value& threadCtx) {
    
    [smartCard beginSessionWithReply:^(BOOL _success, NSError *error) {
        if (_success) {
            std::string hex = APDU_SELECT_IDENTITY_AP_JPKI;//署名用APではない
            std::vector<uint8_t>buf(0);
            hex_to_bytes(hex, buf);
            std::vector<uint8_t>data(sizeof(APDU_SELECT_FILE_DF) + buf.size() -1);
            memcpy(&data[0],
                   APDU_SELECT_FILE_DF,
                   sizeof(APDU_SELECT_FILE_DF));
            data[3] = APDU_SELECT_FILE_DF_P2_RFU_JPKI;
            data[4] = buf.size();
            memcpy(&data[5], &buf[0], buf.size());
            [smartCard
             transmitRequest:[NSData dataWithBytes:&data[0]
                                            length:data.size()]
             reply:^(NSData *response, NSError *error) {
                if ((error == nil) && (_is_response_positive(response, threadCtx))) {
                    _apdu_select_pin_jpki_compute_digital_signature(sem, smartCard, threadCtx);
                }else{
                    _end_session(sem, smartCard, threadCtx);
                }
            }];
        }else{
            //beginSessionWithReply() failed
            dispatch_semaphore_signal(sem);
        }
    }];
    
    // wait for the asynchronous blocks to finish
    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
    
}

#pragma mark HPKI

static void _apdu_select_pin_hpki_compute_digital_signature(dispatch_semaphore_t sem,
                                                            TKSmartCard *smartCard,
                                                            Json::Value& threadCtx) {
    
    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
           APDU_SELECT_EF_UNDER_DF,
           sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_PIN_EF_HPKI_HI;
    data[6] = APDU_SELECT_PIN_EF_HPKI_LO;
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_select_hpki_key_signature(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_select_app_hpki_compute_digital_signature(dispatch_semaphore_t sem,
                                                            TKSmartCard *smartCard,
                                                            Json::Value& threadCtx) {
    
    [smartCard beginSessionWithReply:^(BOOL _success, NSError *error) {
        if (_success) {
            
            std::string hex = APDU_SELECT_SIGNATURE_AP_HPKI;
            std::vector<uint8_t>buf(0);
            hex_to_bytes(hex, buf);

            std::vector<uint8_t>data(sizeof(APDU_SELECT_FILE_DF) + buf.size());
            memcpy(&data[0],
                   APDU_SELECT_FILE_DF,
                   sizeof(APDU_SELECT_FILE_DF));
            
            data[4] = buf.size();
            memcpy(&data[5], &buf[0], buf.size());
            
            [smartCard
             transmitRequest:[NSData dataWithBytes:&data[0]
                                            length:data.size()]
             reply:^(NSData *response, NSError *error) {
                if ((error == nil) && (_is_response_positive(response, threadCtx))) {
                    _apdu_select_pin_hpki_compute_digital_signature(sem, smartCard, threadCtx);
                }else{
                    _end_session(sem, smartCard, threadCtx);
                }
            }];
        }else{
            //beginSessionWithReply() failed
            dispatch_semaphore_signal(sem);
        }
    }];
    
    // wait for the asynchronous blocks to finish
    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
    
}

static void _apdu_select_app_hpki_compute_digital_signature_identity(dispatch_semaphore_t sem,
                                                                     TKSmartCard *smartCard,
                                                                     Json::Value& threadCtx) {
    
    [smartCard beginSessionWithReply:^(BOOL _success, NSError *error) {
        if (_success) {
            
            std::string hex = APDU_SELECT_IDENTITY_AP_HPKI;
            std::vector<uint8_t>buf(0);
            hex_to_bytes(hex, buf);
            
            std::vector<uint8_t>data(sizeof(APDU_SELECT_FILE_DF) + buf.size());
            memcpy(&data[0],
                   APDU_SELECT_FILE_DF,
                   sizeof(APDU_SELECT_FILE_DF));
            
            data[4] = buf.size();
            memcpy(&data[5], &buf[0], buf.size());
   
            [smartCard
             transmitRequest:[NSData dataWithBytes:&data[0]
                                            length:data.size()]
             reply:^(NSData *response, NSError *error) {
                if ((error == nil) && (_is_response_positive(response, threadCtx))) {
                    _apdu_select_pin_hpki_compute_digital_signature_identity(sem, smartCard, threadCtx);
                }else{
                    _end_session(sem, smartCard, threadCtx);
                }
            }];
        }else{
            //beginSessionWithReply() failed
            dispatch_semaphore_signal(sem);
        }
    }];
    
    // wait for the asynchronous blocks to finish
    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
    
}

#pragma mark 証明書

static void _read_block_cert_jpki(dispatch_semaphore_t sem,
                                  TKSmartCard *smartCard,
                                  Json::Value *threadCtx) {
    
    std::vector<uint8_t>data(sizeof(APDU_READ_BINARY));
    memcpy(&data[0],
           APDU_READ_BINARY,
           sizeof(APDU_READ_BINARY));
        
    int block = threadCtx->operator[]("block").asInt();
    int size = threadCtx->operator[]("size").asInt();
    
    data[2] = block;
    
    if(size < 0x100) {
        data[4] = size;
    }
    
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, *threadCtx))) {
                        
            int block = threadCtx->operator[]("block").asInt();
            int size = threadCtx->operator[]("size").asInt();
            
            std::string hex;
            if(size > 0xFF){
                bytes_to_hex((const uint8_t *)[response bytes], 0x0100, hex);
            }else{
                bytes_to_hex((const uint8_t *)[response bytes],   size, hex);
            }
            
            if(threadCtx->isMember("data")) {
                std::string _hex = threadCtx->operator[]("data").asString();
                threadCtx->operator[]("data") = _hex + hex;
            }else{
                threadCtx->operator[]("data") = hex;
            }

            size-=0x100;
            block++;
            
            threadCtx->operator[]("size").operator=(size);
            threadCtx->operator[]("block").operator=(block);
            
            if(size > 0) {
                _read_block_cert_jpki(sem, smartCard, threadCtx);
            }else{
                
                if(threadCtx->isMember("data")) {
                    Json::Value certificate(Json::objectValue);
                    std::string hex = threadCtx->operator[]("data").asString();
                    certificate["der"] = hex;
                    
                    std::vector<uint8_t>buf(0);
                    hex_to_bytes(hex, buf);
                    const uint8_t *p = &buf[0];
                    X509 *x509 = d2i_X509(NULL, &p, buf.size());
                    if(x509) {
                        BIO *bio = BIO_new(BIO_s_mem());
                        if(PEM_write_bio_X509(bio, x509)) {
                            uint64_t len = BIO_number_written(bio) + 1;
                            std::vector<uint8_t>_buf(len);
                            if(BIO_read(bio, &_buf[0], (int)len)) {
                                certificate["pem"] = std::string((const char *)&_buf[0], len);
                            }
                        }
                        BIO_free(bio);
                        
                        X509_NAME *subject = X509_get_subject_name(x509);
                        certificate["subject"] = X509_NAME_oneline(subject, NULL, 0);
                        
                        X509_NAME *issuer = X509_get_issuer_name(x509);
                        certificate["issuer"] = X509_NAME_oneline(issuer, NULL, 0);
                          
                        long serialNumber = ASN1_INTEGER_get(X509_get_serialNumber(x509));
                        long version = X509_get_version(x509);
                        certificate["serialNumber"] = (int)serialNumber;
                        certificate["version"] = (int)version;
                        ASN1_TIME *notBefore = X509_get_notBefore(x509);
                        ASN1_TIME *notAfter = X509_get_notAfter(x509);
                        
                        EVP_PKEY *pubkey = X509_get0_pubkey(x509);
                        if(pubkey) {
                            const RSA *rsa = EVP_PKEY_get0_RSA(pubkey);
                            if(rsa) {
                                BIGNUM *modulus = NULL;
                                BIGNUM *public_exponent = NULL;
                                RSA_get0_key(rsa,
                                             (const BIGNUM **)&modulus,
                                             (const BIGNUM **)&public_exponent,
                                             NULL);
                                if (modulus != NULL) {
                                    certificate["length"] = BN_num_bits(modulus);
                                }
                            }
                        }
                        
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
                
                _end_session(sem, smartCard, *threadCtx);
            }
        }else{
            _end_session(sem, smartCard, *threadCtx);
        }
    }];
    
}
#pragma mark 認証用証明書

static void _apdu_binary_read_jpki_cert_identity(dispatch_semaphore_t sem,
                                                 TKSmartCard *smartCard,
                                                 Json::Value& threadCtx) {
    
    threadCtx["block"] = 0x00;
    
    _read_block_cert_jpki(sem, smartCard, &threadCtx);
}

static void _apdu_binary_read_jpki_cert_identity_length(dispatch_semaphore_t sem,
                                                        TKSmartCard *smartCard,
                                                        Json::Value& threadCtx) {
    
    std::vector<uint8_t>data(sizeof(APDU_READ_BINARY_GET_LENGTH));
    memcpy(&data[0],
           APDU_READ_BINARY_GET_LENGTH,
           sizeof(APDU_READ_BINARY_GET_LENGTH));
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            std::vector<uint8_t>size(4);
            [response getBytes:&size[0] range:NSMakeRange(0, 4)];
            threadCtx["size"] = _get_data_size(size);
            _apdu_binary_read_jpki_cert_identity(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_select_jpki_cert_identity(dispatch_semaphore_t sem,
                                            TKSmartCard *smartCard,
                                            Json::Value& threadCtx) {
    
    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
           APDU_SELECT_EF_UNDER_DF,
           sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_CRT_IDENTITY_EF_JPKI_HI;
    data[6] = APDU_SELECT_CRT_IDENTITY_EF_JPKI_LO;
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_binary_read_jpki_cert_identity_length(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_select_app_jpki_cert_identity(dispatch_semaphore_t sem,
                                                TKSmartCard *smartCard,
                                                Json::Value& threadCtx) {
    
    [smartCard beginSessionWithReply:^(BOOL _success, NSError *error) {
        if (_success) {
            std::string hex = APDU_SELECT_IDENTITY_AP_JPKI;
            std::vector<uint8_t>buf(0);
            hex_to_bytes(hex, buf);
            std::vector<uint8_t>data(sizeof(APDU_SELECT_FILE_DF) + buf.size() -1);
            memcpy(&data[0],
                   APDU_SELECT_FILE_DF,
                   sizeof(APDU_SELECT_FILE_DF));
            data[3] = APDU_SELECT_FILE_DF_P2_RFU_JPKI;
            data[4] = buf.size();
            memcpy(&data[5], &buf[0], buf.size());
            [smartCard
             transmitRequest:[NSData dataWithBytes:&data[0]
                                            length:data.size()]
             reply:^(NSData *response, NSError *error) {
                if ((error == nil) && (_is_response_positive(response, threadCtx))) {
                    _apdu_select_jpki_cert_identity(sem, smartCard, threadCtx);
                }else{
                    _end_session(sem, smartCard, threadCtx);
                }
            }];
        }else{
            //beginSessionWithReply() failed
            dispatch_semaphore_signal(sem);
        }
    }];
    
    // wait for the asynchronous blocks to finish
    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
}

#pragma mark HPKI

static void _apdu_select_hpki_cert_identity(dispatch_semaphore_t sem,
                                            TKSmartCard *smartCard,
                                            Json::Value& threadCtx) {
    
    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
           APDU_SELECT_EF_UNDER_DF,
           sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_CRT_EF_HPKI_HI;
    data[6] = APDU_SELECT_CRT_EF_HPKI_LO;
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_binary_read_jpki_cert_identity_length(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_select_hpki_cert_signature(dispatch_semaphore_t sem,
                                             TKSmartCard *smartCard,
                                             Json::Value& threadCtx) {
    
    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
           APDU_SELECT_EF_UNDER_DF,
           sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_CRT_EF_HPKI_HI;
    data[6] = APDU_SELECT_CRT_EF_HPKI_LO;
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_binary_read_jpki_cert_identity_length(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_select_app_hpki_cert_identity(dispatch_semaphore_t sem,
                                                TKSmartCard *smartCard,
                                                Json::Value& threadCtx) {
    
    
    
    [smartCard beginSessionWithReply:^(BOOL _success, NSError *error) {
        if (_success) {
            
            std::string hex = APDU_SELECT_IDENTITY_AP_HPKI;
            std::vector<uint8_t>buf(0);
            hex_to_bytes(hex, buf);
 
            std::vector<uint8_t>data(sizeof(APDU_SELECT_FILE_DF) + buf.size());
            memcpy(&data[0],
                   APDU_SELECT_FILE_DF,
                   sizeof(APDU_SELECT_FILE_DF));
            
            data[4] = buf.size();
            memcpy(&data[5], &buf[0], buf.size());

            [smartCard
             transmitRequest:[NSData dataWithBytes:&data[0]
                                            length:data.size()]
             reply:^(NSData *response, NSError *error) {
                if ((error == nil) && (_is_response_positive(response, threadCtx))) {
                    _apdu_select_hpki_cert_identity(sem, smartCard, threadCtx);
                }else{
                    _end_session(sem, smartCard, threadCtx);
                }
            }];
        }else{
            //beginSessionWithReply() failed
            dispatch_semaphore_signal(sem);
        }
    }];
    
    // wait for the asynchronous blocks to finish
    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
}

static void _apdu_select_app_hpki_cert_signature(dispatch_semaphore_t sem,
                                                 TKSmartCard *smartCard,
                                                 Json::Value& threadCtx) {
    
    [smartCard beginSessionWithReply:^(BOOL _success, NSError *error) {
        if (_success) {
            
            std::string hex = APDU_SELECT_SIGNATURE_AP_HPKI;
            std::vector<uint8_t>buf(0);
            hex_to_bytes(hex, buf);
            
            std::vector<uint8_t>data(sizeof(APDU_SELECT_FILE_DF) + buf.size());
            memcpy(&data[0],
                   APDU_SELECT_FILE_DF,
                   sizeof(APDU_SELECT_FILE_DF));
            
            data[4] = buf.size();
            memcpy(&data[5], &buf[0], buf.size());
            
            [smartCard
             transmitRequest:[NSData dataWithBytes:&data[0]
                                            length:data.size()]
             reply:^(NSData *response, NSError *error) {
                if ((error == nil) && (_is_response_positive(response, threadCtx))) {
                    _apdu_select_hpki_cert_signature(sem, smartCard, threadCtx);
                }else{
                    _end_session(sem, smartCard, threadCtx);
                }
            }];
        }else{
            //beginSessionWithReply() failed
            dispatch_semaphore_signal(sem);
        }
    }];
    
    // wait for the asynchronous blocks to finish
    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
}

#pragma mark 署名用証明書

static void _apdu_binary_read_jpki_cert_signature(dispatch_semaphore_t sem,
                                                  TKSmartCard *smartCard,
                                                  Json::Value& threadCtx) {
    
    threadCtx["block"] = 0x00;
    
    _read_block_cert_jpki(sem, smartCard, &threadCtx);
}

static void _apdu_binary_read_jpki_cert_signature_length(dispatch_semaphore_t sem,
                                                         TKSmartCard *smartCard,
                                                         Json::Value& threadCtx) {
    
    std::vector<uint8_t>data(sizeof(APDU_READ_BINARY_GET_LENGTH));
    memcpy(&data[0],
           APDU_READ_BINARY_GET_LENGTH,
           sizeof(APDU_READ_BINARY_GET_LENGTH));
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            std::vector<uint8_t>size(4);
            [response getBytes:&size[0] range:NSMakeRange(0, 4)];
            threadCtx["size"] = _get_data_size(size);
            _apdu_binary_read_jpki_cert_signature(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_select_jpki_cert_signature(dispatch_semaphore_t sem,
                                             TKSmartCard *smartCard,
                                             Json::Value& threadCtx) {
    
    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
           APDU_SELECT_EF_UNDER_DF,
           sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_CRT_SIGNATURE_EF_JPKI_HI;
    data[6] = APDU_SELECT_CRT_SIGNATURE_EF_JPKI_LO;
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_binary_read_jpki_cert_signature_length(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_verify_app_jpki_cert_signature(dispatch_semaphore_t sem,
                                                 TKSmartCard *smartCard,
                                                 Json::Value& threadCtx) {
    
    std::string pin = threadCtx["pin6"].asString();
    
    std::vector<uint8_t>data(sizeof(APDU_VERIFY_PIN) + pin.length());
    memcpy(&data[0],
           APDU_VERIFY_PIN,
           sizeof(APDU_VERIFY_PIN));
    data[3] = APDU_VERIFY_PIN_EF_JPKI;
    data[4] = pin.length();
    if(pin.length()) {
        memcpy(&data[5], pin.data(), pin.length());
    }

    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_select_jpki_cert_signature(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_select_pin_jpki_cert_signature(dispatch_semaphore_t sem,
                                                 TKSmartCard *smartCard,
                                                 Json::Value& threadCtx) {
    
    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
           APDU_SELECT_EF_UNDER_DF,
           sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_PIN_SIGNATURE_EF_JPKI_HI;
    data[6] = APDU_SELECT_PIN_SIGNATURE_EF_JPKI_LO;
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_verify_app_jpki_cert_signature(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_select_app_jpki_cert_signature(dispatch_semaphore_t sem,
                                                 TKSmartCard *smartCard,
                                                 Json::Value& threadCtx) {
    
    [smartCard beginSessionWithReply:^(BOOL _success, NSError *error) {
        if (_success) {
            std::string hex = APDU_SELECT_IDENTITY_AP_JPKI;
            std::vector<uint8_t>buf(0);
            hex_to_bytes(hex, buf);
            std::vector<uint8_t>data(sizeof(APDU_SELECT_FILE_DF) + buf.size() -1);
            memcpy(&data[0],
                   APDU_SELECT_FILE_DF,
                   sizeof(APDU_SELECT_FILE_DF));
            data[3] = APDU_SELECT_FILE_DF_P2_RFU_JPKI;
            data[4] = buf.size();
            memcpy(&data[5], &buf[0], buf.size());
            [smartCard
             transmitRequest:[NSData dataWithBytes:&data[0]
                                            length:data.size()]
             reply:^(NSData *response, NSError *error) {
                if ((error == nil) && (_is_response_positive(response, threadCtx))) {
                    _apdu_select_pin_jpki_cert_signature(sem, smartCard, threadCtx);
                }else{
                    _end_session(sem, smartCard, threadCtx);
                }
            }];
        }else{
            //beginSessionWithReply() failed
            dispatch_semaphore_signal(sem);
        }
    }];
    
    // wait for the asynchronous blocks to finish
    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
}

#pragma mark 個人番号

static void _apdu_binary_read_my_number(dispatch_semaphore_t sem,
                                        TKSmartCard *smartCard,
                                        Json::Value& threadCtx) {
    
    int size = threadCtx["size"].asInt();
    
    std::vector<uint8_t>data(sizeof(APDU_READ_BINARY));
    memcpy(&data[0],
           APDU_READ_BINARY,
           sizeof(APDU_READ_BINARY));
    data[4] = size;
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            std::vector<uint8_t>buf(size);
            [response getBytes:&buf[0] range:NSMakeRange(0, size)];
            int len = buf[2];
            threadCtx["myNumber"] = std::string((const char *)&buf[3], len);
            threadCtx["success"] = true;
            _end_session(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static int _get_data_size(std::vector<uint8_t>& size) {
    
    if(size.size() != 4) {
        return 0;
    }else{
        int data_length_length, data_length, offset = 2;
        if ((size[1] >> 7) == 0){
            data_length_length = 1;
            data_length = size[2];
            offset++;
        }else{
            data_length = 0;
            data_length_length = size[1] & 0x7f;
            for(int i = 2; i < data_length_length+2; ++i) {
                data_length = (data_length << 8) + size[i];
                offset++;
            }
        }
        return offset + data_length;
    }
}

static void _apdu_binary_read_my_number_length(dispatch_semaphore_t sem,
                                               TKSmartCard *smartCard,
                                               Json::Value& threadCtx) {
    
    std::vector<uint8_t>data(sizeof(APDU_READ_BINARY_GET_LENGTH));
    memcpy(&data[0],
           APDU_READ_BINARY_GET_LENGTH,
           sizeof(APDU_READ_BINARY_GET_LENGTH));
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            std::vector<uint8_t>size(4);
            [response getBytes:&size[0] range:NSMakeRange(0, 4)];
            threadCtx["size"] = _get_data_size(size);
            _apdu_binary_read_my_number(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_select_my_number(dispatch_semaphore_t sem,
                                 TKSmartCard *smartCard,
                                 Json::Value& threadCtx) {
        
    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
           APDU_SELECT_EF_UNDER_DF,
           sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_MNUMBER_EF_JPKI_HI;
    data[6] = APDU_SELECT_MNUMBER_EF_JPKI_LO;
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_binary_read_my_number_length(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_verify_app_aux_my_number(dispatch_semaphore_t sem,
                                           TKSmartCard *smartCard,
                                           Json::Value& threadCtx) {
    
    std::string pin = threadCtx["pin4"].asString();
    
    std::vector<uint8_t>data(sizeof(APDU_VERIFY_PIN) + pin.length());
    memcpy(&data[0],
           APDU_VERIFY_PIN,
           sizeof(APDU_VERIFY_PIN));
    data[3] = APDU_VERIFY_PIN_EF_JPKI;
    data[4] = pin.length();
    if(pin.length()) {
        memcpy(&data[5], pin.data(), pin.length());
    }

    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_select_my_number(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_select_pin_aux_my_number(dispatch_semaphore_t sem,
                                           TKSmartCard *smartCard,
                                           Json::Value& threadCtx) {
    
    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
           APDU_SELECT_EF_UNDER_DF,
           sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_PIN_CARD_EF_JPKI_HI;
    data[6] = APDU_SELECT_PIN_CARD_EF_JPKI_LO;
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_verify_app_aux_my_number(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_select_app_aux_my_number(dispatch_semaphore_t sem,
                                           TKSmartCard *smartCard,
                                           Json::Value& threadCtx) {
     
    [smartCard beginSessionWithReply:^(BOOL _success, NSError *error) {
        if (_success) {
            std::string hex = APDU_SELECT_CARD_AP_JPKI;
            std::vector<uint8_t>buf(0);
            hex_to_bytes(hex, buf);
            std::vector<uint8_t>data(sizeof(APDU_SELECT_FILE_DF) + buf.size() -1);
            memcpy(&data[0],
                   APDU_SELECT_FILE_DF,
                   sizeof(APDU_SELECT_FILE_DF));
            data[3] = APDU_SELECT_FILE_DF_P2_RFU_JPKI;
            data[4] = buf.size();
            memcpy(&data[5], &buf[0], buf.size());
            [smartCard
             transmitRequest:[NSData dataWithBytes:&data[0]
                                            length:data.size()]
             reply:^(NSData *response, NSError *error) {
                if ((error == nil) && (_is_response_positive(response, threadCtx))) {
                    _apdu_select_pin_aux_my_number(sem, smartCard, threadCtx);
                }else{
                    _end_session(sem, smartCard, threadCtx);
                }
            }];
        }else{
            //beginSessionWithReply() failed
            dispatch_semaphore_signal(sem);
        }
    }];
    
    // wait for the asynchronous blocks to finish
    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
}

#pragma mark 基本4情報

static void _read_block_basic4i(dispatch_semaphore_t sem,
                                  TKSmartCard *smartCard,
                                  Json::Value *threadCtx) {
    
    std::vector<uint8_t>data(sizeof(APDU_READ_BINARY));
    memcpy(&data[0],
           APDU_READ_BINARY,
           sizeof(APDU_READ_BINARY));
        
    int block = threadCtx->operator[]("block").asInt();
    int size = threadCtx->operator[]("size").asInt();
    
    data[2] = block;
    
    if(size < 0x100) {
        data[4] = size;
    }
    
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, *threadCtx))) {
                        
            int block = threadCtx->operator[]("block").asInt();
            int size = threadCtx->operator[]("size").asInt();
            
            std::string hex;
            if(size > 0xFF){
                bytes_to_hex((const uint8_t *)[response bytes], 0x0100, hex);
            }else{
                bytes_to_hex((const uint8_t *)[response bytes],   size, hex);
            }
            
            if(threadCtx->isMember("data")) {
                std::string _hex = threadCtx->operator[]("data").asString();
                threadCtx->operator[]("data") = _hex + hex;
            }else{
                threadCtx->operator[]("data") = hex;
            }

            size-=0x100;
            block++;
            
            threadCtx->operator[]("size").operator=(size);
            threadCtx->operator[]("block").operator=(block);
            
            if(size > 0) {
                _read_block_basic4i(sem, smartCard, threadCtx);
            }else{
                
                if(threadCtx->isMember("data")) {

                    std::string hex = threadCtx->operator[]("data").asString();
                    std::vector<uint8_t>buf(0);
                    hex_to_bytes(hex, buf);
                    bool boundary = false;
                    bool field_id = false;
                    uint8_t field_type = 0;
                    
                    for (size_t i = 0; i < buf.size(); ++i) {
                        if(buf[i] == 0xDF) {
                            boundary = true;
                            continue;
                        }
                        if(boundary) {
                            boundary = false;
                            field_id = true;
                            field_type = buf[i];
                            continue;
                        }
                        if(field_id) {
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
                                    threadCtx->operator[]("commonName").operator=(std::string((const char *)&buf[i+1], field_length));
                                    break;
                                case 0x23:
                                    //address
//                                    threadCtx["address"] = std::string((const char *)&buf[i+1], field_length);
                                    threadCtx->operator[]("address").operator=(std::string((const char *)&buf[i+1], field_length));
                                    break;
                                case 0x24:
                                    //dateOfBirth
//                                    threadCtx["dateOfBirth"] = /*std::string((const char *)&buf[i+1], field_length)*/;
                                    threadCtx->operator[]("dateOfBirth").operator=(std::string((const char *)&buf[i+1], field_length));
                                    break;
                                case 0x25:
                                    //sex
//                                    threadCtx["gender"] = std::string((const char *)&buf[i+1], field_length);
                                    threadCtx->operator[]("gender").operator=(std::string((const char *)&buf[i+1], field_length));
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
                
                _end_session(sem, smartCard, *threadCtx);
            }
        }else{
            _end_session(sem, smartCard, *threadCtx);
        }
    }];
    
}

static void _apdu_binary_read_basic4i(dispatch_semaphore_t sem,
                                                  TKSmartCard *smartCard,
                                                  Json::Value& threadCtx) {
    
    threadCtx["block"] = 0x00;
    
    _read_block_basic4i(sem, smartCard, &threadCtx);
}

static void _apdu_binary_read_basic4i_length(dispatch_semaphore_t sem,
                                             TKSmartCard *smartCard,
                                             Json::Value& threadCtx) {
    
    std::vector<uint8_t>data(sizeof(APDU_READ_BINARY_GET_LENGTH));
    memcpy(&data[0],
           APDU_READ_BINARY_GET_LENGTH,
           sizeof(APDU_READ_BINARY_GET_LENGTH));
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            std::vector<uint8_t>size(4);
            [response getBytes:&size[0] range:NSMakeRange(0, 4)];
            threadCtx["size"] = _get_data_size(size);
            _apdu_binary_read_basic4i(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_select_basic4i(dispatch_semaphore_t sem,
                                 TKSmartCard *smartCard,
                                 Json::Value& threadCtx) {
    
    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
           APDU_SELECT_EF_UNDER_DF,
           sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_BASIC4I_EF_JPKI_HI;
    data[6] = APDU_SELECT_BASIC4I_EF_JPKI_LO;
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_binary_read_basic4i_length(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_verify_app_aux_basic4i(dispatch_semaphore_t sem,
                                 TKSmartCard *smartCard,
                                 Json::Value& threadCtx) {
    
    std::string pin = threadCtx["pin4"].asString();
    
    std::vector<uint8_t>data(sizeof(APDU_VERIFY_PIN) + pin.length());
    memcpy(&data[0],
           APDU_VERIFY_PIN,
           sizeof(APDU_VERIFY_PIN));
    data[3] = APDU_VERIFY_PIN_EF_JPKI;
    data[4] = pin.length();
    if(pin.length()) {
        memcpy(&data[5], pin.data(), pin.length());
    }

    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_select_basic4i(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_select_pin_aux_basic4i(dispatch_semaphore_t sem,
                                 TKSmartCard *smartCard,
                                 Json::Value& threadCtx) {
    
    std::vector<uint8_t>data(sizeof(APDU_SELECT_EF_UNDER_DF));
    memcpy(&data[0],
           APDU_SELECT_EF_UNDER_DF,
           sizeof(APDU_SELECT_EF_UNDER_DF));
    data[5] = APDU_SELECT_PIN_CARD_EF_JPKI_HI;
    data[6] = APDU_SELECT_PIN_CARD_EF_JPKI_LO;
    [smartCard
     transmitRequest:[NSData dataWithBytes:&data[0]
                                    length:data.size()]
     reply:^(NSData *response, NSError *error) {
        if ((error == nil) && (_is_response_positive(response, threadCtx))) {
            _apdu_verify_app_aux_basic4i(sem, smartCard, threadCtx);
        }else{
            _end_session(sem, smartCard, threadCtx);
        }
    }];
}

static void _apdu_select_app_aux_basic4i(dispatch_semaphore_t sem,
                                         TKSmartCard *smartCard,
                                         Json::Value& threadCtx) {
     
    [smartCard beginSessionWithReply:^(BOOL _success, NSError *error) {
        if (_success) {
            std::string hex = APDU_SELECT_CARD_AP_JPKI;
            std::vector<uint8_t>buf(0);
            hex_to_bytes(hex, buf);
            std::vector<uint8_t>data(sizeof(APDU_SELECT_FILE_DF) + buf.size() -1);
            memcpy(&data[0],
                   APDU_SELECT_FILE_DF,
                   sizeof(APDU_SELECT_FILE_DF));
            data[3] = APDU_SELECT_FILE_DF_P2_RFU_JPKI;
            data[4] = buf.size();
            memcpy(&data[5], &buf[0], buf.size());
            [smartCard
             transmitRequest:[NSData dataWithBytes:&data[0]
                                            length:data.size()]
             reply:^(NSData *response, NSError *error) {
                if ((error == nil) && (_is_response_positive(response, threadCtx))) {
                    _apdu_select_pin_aux_basic4i(sem, smartCard, threadCtx);
                }else{
                    _end_session(sem, smartCard, threadCtx);
                }
            }];
        }else{
            //beginSessionWithReply() failed
            dispatch_semaphore_signal(sem);
        }
    }];
    
    // wait for the asynchronous blocks to finish
    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
}

#pragma mark -

void _get_my_number_macos(Json::Value& threadCtx) {
    
    if(_check_access(threadCtx)) {
        
        TKSmartCard *smartCard = _select_slot(threadCtx);
        if(smartCard) {
            pki_type_t pki_type = (pki_type_t)threadCtx["type"].asInt();
            if(pki_type==pki_type_j) {
                dispatch_semaphore_t sem = dispatch_semaphore_create(0);
                _apdu_select_app_aux_my_number(sem, smartCard, threadCtx);
                threadCtx["success"] = threadCtx["success"].asBool();
            }
        }else{
            threadCtx["error"] = "TKSmartCard::makeSmartCard() failed";
        }
    }
}

#pragma mark -

void _sign_with_certificate_macos(Json::Value& threadCtx) {
    
    if(_check_access(threadCtx)) {
        
        TKSmartCard *smartCard = _select_slot(threadCtx);
        if(smartCard) {
            pki_type_t pki_type = (pki_type_t)threadCtx["type"].asInt();
            dispatch_semaphore_t sem = dispatch_semaphore_create(0);
            if(threadCtx["certificateType"].asInt() == certificate_type_identity) {
                switch (pki_type) {
                    case pki_type_j:
                        _apdu_select_app_jpki_compute_digital_signature_identity(sem, smartCard, threadCtx);
                        break;
                    case pki_type_h:
                        _apdu_select_app_hpki_compute_digital_signature_identity(sem, smartCard, threadCtx);
                        break;
                    default:
                        break;
                }
            }else{
                switch (pki_type) {
                    case pki_type_j:
                        _apdu_select_app_jpki_compute_digital_signature(sem, smartCard, threadCtx);
                        break;
                    case pki_type_h:
                        _apdu_select_app_hpki_compute_digital_signature(sem, smartCard, threadCtx);
                        break;
                    default:
                        break;
                }
            }
            threadCtx["success"] = threadCtx["success"].asBool();
        }else{
            threadCtx["error"] = "TKSmartCard::makeSmartCard() failed";
        }
    }
}

static pki_type_t _get_pki_type(Json::Value& threadCtx) {
    
    if(threadCtx["historicalBytes"].isString()){
        std::string historicalBytes = threadCtx["historicalBytes"].asString();
        if(historicalBytes == ID_HPKI) {
            return pki_type_h;
        }
        if(historicalBytes == ID_JPKI) {
            return pki_type_j;
        }
        if(historicalBytes == ID_JGID) {
            return pki_type_j;
        }
    }
    
    return pki_type_unknown;
}

void _get_my_certificate_macos(Json::Value& threadCtx) {
        
    if(_check_access(threadCtx)) {
        
        TKSmartCard *smartCard = _select_slot(threadCtx);
        if(smartCard) {
            pki_type_t pki_type = (pki_type_t)threadCtx["type"].asInt();
            dispatch_semaphore_t sem = dispatch_semaphore_create(0);
            if(threadCtx["certificateType"].asInt() == certificate_type_identity) {
                switch (pki_type) {
                    case pki_type_j:
                        _apdu_select_app_jpki_cert_identity(sem, smartCard, threadCtx);
                        break;
                    case pki_type_h:
                        _apdu_select_app_hpki_cert_identity(sem, smartCard, threadCtx);
                        break;
                    default:
                        break;
                }
            }else{
                switch (pki_type) {
                    case pki_type_j:
                        _apdu_select_app_jpki_cert_signature(sem, smartCard, threadCtx);
                        break;
                    case pki_type_h:
                        _apdu_select_app_hpki_cert_signature(sem, smartCard, threadCtx);
                        break;
                    default:
                        break;
                }
            }
            threadCtx["success"] = threadCtx["success"].asBool();
        }else{
            threadCtx["error"] = "TKSmartCard::makeSmartCard() failed";
        }
    }
}

void _get_my_information_macos(Json::Value& threadCtx) {
    
    if(_check_access(threadCtx)) {
        
        TKSmartCard *smartCard = _select_slot(threadCtx);
        if(smartCard) {
            pki_type_t pki_type = (pki_type_t)threadCtx["type"].asInt();
            if(pki_type==pki_type_j) {
                dispatch_semaphore_t sem = dispatch_semaphore_create(0);
                _apdu_select_app_aux_basic4i(sem, smartCard, threadCtx);
                threadCtx["success"] = threadCtx["success"].asBool();
            }
        }else{
            threadCtx["error"] = "TKSmartCard::makeSmartCard() failed";
        }
    }
}
