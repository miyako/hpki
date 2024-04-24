#include "My-Number-Card.h"

void _sign_with_certificate(Json::Value& threadCtx) {
#if __APPLE__
    _sign_with_certificate_macos(threadCtx);
#else
    _sign_with_certificate_windows(threadCtx);
#endif
}

void _get_my_certificate(Json::Value& threadCtx) {
#if __APPLE__
    _get_my_certificate_macos(threadCtx);
#else
    _get_my_certificate_windows(threadCtx);
#endif
}

void _get_my_number(Json::Value& threadCtx) {
#if __APPLE__
    _get_my_number_macos(threadCtx);
#else
    _get_my_number_windows(threadCtx);
#endif
}

void _get_my_information(Json::Value& threadCtx) {
#if __APPLE__
    _get_my_information_macos(threadCtx);
#else
    _get_my_information_windows(threadCtx);
#endif
}

void _get_slots(Json::Value& threadCtx) {
#if __APPLE__
    _get_slots_macos(threadCtx);
#else
    _get_slots_windows(threadCtx);
#endif
}
