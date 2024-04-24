#ifndef __HPKI_H__
#define __HPKI_H__

#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "json/json.h"
#include "My-Number-Card.h"

int hpki(int argc, char * argv[]);

#ifdef __APPLE__

#endif

#include "getopt.h"

#define VERSION "1.0.0"

#define OPT_LIST "p:P:r:c:s:milv"

struct option longopts[] = {
    {"help", no_argument, NULL, '?'},
    {"output", required_argument, NULL, 'o'},
    {"pin4", required_argument, NULL, 'p'},
    {"pin6", required_argument, NULL, 'P'},
    {"reader", required_argument, NULL, 'r'},
    {"myinfo", no_argument, NULL, 'i'},
    {"sign", required_argument, NULL, 's'},
    {"certificate", required_argument, NULL, 'c'},
    {"mynumber", no_argument, NULL, 'm'},
    {"list", no_argument, NULL, 'l'},
    {"version", no_argument, NULL, 'v'},
    {0, 0, 0, 0}
};

#define BUFFER_SIZE 8192

#ifdef __APPLE__
#include <Foundation/Foundation.h>
void create_parent_folder(const char *utf8_path);
#endif

#ifdef __WINDOWS__
#include "Shlobj.h"
#include "Windows.h"
void create_parent_folder(const char *utf8_path);
#endif

#endif
