#ifndef PLUGIN_MY_NUMBER_CARD_MACOS_H
#define PLUGIN_MY_NUMBER_CARD_MACOS_H

#include "apdu.h"

#if __APPLE__
#import <security/Security.h>
#import <CryptoTokenKit/CryptoTokenKit.h>
#endif

void _sign_with_certificate_macos(Json::Value& threadCtx);
void _get_my_certificate_macos(Json::Value& threadCtx);
void _get_my_number_macos(Json::Value& threadCtx);
void _get_my_information_macos(Json::Value& threadCtx);
void _get_slots_macos(Json::Value& threadCtx);

static int _get_data_size(std::vector<uint8_t>& size);
static pki_type_t _get_pki_type(Json::Value& threadCtx);

#endif /* PLUGIN_MY_NUMBER_CARD_MACOS_H */
