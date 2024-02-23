#pragma once


typedef struct creds {
    char ssid[20];
    char pw[20];
} WifiCreds_t;


const WifiCreds_t WIFICREDS[] = {
    {{.ssid = "myssid"}, {.pw = "mypw"}},
    // {{.ssid = ""}, {.pw = ""}},
};
