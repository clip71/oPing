/*
 * oPing - Analog of tnsping
 *
 * version 1.1.11 - 18.05.2015
 *
 * Copyright (C) 2014-15 OJSC Introtest, Tyumenska region, Surgut
 * Konstantin Slabouzov <slabouzov@introtest.com>
 *
 */

#include "stdafx.h"

#pragma comment(lib, "oci.lib")

#include <windows.h>
#include <oci.h>
#include "aOracle.h"

static char szConnect[1024];
static aOracle lda;


BOOL WINAPI ControlHandler(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_C_EVENT:		// SERVICE_CONTROL_STOP in debug mode
	case CTRL_BREAK_EVENT:	// use Ctrl+C or Ctrl+Break to simulate
    case CTRL_CLOSE_EVENT:
        _putts("Ctrl+C signaled");
        ExitProcess(0);
        exit(0);
		return TRUE;
	}
	return FALSE;
}


void skipSpaces(char** pPointer)
{
    while ((**pPointer == ' ' || **pPointer == '\t' || **pPointer == '\n') && **pPointer)
        (*pPointer)++;
}


int getToken(char* pszToken, char** pPointer)
{
    skipSpaces(pPointer);

    while (isalpha(**pPointer) || isdigit(**pPointer) || **pPointer == '.' || **pPointer == '_')
    {
        *pszToken++ = *(*pPointer)++;
    }

    skipSpaces(pPointer);
    *pszToken = '\0';

    return 0;
}


int analyzeTns(const char* pszAlias)
{
    char szTnsAdmin[MAX_PATH];
    char szPath[MAX_PATH];
    size_t requiredSize;

    szTnsAdmin[0] = '\0';
    getenv_s(&requiredSize, szTnsAdmin, sizeof szTnsAdmin, "TNS_ADMIN");
    if (szTnsAdmin[0] == '\0')
        return 1;

    strcpy_s(szPath, sizeof szPath, szTnsAdmin);
    if (szPath[strlen(szPath)-1] != '\\')
        strcat_s(szPath, sizeof szPath, "\\");
    strcat_s(szPath, sizeof szPath, "tnsnames.ora");

    FILE* f = NULL;
    fopen_s(&f, szPath, "rt");
    if (!f) {
        return 2;
    }

    char szTns[1024 * 32] = "";
    char szBuff[1024];
    char* p;

    // read & filter garbage
    for (;;)
    {
        if (fgets(szBuff, sizeof szBuff, f) == NULL)
            break;

        p = szBuff;
        skipSpaces(&p);
        if (*p == '\0' || *p == '#')
            continue;

        if (p[strlen(p)-1] == '\n')
            p[strlen(p)-1] = '\0';
        strcat_s(szTns, sizeof szTns, p);
        strcat_s(szTns, sizeof szTns, " ");
    }

    //
    int nBrackets = 0;
    char* pFound = NULL;
    for (p=szTns; *p; p++)
    {
        if (*p == '(') {
            nBrackets++;
            continue;
        }
        if (*p == ')') {
            nBrackets--;
            if (pFound && nBrackets == 0) {
                szConnect[p - pFound] = '\0';
                break;
            }
            continue;
        }

        if (nBrackets == 0)
        {
            getToken(szBuff, &p);
            if (*p++ != '=')
                break;

            if (_stricmp(pszAlias, szBuff)) {
                continue;
            }

            // alias founded
            pFound = p + 1;
            strncpy_s(szConnect, sizeof szConnect, p, (sizeof szConnect) - 1);
            continue;
        }
    }

    return 0;
}


int _tmain(int argc, _TCHAR* argv[])
{
    _tprintf(_T("\nTNS Ping Utility for windows: Version 1.1\n\n"));
    _tprintf(_T("Copyright (c) 2013-14, OJSC Introtest, Surgut Russia. clip@introtest.net\n\n"));

    if (argc < 2) {
        _tprintf(_T("Insufficient arguments.  Usage:  oping <address> [<count>]\n"));
        return 1;
    }

    char szError[512];
    DWORD dwTics;
    int nRC;

    int nCount = 1;
    if (argc >= 3) {
        nCount = atoi(argv[2]);
    }

    //
    analyzeTns(argv[1]);
    if (szConnect[0])
        _tprintf("Attempting to contact %s\n", szConnect);

	SetConsoleCtrlHandler(ControlHandler, TRUE);

    //
    for (int n=0; n<nCount; n++)
    {
        dwTics = GetTickCount();

        //int nNonBlock = 1;
        //nRC = lda.AttrSetEnv(OCI_ATTR_NONBLOCKING_MODE, &nNonBlock, sizeof nNonBlock);
        //nRC = lda.GetErrorText(szError, sizeof szError);
        nRC = lda.ServerAttach(argv[1]);

        dwTics = GetTickCount() - dwTics;

        switch (nRC) {
        case OCI_SUCCESS:
            strcpy_s(szError, sizeof szError, "OK");
            lda.ServerDetach();
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
                case OCI_FATAL_TWO_TASK_COMMUNICATION_PROTOCOL: // ?
                case OCI_TNS_ILLEGAL_ADDRESS_PARAMETERS: // 12533
                case OCI_TNS_LOST_CONTACT: // 12547
                case OCI_TNS_PACKET_CHECKSUM_FAILURE:
                case OCI_TNS_PACKET_READER_FAILURE:
                case OCI_TNS_BAD_PACKET:
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
        Sleep(1000);
    }

	return 0;
}
