// oPing.cpp: определ€ет точку входа дл€ консольного приложени€.
//

#include "stdafx.h"

#pragma comment(lib, "oci.lib")

#include <windows.h>
#include <oci.h>
#include "aOracle.h"

int _tmain(int argc, _TCHAR* argv[])
{
    _tprintf(_T("\nTNS Ping Utility for windows: Version 1.0\n\n"));
    _tprintf(_T("Copyright (c) 2013, OJSC Introtest, Surgut Russia. clip@introtest.net\n\n"));

    if (argc < 2) {
        _tprintf(_T("Insufficient arguments.  Usage:  oping <address> [<count>]\n"));
        return 1;
    }

    aOracle lda;
    char szError[512];
    DWORD dwTics;
    int nRC;

    int nCount = 1;
    if (argc >= 3) {
        nCount = atoi(argv[2]);
    }
/*
    OCIServer* pServer = NULL;
    nRC = OCIHandleAlloc(lda.getEnvHandle(), (dvoid**)&pServer, OCI_HTYPE_SERVER, 0, 0);
    nRC = OCIServerAttach(pServer, lda.getErrorHandle(), (text*)argv[1], strlen(argv[1]), OCI_DEFAULT);
    nRC = lda.GetErrorText(szError, sizeof szError);
*/
    for (int n=0; n<nCount; n++)
    {
        dwTics = GetTickCount();

        nRC = lda.Logon("x", "x", argv[1]);

        dwTics = GetTickCount() - dwTics;

        switch (nRC) {
        case OCI_SUCCESS:
            strcpy_s(szError, sizeof szError, "OK");
            break;
        default:
            if (nRC == -1) {
                nRC = lda.GetErrorText(szError, sizeof szError);
                switch (nRC) {
                case OCI_INVALID_USERNAME_PASSWORD:
                    strcpy_s(szError, sizeof szError, "OK");
                    break;
                case OCI_TNS_CONNECT_TIMEOUT_OCCURRED: // при таймауте цикл не прерываем
                    break;
                default:
                    nCount = 0; // прервем цикл
                    break;
                }
            }
            break;
        }

        if (lda.IsConnected())
            lda.Logoff();

        _tprintf("%s (%d msec)\n", szError, dwTics);
    }

	return 0;
}
