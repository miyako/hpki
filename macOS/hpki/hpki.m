#include "hpki.h"

#ifdef _CONSOLE_
int main(int argc, char *argv[]) {
    
    return hpki(argc, argv);
}
#else
INT WINAPI WinMain (HINSTANCE hInstance, 
                    HINSTANCE hPrevInstance,
                    PSTR lpCmdLine,
                    INT iCmdShow {
                        
    return 0;
}
#endif

static void list(void) {

    using namespace Json;
    Value threadCtx(objectValue);
    threadCtx["success"] = false;
    
    _get_slots(threadCtx);
    
    StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    builder["indentation"] = "   ";
    std::unique_ptr<Json::StreamWriter> writer(
    builder.newStreamWriter());
    writer->write(threadCtx, &std::cout);

    exit(0);
}

static void version(void) {
    fprintf(stderr, "hpki version %s\n", VERSION);
    exit(1);
}

static void usage(void) {
    
    fprintf(stderr, "usage: hpki [options...]\n");

    fprintf(stderr, "-c, --certificate type: print certificate\n");
    fprintf(stderr, "-s, --sign type: sign document\n");
    fprintf(stderr, "-o, --output path: output file path\n");
    fprintf(stderr, "-p, --pin4 val: specify scard short pin\n");
    fprintf(stderr, "-P, --pin6 val: specify scard long pin\n");
    fprintf(stderr, "-r, --reader val: specify scard reader\n");
    fprintf(stderr, "-m, --mynumber: print personal identification number\n");
    fprintf(stderr, "-i, --myinfo: print basic personal information\n");
    fprintf(stderr, "-l, --list: print scard readers\n");
    fprintf(stderr, "-a, --algorithm: digest info hash algorithm (sha1,sha256,sha512)\n");
    fprintf(stderr, "-v, --version: show version information\n");

    exit(1);
}

int hpki(int argc, char *argv[]) {

    if (argc == 1) {
                list();
            }
    
    int opt, longoptind;
    std::string pin4;
    std::string pin6;
    std::string slot;
    std::string type;
    std::string output;
    std::string hex;
    
    bool printMyNumber = false;
    bool printMyInformation = false;
    bool printCertificateSignature = false;
    bool printCertificateIdentity = false;
    bool signWithCertificateSignature = false;
    bool signWithCertificateIdentity = false;
    int inputIsStream = true;
    int outputIsStream = true;
    hash_algorithm algorithm = hash_algorithm_sha256;
    
    while ((opt = getopt_long(argc, argv, OPT_LIST, longopts, &longoptind)) != -1)
    {
        switch(opt)
        {
            case 'o':
                output = (optarg);
                outputIsStream = false;
                break;
            case 'a':
                type = (optarg);
                if(type == "sha1"){
                    algorithm = hash_algorithm_sha1;
                }
                if(type == "sha256"){
                    algorithm = hash_algorithm_sha256;
                }
                if(type == "sha512"){
                    algorithm = hash_algorithm_sha512;
                }
                break;
            case 'c':
                type = (optarg);
                if(type == "signature"){
                    printCertificateSignature = true;
                }
                if(type == "identity"){
                    printCertificateIdentity = true;
                }
                break;
            case 's':
                type = (optarg);
                if(type == "signature"){
                    signWithCertificateSignature = true;
                }
                if(type == "identity"){
                    signWithCertificateIdentity = true;
                }
                break;
            case 'r':
                slot = (optarg);
                break;
            case 'p':
                pin4 = (optarg);
                break;
            case 'P':
                pin6 = (optarg);
                break;
            case 'm':
                printMyNumber = true;
                break;
            case 'i':
                printMyInformation = true;
                break;
            case 'l':
                list();
                break;
            case 'v':
                version();
                break;
            default:
                usage();
                break;
        }
        
    }
    
    using namespace Json;
    Value threadCtx(objectValue);
    threadCtx["success"] = false;
    threadCtx["pin4"] = pin4;
    threadCtx["pin6"] = pin6;
    threadCtx["slotName"] = slot;
    
    if(signWithCertificateSignature | signWithCertificateIdentity) {
        
        argc -= optind;
        argv += optind;
        
        if(argc) inputIsStream = 0;

        unsigned int size = BUFFER_SIZE;
        unsigned char *buf = (unsigned char *)calloc(size, sizeof(char));
        
        if(!buf)
        {
            exit(1);
        }

        unsigned int ret, i = 0;
        FILE *fp = NULL;
        
        if(inputIsStream) {
            fp = stdin;
        }else{
            const char *input = argv[0];
    #ifdef __WINDOWS__
            fp = ufopen(input, L"rb");
    #else
            fp = fopen(input, "rb");
    #endif
        }
        while((ret = (unsigned int)fread(&buf[i], sizeof(char), BUFFER_SIZE, fp)) > 0)
        {
            i += ret;
            size += BUFFER_SIZE;
            buf = (unsigned char *)realloc(buf, size);
            if(!buf)
            {
                exit(1);
            }
        }
        if(inputIsStream) {
            fclose(fp);
        }
        
        int DIGEST_INFO_size;
        const void *DIGEST_INFO;
        const EVP_MD *md;
        size_t DIGEST_INFO_start;
        unsigned char *(*sha)(const unsigned char*, size_t, unsigned char*);
        
        switch(algorithm) {
            case hash_algorithm_sha1:
                DIGEST_INFO_size = sizeof(DIGEST_INFO_1);
                DIGEST_INFO = DIGEST_INFO_1;
                md = EVP_sha1();
                DIGEST_INFO_start = 15;
                sha = SHA1;
                break;
            case hash_algorithm_sha512:
                DIGEST_INFO_size = sizeof(DIGEST_INFO_512);
                DIGEST_INFO = DIGEST_INFO_512;
                md = EVP_sha512();
                DIGEST_INFO_start = 19;
                sha = SHA512;
                break;
            default:
                DIGEST_INFO_size = sizeof(DIGEST_INFO_256);
                DIGEST_INFO = DIGEST_INFO_256;
                md = EVP_sha256();
                DIGEST_INFO_start = 19;
                sha = SHA256;
                break;
        }
        
        std::vector<uint8_t>digestInfo(DIGEST_INFO_size);
        memcpy(&digestInfo[0],
               DIGEST_INFO,
               DIGEST_INFO_size);
        
        uint8_t *_buf = &digestInfo[DIGEST_INFO_start];
        sha((unsigned char *)buf, i, _buf);
        free(buf);

        bytes_to_hex(&digestInfo[0], digestInfo.size(), hex);

        threadCtx["digestInfo"] = hex;
        threadCtx["algorithm"] = algorithm;
    }
    
    if(signWithCertificateSignature) {
        threadCtx["certificateType"] = certificate_type_signature;
        _sign_with_certificate(threadCtx);
    }else
    if(signWithCertificateIdentity) {
        threadCtx["certificateType"] = certificate_type_identity;
        _sign_with_certificate(threadCtx);
    }else
    if(printCertificateIdentity) {
        threadCtx["certificateType"] = certificate_type_identity;
        _get_my_certificate(threadCtx);
    }else
    if(printCertificateSignature) {
        threadCtx["certificateType"] = certificate_type_signature;
        _get_my_certificate(threadCtx);
    }else{
        if(printMyNumber) {
            _get_my_number(threadCtx);
        }
        
        if(printMyInformation) {
            _get_my_information(threadCtx);
        }
    }

    threadCtx.removeMember("pin4");
    threadCtx.removeMember("pin6");
    threadCtx.removeMember("size");
    threadCtx.removeMember("block");
    threadCtx.removeMember("type");
    threadCtx.removeMember("data");
    threadCtx.removeMember("certificateType");
    
    if(threadCtx["historicalBytes"].isString()){
        std::string historicalBytes = threadCtx["historicalBytes"].asString();
        if(historicalBytes == ID_HPKI) {
            threadCtx["cardType"] = NAME_HPKI;
        }
        if(historicalBytes == ID_JPKI) {
            threadCtx["cardType"] = NAME_JPKI;
        }
        if(historicalBytes == ID_JGID) {
            threadCtx["cardType"] = NAME_JPKI;
        }
        threadCtx.removeMember("historicalBytes");
    }
    
    StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    builder["indentation"] = "   ";
    std::unique_ptr<StreamWriter> writer(
    builder.newStreamWriter());
    std::stringstream s;
    writer->write(threadCtx, &s);
    
    if(!outputIsStream)
    {
        FILE *out = NULL;
#ifdef __WINDOWS__
        out = ufopen(output.c_str(), L"wb");
#else
        create_parent_folder(output.c_str());
        out = fopen(output.c_str(), "wb");
#endif
        if(!out)
        {
            exit(1);
        }else{
            fwrite(s.str().c_str(), s.str().length(), sizeof(unsigned char), out);
            fclose(out);
        }
    }
    else
    {
        std::cout << s.str() << std::endl;
    }
        
    return 0;
}

#ifdef __APPLE__
void create_parent_folder(const char *utf8_path) {
    
    NSString *filePath = (NSString *)CFBridgingRelease(CFStringCreateWithFileSystemRepresentation(kCFAllocatorDefault, utf8_path));
    
    switch ([[filePath pathComponents]count]) {
            
        case 0:
        case 1:

            break;
        default:
        {
            NSString *path = (NSString *)CFBridgingRelease(CFStringCreateWithFileSystemRepresentation(kCFAllocatorDefault, [[filePath stringByDeletingLastPathComponent]fileSystemRepresentation]));
            [[NSFileManager defaultManager] createDirectoryAtPath:path
                                       withIntermediateDirectories:YES
                                                        attributes:nil
                                                             error:NULL];
        }
            break;
    }

}
#endif

#ifdef __WINDOWS__
void create_parent_folder(const char *utf8_path) {
    
    wchar_t fDrive[_MAX_DRIVE], fDir[_MAX_DIR], fName[_MAX_FNAME], fExt[_MAX_EXT];
    wchar_t utf16_path[_MAX_PATH] = {0};
    size_t utf8_len = strlen(utf8_path);
    size_t pos = 0;
    int len = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)utf8_path, utf8_len, NULL, 0);
    if(len){
        if(MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)utf8_path, utf8_len, (LPWSTR)utf16_path, len)){
            _wsplitpath_s(utf16_path, fDrive, fDir, fName, fExt);
            pos = wcslen(fDrive) + wcslen(fDir);
            utf16_path[pos] = 0x0;
            SHCreateDirectory(NULL, (PCWSTR)utf16_path);
        }
    }
}
FILE *ufopen(const char *utf8_path, const wchar_t *mode) {
    
    create_parent_folder(utf8_path);
    wchar_t utf16_path[_MAX_PATH] = {0};
    size_t utf8_len = strlen(utf8_path);
    int len = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)utf8_path, utf8_len, NULL, 0);
    if(len){
        if(MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)utf8_path, utf8_len, (LPWSTR)utf16_path, len)){
            return _wfopen(utf16_path, mode);
        }
    }
    return 0;
}
#endif
