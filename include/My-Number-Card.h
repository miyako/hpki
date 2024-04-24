#ifndef MY_NUMBER_CARD_H
#define MY_NUMBER_CARD_H

#define _CONSOLE_

#ifdef __APPLE__
#include "My-Number-Card_macOS.h"
#else
#include "My-Number-Card_windows.h"
#endif

void _sign_with_certificate(Json::Value& threadCtx);
void _get_my_certificate(Json::Value& threadCtx);
void _get_my_number(Json::Value& threadCtx);
void _get_my_information(Json::Value& threadCtx);
void _get_slots(Json::Value& threadCtx);

#endif /* MY_NUMBER_CARD_H */
