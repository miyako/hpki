#include "apdu.h"

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
