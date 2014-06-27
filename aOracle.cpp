#include "StdAfx.h"
#include <string.h>
#include <strsafe.h>
#include "aOracle.h"

aOracle::aOracle(void)
{
    m_pEnv = NULL;
    m_pErr = NULL;
    m_pSvc = NULL;

    m_szUser[0] = '\0';
    m_nRC = OCI_SUCCESS;
    m_bConnected = false;

    Init();
}

aOracle::~aOracle(void)
{
    Shut();
}

void* aOracle::HandleAlloc(int nHType)
{
    void* pHandle = NULL;
    int nRC = OCIHandleAlloc((dvoid*)m_pEnv,
        (dvoid**)&pHandle, nHType, (size_t)0, (dvoid **)0);
    if (nRC < 0)
    {
#ifdef _DEBUG
        char szBuff[256];
        m_nRC = GetErrorText(szBuff, sizeof szBuff);
#else
        m_nRC = GetErrorCode();
#endif
        pHandle = NULL;
    }

    return pHandle;
}

int aOracle::GetErrorCode()
{
    int nErrCode = OCI_SUCCESS;
    OCIErrorGet((dvoid *)m_pErr, (ub4) 1, (text *) NULL, &nErrCode, (OraText*)NULL, 
        (ub4) 0, OCI_HTYPE_ERROR);

    return nErrCode;
}

int aOracle::GetErrorText(char* pszError, int nSize)
{
    int nErrCode = OCI_SUCCESS;
    OCIErrorGet((dvoid *)m_pErr, (ub4) 1, (OraText*) NULL,
        &nErrCode, (OraText*)m_szError, (ub4) sizeof m_szError, OCI_HTYPE_ERROR);

    if (pszError) {
        strcpy_s(pszError, nSize, m_szError);
        pszError[nSize - 1] = '\0';
    }

    return nErrCode;
}

int aOracle::Init()
{
    // Initialize OCI
    int nRC = OCIEnvCreate(&m_pEnv, (ub4) OCI_DEFAULT,
        (dvoid *) 0, (dvoid * (*)(dvoid *,size_t)) 0,
        (dvoid * (*)(dvoid *, dvoid *, size_t)) 0,
        (void (*)(dvoid *, dvoid *)) 0, (size_t) 0, (dvoid **) 0);

    // service context
    m_pSvc = (OCISvcCtx*)HandleAlloc(OCI_HTYPE_SVCCTX);
    // error context
    m_pErr = (OCIError*)HandleAlloc(OCI_HTYPE_ERROR);

    return OCI_SUCCESS;
}

int aOracle::Shut()
{
    int nRC = Logoff();

    if (m_pErr)
    {
        nRC = OCIHandleFree((dvoid *) m_pErr, OCI_HTYPE_ERROR);
        m_pErr = NULL;
    }
    if (m_pSvc)
    {
        nRC = OCIHandleFree((dvoid *) m_pSvc, OCI_HTYPE_SVCCTX);
        m_pSvc = NULL;
    }
    if (m_pEnv)
    {
        nRC = OCIHandleFree((dvoid *) m_pEnv, OCI_HTYPE_ENV);
        m_pEnv = NULL;
    }

    return nRC;
}

int aOracle::Logon(char* pszUser, char* pszPassword, char* pszHost)
{
    char szPassword[64], szHost[64];

	strcpy_s(m_szUser, sizeof m_szUser, pszUser);
    char* pChr = strchr(m_szUser, '@');
	if (pChr) {
		strncpy_s(szHost, sizeof szHost, pChr + 1, sizeof szHost);
        *pChr = '\0';
	}
	else {
		strncpy_s(szHost, sizeof szHost, pszHost, sizeof szHost);
	}

	// если пароль пропущен, поищем его в имени пользователя
	if (!pszPassword || !*pszPassword) {
		pChr = strchr(m_szUser, '/');
		if (!pChr) {
//			return m_nRC = OCI_NULL_PASSWORD_GIVEN;
            szPassword[0] = '\0';
		}
		else {
    		strncpy_s(szPassword, sizeof szPassword, pChr + 1, sizeof szPassword);
			*pChr = '\0';
		}
	}
	else {
		strncpy_s(szPassword, sizeof szPassword, pszPassword, sizeof szPassword);
	}

    //
    int nRC = OCILogon(m_pEnv, m_pErr, &m_pSvc,
        (OraText*)m_szUser, (ub4)strlen(m_szUser),
        (OraText*)szPassword, (ub4)strlen(szPassword),
        (OraText*)szHost, (ub4)strlen(szHost));

    if (nRC < OCI_SUCCESS)
    {
#ifdef _DEBUG
        char szBuff[256];
        m_nRC = GetErrorText(szBuff, sizeof szBuff);
#else
        m_nRC = GetErrorCode();
#endif
    }
    else
        m_bConnected = true;

    return nRC;
}

int aOracle::Logoff()
{
    if (!m_bConnected)
        return OCI_SUCCESS;

    int nRC = OCILogoff(m_pSvc, m_pErr);

    m_bConnected = false;

    return nRC;
}

aOraStmt::aOraStmt()
{
    Reset();

    m_pOra = NULL;
    m_pStmt = NULL;
}

aOraStmt::aOraStmt(aOracle* pOra)
{
    Reset();

    Open(pOra);
}

aOraStmt::~aOraStmt()
{
    Close();

    Reset();
}

void aOraStmt::Reset()
{
    m_bFormatQuery = true;
    m_nDefin = 1;
    m_nRPC = 0;
}

int aOraStmt::Open(aOracle* pOra)
{
    if (!pOra)
        return -1;

    m_pOra = pOra;

    // staintment context
    m_pStmt = (OCIStmt*)m_pOra->HandleAlloc(OCI_HTYPE_STMT);

    return m_pOra->GetRC();
}

int aOraStmt::Close()
{
    // Free handles
    int nRC = m_pStmt ? OCIHandleFree((dvoid *) m_pStmt, OCI_HTYPE_STMT) : 0;
    m_pStmt = NULL;

    //
    m_pOra = NULL;

    return nRC;
}

int aOraStmt::CheckError(int nRC)
{
char szErrBuf[512];

    m_nRC = nRC;
    switch (nRC)
    {
    case OCI_SUCCESS:
        break;
    case OCI_SUCCESS_WITH_INFO:
        //AfxMessageBox("Error OCI_SUCCESS_WITH_INFO");
        break;
    case OCI_NEED_DATA:
        break;
    case OCI_NO_DATA:
        //AfxMessageBox("Error OCI_NO_DATA\n");
        break;
    case OCI_ERROR:
        OCIErrorGet ((dvoid *) GetOra()->m_pErr, (ub4) 1, (text *) NULL, &m_nRC,
            (text*)szErrBuf, (ub4) sizeof(szErrBuf), (ub4) OCI_HTYPE_ERROR);
/*
        //
		if (m_bShowMessages
            && m_nRC != OCI_FETCHED_COLUMN_VALUE_IS_NULL
            && m_nRC != OCI_FETCH_OUT_OF_SEQUENCES)
        {
            char szBuff[128];
            int nPEO = 0;

            AttrGet(&nPEO, sizeof nPEO, OCI_ATTR_PARSE_ERROR_OFFSET);
            aOra8::GetParseErrorKey(nPEO, 
                GetQuery(), szBuff, sizeof szBuff);

            {
                sprintf_s(szError, sizeof szError,
                    "Ошибка выполнения %s\n%s\n%s\n\n%s", pszFrom, szErrBuf, szBuff,
                    m_pOra->m_bShowQueries ? LPCTSTR(m_strQuery) : ""
                    );
                AfxMessageBox(szError, MB_OK | MB_ICONERROR);
            }
    	}*/
        //AfxMessageBox(szBuff);
        break;
    case OCI_STILL_EXECUTING:
//        AfxMessageBox("Error OCI_STILL_EXECUTE");
        break;
    case OCI_CONTINUE:
        break;
    default:
        break;
    }

    return m_nRC;
}

int aOraStmt::Prepare(char* pszQuery, ...)
{
    va_list pData;

	if (!GetOra() || !GetOra()->IsConnected())
		return -1;

    Reset();

	if (m_bFormatQuery) {
		va_start(pData, pszQuery);
        StringCchVPrintfA(m_szQuery, sizeof m_szQuery, pszQuery, pData);
		va_end(pData);
	}
	else
		strcpy_s(m_szQuery, sizeof m_szQuery, pszQuery);

    int nRC = OCIStmtPrepare(m_pStmt, GetOra()->m_pErr, (OraText*)m_szQuery,
        (ub4) strlen(m_szQuery), (ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT);
    if (nRC != OCI_SUCCESS)
    {
#ifdef _DEBUG
        char szBuff[256];
        GetOra()->m_nRC = GetOra()->GetErrorText(szBuff, sizeof szBuff);
#else
        GetOra()->m_nRC = GetOra()->GetErrorCode();
#endif
    }

    return nRC;
}

int aOraStmt::BindByName(char* pszVar, void* pData, int nSize, int nType)
{
    OCIBind* bind;

    int nRC = OCIBindByName(m_pStmt, &bind, GetOra()->m_pErr,
        (OraText*)pszVar, strlen(pszVar),
        pData, (ub2)nSize, (ub2)nType,
        0, 0, 0, 0, 0, OCI_DEFAULT);
    if (nRC != OCI_SUCCESS)
    {
#ifdef _DEBUG
        char szBuff[256];
        GetOra()->m_nRC = GetOra()->GetErrorText(szBuff, sizeof szBuff);
#else
        GetOra()->m_nRC = GetOra()->GetErrorCode();
#endif
    }

    return nRC;
}

int aOraStmt::Defin(void* pData, int nSize, int nType, int* pnRetLen)
{
    OCIDefine* pDefin;

    // Define the select list items
    int nRC = OCIDefineByPos(m_pStmt, &pDefin/*&m_pDfn[m_nDefin - 1]*/,
        GetOra()->m_pErr, m_nDefin, (dvoid *) pData,
        (sword) nSize, (unsigned short)nType, (dvoid *) 0, (ub2 *)pnRetLen,
        (ub2 *)0, OCI_DEFAULT);

    m_nDefin++;

    return nRC;
}

int aOraStmt::Exec(int nFetchedRows)
{
    if (!m_pOra->IsConnected())
        return -1;

    // Execute the SQL statment
    int nRC = OCIStmtExecute(
        GetOra()->m_pSvc, m_pStmt, GetOra()->m_pErr,
        (ub4) nFetchedRows, (ub4) 0,
        (CONST OCISnapshot *) NULL, (OCISnapshot *) NULL, OCI_DEFAULT);
    GetOra()->m_nRC = GetOra()->GetErrorText(NULL, 0);

	switch (GetOra()->m_nRC) {
	case OCI_SUCCESS: // 0
		break;
	case OCI_FETCHED_COLUMN_VALUE_IS_NULL: // 1405
    case OCI_FETCHED_COLUMN_VALUE_WAS_TRUNCATED: // 1406
		nRC = OCI_SUCCESS;
        break;
    case OCI_END_OF_FILE_ON_COMMUNICATION_CHANNEL: // 3113
    case OCI_NOT_CONNECTED: // 3114
        m_pOra->Logoff();
        break;
	}

    return nRC;
}

int aOraStmt::Fetch(int nFetchedRows/* = 1*/)
{
    int nRC = OCIStmtFetch(m_pStmt, GetOra()->m_pErr, nFetchedRows, OCI_FETCH_NEXT, OCI_DEFAULT);
    CheckError(nRC);
    switch (m_nRC) {
    case OCI_SUCCESS:
        m_nRPC += nFetchedRows;
        break;
    case OCI_NO_DATA:
        break;
    // допустимые исключения для этой функции
    case OCI_FETCHED_COLUMN_VALUE_IS_NULL: // 1405
    case OCI_FETCHED_COLUMN_VALUE_WAS_TRUNCATED: // 1406
        m_nRPC += nFetchedRows;
        nRC = OCI_SUCCESS;
        break;
    default:
        break;
    }
/*
    for (i=0; i<m_nDefin; i++)
        if (m_pszDfn[i])
            *m_pstrDfn[i] = m_pszDfn[i];
*/
    return nRC;
}
