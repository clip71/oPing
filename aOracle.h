#pragma once

#include "oci.h"
#include <string.h>

#define OCI_NO_ERRORS                               (0)
#define OCI_INVALID_IDENTIFIER                      (904)
#define OCI_TABLE_DOES_NOT_EXIST                    (942)
#define OCI_INVALID_CURSOR                          (1001)
#define OCI_FETCH_OUT_OF_SEQUENCES		            (1002)
#define OCI_NULL_PASSWORD_GIVEN                     (1005)
#define OCI_VARIABLE_NOT_IN_SELECT_LIST	            (1007)
#define OCI_INVALID_USERNAME_PASSWORD               (1017)
#define OCI_ILLEGAL_VARIABLE_NAME_NUMBER            (1036)
#define ORA_USER_DOES_NOT_EXIST                     (1918)

// значения возвращаемые OCIErrorGet, хранится в m_nRC
// OCI_NO_DATA - это результат возвращаемый родными функциями OCI
#define OCI_NO_DATA_FOUND					        (1403)
#define OCI_NO_MORE_ROWS					        (1403)

#define OCI_FETCHED_COLUMN_VALUE_IS_NULL	        (1405)
#define OCI_FETCHED_COLUMN_VALUE_WAS_TRUNCATED      (1406)
#define OCI_CHILD_RECORD_FOUND			            (2292)
#define OCI_FATAL_TWO_TASK_COMMUNICATION_PROTOCOL   (3106)
#define OCI_END_OF_FILE_ON_COMMUNICATION_CHANNEL    (3113)
#define OCI_NOT_CONNECTED                           (3114)
#define OCI_OBJECT_DOES_NOT_EXIST                   (4043)
#define OCI_ERRORS_DURING_RECOMPILATION             (4045)
#define OCI_STATE_PACKAGES_DISCARDED                (4068)
#define OCI_PLSQL_NUMERIC_OR_VALUE_ERROR            (6502)
#define OCI_PLSQL_COMPILATION_ERROR                 (6550)

#define OCI_TNS_COULD_NOT_RESOLVE_SERVICE_NAME      (12154)
#define OCI_TNS_CONNECT_TIMEOUT_OCCURRED            (12170)
#define OCI_TNS_OPERATION_TIMED_OUT                 (12535)
#define OCI_TNS_PROTOCOL_ADAPTER_ERROR              (12560)
#define OCI_TNS_PACKET_CHECKSUM_FAILURE             (12569)
#define OCI_ALREADY_CONNECTED_TO_A_SERVER           (24309)
#define OCI_PASSWORD_WILL_EXPIRE                    (28002)

class aOracle
{
    friend class aOraStmt;

public:
    aOracle(void);
    ~aOracle(void);

	inline bool IsConnected() const { return m_bConnected; }
    inline int GetRC() const { return m_nRC; };
    inline OCIEnv* getEnvHandle() const { return m_pEnv; };
    inline OCIError* getErrorHandle() const { return m_pErr; };
	inline void Commit() { OCITransCommit(m_pSvc, m_pErr, (ub4)0); };

    inline void GetUserName(char* pszUser, int nSize) { strcpy_s(pszUser, nSize, m_szUser); };
    int GetErrorCode();
    int GetErrorText(char* pszError, int nSize);
    int Init();
    int Shut();
    int ServerAttach(char* pszHost);
    int ServerDetach();
    int Logon(char* pszUser, char* pszPassword, char* pszHost);
    int Logoff();

    inline int AttrSetSrv(ub4 nAttr, void* pData, int nLen) {
        return OCIAttrSet(m_pSrv, OCI_HTYPE_SERVER, pData, (ub4) nLen, (ub4) nAttr, m_pErr);
    };
    inline int Break() { return OCIBreak(m_pSvc, m_pErr); };
    inline int Reset() { return OCIReset(m_pSvc, m_pErr); };

protected:
    void* HandleAlloc(int nHType);
public:
    OCIEnv* m_pEnv;
    OCIError* m_pErr;
    OCISvcCtx* m_pSvc;
    OCIServer* m_pSrv;

    char m_szUser[64];
    char m_szError[128];
    bool m_bConnected;
    int m_nRC;
};

class aOraStmt
{
public:
    aOraStmt();
    aOraStmt(aOracle* pOra);
    ~aOraStmt();

    inline aOracle* GetOra() const { return m_pOra; };
    inline bool IsOpened() const { return m_pStmt != NULL; };
    inline int GetRPC() { return m_nRPC; };
    inline int GetRC() { return m_nRC; };

    void Reset();
    int Open(aOracle* pOra);
    int Close();
    int CheckError(int nRC);
    int Prepare(char* pszQuery, ...);
    int BindByName(char* pszVar, void* pData, int nSize, int nType);
    int Defin(void* pData, int nSize, int nType, int* pnRetLen = NULL);
    int Exec(int nFetchedRows);
    int Fetch(int nFetchedRows = 1);

    aOracle* m_pOra;
    OCIStmt* m_pStmt;
    char m_szQuery[2048];
    bool m_bFormatQuery;
    int m_nDefin;
    int m_nRPC;
    int m_nRC;
};
