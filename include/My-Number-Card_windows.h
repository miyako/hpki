#ifndef PLUGIN_MY_NUMBER_CARD_WINDOWS_H
#define PLUGIN_MY_NUMBER_CARD_WINDOWS_H

#include "apdu.h"

#if __WINDOWS__
#include "winscard.h"
#define DEFAULT_TIMEOUT_MS_FOR_RESOURCE_MANAGER 5000
#define LIBPCSC_API_TIMEOUT 100 
static bool _parse_atr(Json::Value& threadCtx);
void u16_to_u8(std::wstring& u16, std::string& u8);
void u8_to_u16(std::string& u8, std::wstring& u16);
#endif

void _sign_with_certificate_windows(Json::Value& threadCtx);
void _get_my_certificate_windows(Json::Value& threadCtx);
void _get_my_number_windows(Json::Value& threadCtx);
void _get_my_information_windows(Json::Value& threadCtx);
void _get_slots_windows(Json::Value& threadCtx);

//static int _get_data_size(std::vector<uint8_t>& size);
static pki_type_t _get_pki_type(Json::Value& threadCtx);

#endif /* PLUGIN_MY_NUMBER_CARD_WINDOWS_H */
