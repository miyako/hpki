#include "apdu.h"

#if _WIN32
#include <time.h>
#include <iomanip>
#include <sstream>
extern "C" char* strptime(const char* s,
    const char* f,
    struct tm* tm) {
    // Isn't the C++ standard lib nice? std::get_time is defined such that its
    // format parameters are the exact same as strptime. Of course, we have to
    // create a string stream first, and imbue it with the current C locale, and
    // we also have to make sure we return the right things if it fails, or
    // if it succeeds, but this is still far simpler an implementation than any
    // of the versions in any of the C standard libraries.
    std::istringstream input(s);
    input.imbue(std::locale(setlocale(LC_ALL, nullptr)));
    input >> std::get_time(tm, f);
    if (input.fail()) {
        return nullptr;
    }
    return (char*)(s + input.tellg());
}
#endif

void asn_time_to_iso(ASN1_TIME *asn_time, std::string& str) {
    
    if(asn_time) {
        if(asn_time->type == V_ASN1_UTCTIME){
            std::vector<char>buf(32);
            struct tm tm_time;
            strptime((const char*)asn_time->data, "%y%m%d%H%M%SZ", &tm_time);
            strftime(&buf[0], sizeof(char) * 32, "%Y-%m-%dT%H:%M:%SZ", &tm_time);
            str = &buf[0];
        }
    }
}

static const char b64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void bytes_to_b64(const uint8_t *pbtData, const size_t szBytes, std::string &b64) {
        
    const ::std::size_t binlen = szBytes;
    
    ::std::size_t olen = (((binlen + 2) / 3) * 4);
    
//    if(fold)
//    {
//        olen += olen / 72; /* account for line feeds */
//    }

    // Use = signs so the end is properly padded.
    std::string retval(olen, '=');
    
    ::std::size_t outpos = 0;
    ::std::size_t line_len = 0;
    int bits_collected = 0;
    unsigned int accumulator = 0;
        
    for (size_t i = 0; i < szBytes; ++i) {
        accumulator = (accumulator << 8) | (pbtData[i] & 0xffu);
        bits_collected += 8;
        while (bits_collected >= 6) {
            bits_collected -= 6;
            retval[outpos++] = b64_table[(accumulator >> bits_collected) & 0x3fu];
//            if(fold)
//            {
//                line_len++;
//                if (line_len >= 72) {
//                    retval[outpos++] = '\n';
//                    line_len = 0;
//                }
//            }
        }
    }
    
    if (bits_collected > 0) { // Any trailing bits that are missing.
        accumulator <<= 6 - bits_collected;
        retval[outpos++] = b64_table[accumulator & 0x3fu];
    }
    
    b64 = retval;
}
    
void bytes_to_hex(const uint8_t *pbtData, const size_t szBytes, std::string &hex) {
    
    std::vector<uint8_t> buf((szBytes * 2) + 1);
    memset((char *)&buf[0], 0, buf.size());
    
    for (size_t i = 0; i < szBytes; ++i) {
        sprintf((char *)&buf[i * 2], "%02x", pbtData[i]);
    }
    
    hex = std::string((char *)&buf[0], (szBytes * 2));
}

void hex_to_bytes(std::string &hex, std::vector<uint8_t>& buf) {
    
    if(hex.length() % 2 == 0)
    {
        size_t len = hex.length() / 2;
        buf.resize(len);
        size_t pos = 0;
        const char digits[] = "0123456789abcdef";
        for(size_t i = 0; i < hex.length(); i+=2){
            const char *hi = strchr(digits, tolower(hex.at( i )));
            const char *lo = strchr(digits, tolower(hex.at(i+1)));
            char byte = 0;
            if(hi[0]>96){
                byte = (hi[0]-87) << 4;
            }
            else{
                byte = (hi[0]-48) << 4;
            }
            if(lo[0]>96){
                byte += (lo[0]-87);
            }
            else{
                byte += (lo[0]-48);
            }
            buf[pos++] = byte;
        }
    }
}
