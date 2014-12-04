/*
**
**	ODBCArrayUpdatesU.c
**	written by Gordon Crenshaw from Progress DataDirect
**
**	This application was originally written and compiled in Visual Studio 2013 Express.  When compiling this application in Visual Studio, create
**	a Win32 Console Application template.  In the application wizard, create an empty project.  Since this code is written in C, you'll need to
**	change the default compiler.  Select the project name from the Project Explorer.  From the main menu, select Project -> Preferences.  Under
**	C/C++, select All Options.  Change the Compile As property from Default to Compile As C Code(/TC)
**
*/

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sql.h>
#include <sqlext.h>
#include <sqlucode.h>
#include <sys/timeb.h>

#define	DESC_LEN	50
#define	ARRAY_SIZE	1000  // Should be set to the maximum ArraySize[] value
#define ARRAY_ITEMS(x)	(sizeof(x)/sizeof(x[0]))

void displayError(SQLSMALLINT handleType, SQLHANDLE handle);
static int getLoginInfo(char *useridStr, char *username, char *password, char *datasource);
void InitODBCEnv(SQLHANDLE *henv);
void ODBCConnect(SQLHANDLE henv, SQLHANDLE *hdbc, wchar_t *username, wchar_t *password, wchar_t *dataSource);
void ArrayUpdate(SQLHANDLE hdbc, SQLHANDLE hstmt, wchar_t *TableName, unsigned int SizeOfArray, unsigned int NumberRecords);
void Terminate(SQLHANDLE henv, SQLHANDLE hdbc, SQLHANDLE hstmt);
void PrintTableHeader();
void ClearTable(SQLHANDLE hstmt, wchar_t *TableName);
void InitODBCStmt(SQLHANDLE hdbc, SQLHANDLE *hstmt);
unsigned int SelectCount(SQLHANDLE hstmt, wchar_t *TableName);
void CreateTable(SQLHANDLE hstmt, wchar_t *TableName);
void DropTable(SQLHANDLE hstmt, wchar_t *TableName);
void ArrayInsert(SQLHANDLE hdbc, SQLHANDLE hstmt, wchar_t *TableName, unsigned int SizeOfArray, unsigned int NumberRecords);

static const unsigned int		ArraySize[] = {1,10,50,100,1000};	// Set ARRAY_SIZE define to maximum value
static const unsigned int		ArraySizeLen = ARRAY_ITEMS(ArraySize);
static const unsigned int		NumRecUpdated[] = {100,500,1000,5000};
static const unsigned int		NumRecUpdatedLen = ARRAY_ITEMS(NumRecUpdated);


void main(int argc, char ** argv)
{
  
	SQLHANDLE	henv;
	SQLHANDLE	hdbc;
	SQLHANDLE	hstmt;

	wchar_t	username[256];
	wchar_t	password[256];
	wchar_t	datasource[256];
	unsigned int		i,
		j;
  
	wchar_t	TableName[50];

	wcscpy(datasource, L"OracleXE");
	//wcscpy(datasource, L"SQLServer Express");
	//wcscpy(datasource, L"Microsoft SQL Server");
	wcscpy(username, L"test");
	wcscpy(password, L"test");
	wcscpy(TableName, L"EMPLOYEES_PERFTEST");

  
/*	if (argc < 2) {
		fprintf (stderr, "Usage: ODBCArrayUpdate username/password@datasource\n\n");
		exit(255);
	}
*/

/*	if (getLoginInfo(argv[1], username, password, datasource) != SQL_SUCCESS) {
		fprintf (stderr, "Error: ' %s ' invalid login\n", argv[1]);
		fprintf (stderr, "Usage: ODBCArrayUpdate username/password@datasource\n\n");
		exit(255);
	}
*/  
	wprintf(L"\nODBC ARRAY UPDATE TEST\n\n");

	InitODBCEnv(&henv);
	ODBCConnect(henv, &hdbc, username, password, datasource);
	InitODBCStmt(hdbc, &hstmt);
  
	wprintf(L"\nSETTING UP ENVIRONMENT\n");
	wprintf(L"Dropping table: %s\n", TableName);
	DropTable(hstmt, TableName);

	wprintf(L"Creating table: %s\n", TableName);
	CreateTable(hstmt, TableName);

	wprintf(L"Filling table: %s\n", TableName);
	ArrayInsert(hdbc, hstmt, TableName, 1000, 5000);

	wprintf(L"\n\n");
	PrintTableHeader();

	for (i = 0; i < ArraySizeLen; i++) {
		for (j = 0; j < NumRecUpdatedLen; j++) {
			ArrayUpdate(hdbc, hstmt, TableName, ArraySize[i], NumRecUpdated[j]);
			wprintf(L"	resetting test...");
			ClearTable(hstmt, TableName);
			ArrayInsert(hdbc, hstmt, TableName, 1000, 5000);
			wprintf(L"done\n");
		}
		wprintf(L"\n");
	}

    Terminate(henv, hdbc, hstmt);

	wprintf(L"ODBC ARRAY UPDATE TEST COMPLETED.\n\n");
	wprintf(L"===========================================================\n");
	wprintf(L"Press ENTER to end program...");
	getchar();

}

void displayError(SQLSMALLINT handleType, SQLHANDLE handle)
{
	SQLSMALLINT	recNumber = 1;
	SQLWCHAR	sqlstate[10];
	SQLWCHAR	errmsg[SQL_MAX_MESSAGE_LENGTH];
	SDWORD	nativeerr;
	SWORD	actualmsglen;
	RETCODE	rc;

loop:  	rc = SQLGetDiagRecW(handleType, handle, recNumber, (SQLWCHAR *)sqlstate, &nativeerr, (SQLWCHAR *)errmsg, SQL_MAX_MESSAGE_LENGTH - 1, &actualmsglen);

	if (rc != SQL_SUCCESS) {
		wprintf(L"SQLGetDiagRec failed!\n");
		return;
	}

	if (rc != SQL_NO_DATA_FOUND) {
		wprintf(L"SQLSTATE = %s\n", sqlstate);
		wprintf(L"NATIVE ERROR = %d\n", nativeerr);
		errmsg[actualmsglen] = '\0';
		wprintf(L"MSG = %s\n\n", errmsg);
		recNumber++;
		goto loop;
	}
}

static int getLoginInfo(char *useridStr, char *username, char *password,
                        char *datasource)
{
  int rc = 0, pos = 0;
  char *ptr, *ptr2;
  
  ptr = strchr(useridStr, '/');
  /* Get username out of user/password@dblink */
  if (!ptr) {
    return 1;
  }
  pos = (ptr - useridStr);
  strncpy(username, useridStr, pos);
  username[pos] = '\0';
  
  /* Get password out of user/password@dblink */
  pos = 0;
  ptr++;
  ptr2 = strchr(ptr, '@');
  if (!ptr2) {
    return 1;
  }
  pos = (ptr2 - ptr);
  strncpy(password, ptr, pos);
  password[pos] = '\0';
  
  /* Check if dblink is set. */
  ptr2++;
  strcpy(datasource, ptr2);                   
  
  return rc;
} 


void InitODBCEnv (SQLHANDLE *henv)
{
	RETCODE	rc;

	// Allocate an Environment Handle
	rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, henv);
	if (rc != SQL_SUCCESS) {
		wprintf (L"Couldn't initialize ODBC environment.\n");
		exit(255);
	}
	
	rc = SQLSetEnvAttr(*henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, SQL_IS_INTEGER);
	if (rc != SQL_SUCCESS) {
		wprintf (L"Couldn't specify ODBC version.\n");
		exit(255);
	}
}

void ODBCConnect(SQLHANDLE henv, SQLHANDLE *hdbc, wchar_t *username, wchar_t *password, wchar_t *dataSource)
{
	SQLRETURN rc;

	rc = SQLAllocHandle(SQL_HANDLE_DBC, henv, hdbc);
	if (rc != SQL_SUCCESS) {
		wprintf(L"Couldn't initialize ODBC connection.\n");
		exit(255);
	}

	wprintf(L"===========================================================\n");
	wprintf(L"Connecting to: %s\n", dataSource);
	wprintf(L"     User:     %s\n", username);
	wprintf(L"     Password: %s\n", password);
	wprintf(L"\n");
	rc = SQLConnectW(*hdbc, dataSource, wcslen(dataSource), username, wcslen(username), password, wcslen(password));
	if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
		wprintf(L"Did not connect to %s\n", dataSource);
		displayError(SQL_HANDLE_DBC, *hdbc);
		exit(255);
	}
	else {
		wprintf(L"CONNECTED!\n", dataSource);
	}
	wprintf(L"===========================================================\n");

}

void InitODBCStmt(SQLHANDLE hdbc, SQLHANDLE *hstmt)
{
	RETCODE	rc;

	// Allocate an Environment Handle
	rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, hstmt);
	if (rc != SQL_SUCCESS) {
		wprintf(L"Couldn't initialize ODBC environment.\n");
		exit(255);
	}

}


void ArrayUpdate (SQLHANDLE hdbc, SQLHANDLE hstmt, wchar_t *TableName, unsigned int SizeOfArray, unsigned int NumberRecords)
{
	
	SQLRETURN	rc;

	wchar_t	Statement[1000];
	int		EmpID = 10000;

	SQLCHAR	FirstNameArray[ARRAY_SIZE][DESC_LEN];
	SQLCHAR	LastNameArray[ARRAY_SIZE][DESC_LEN];
	SQL_DATE_STRUCT		HireDateArray[ARRAY_SIZE];
	int	SalaryArray[ARRAY_SIZE];
	int	EmpIDArray[ARRAY_SIZE];



	SQLLEN	FirstNameIndArray[ARRAY_SIZE],
		LastNameIndArray[ARRAY_SIZE],
		EmpIDIndArray[ARRAY_SIZE],
		HireDateIndArray[ARRAY_SIZE],
		SalaryIndArray[ARRAY_SIZE];

	SQLINTEGER	i,
		NumRecords,
		Loop,
		RowCount,
		NumRowsAffected; 
	SQLUSMALLINT	ParamStatusArray[ARRAY_SIZE];
	SQLULEN		ParamsProcessed;


	struct _timeb timeBuffer;
	unsigned long beginMSec,
		endMSec;

	wcscpy(Statement, L"update ");
	wcscat(Statement, TableName);
	wcscat(Statement, L" set FIRST_NAME = ?, LAST_NAME = ?, HIRE_DATE = ?, SALARY = ? where EMP_ID = ?");


	RowCount = 0;
	NumRowsAffected = 0;

	// Set AUTOCOMMIT off
	rc = SQLSetConnectAttr (hdbc, SQL_ATTR_AUTOCOMMIT,(SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER);
	
	// Set the SQL_ATTR_PARAM_BIND_TYPE statement attrivute to use column-wise binding
	rc = SQLSetStmtAttr (hstmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0);
	
	// Specify the number of elements in each parameter array
	//fprintf(stderr, "Setting SQLSetStmtAttr->SQL_ATTR_PARAMSET_SIZE for %d row(s) in array...", ARRAY_SIZE);
	//rc = SQLSetStmtAttr (hstmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)ARRAY_SIZE, 0);
	//fprintf(stderr, "done!\n");

	// Specify an array in which to return the status of each set of parameters.
	rc = SQLSetStmtAttr (hstmt, SQL_ATTR_PARAM_STATUS_PTR, ParamStatusArray, 0);
	
	// Specify an SQLUINTEGER value in which to return the number of sets of parameters
	// processed.
	rc = SQLSetStmtAttr(hstmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &ParamsProcessed, 0);
	
	// Bind the parameters in column-wise fashion
	rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0,
		FirstNameArray, DESC_LEN, FirstNameIndArray);
	rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 30, 0,
		LastNameArray, DESC_LEN, LastNameIndArray);
	rc = SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_TYPE_DATE, SQL_TYPE_TIMESTAMP, 19, 0,
		HireDateArray, 19, HireDateIndArray);
	rc = SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 9, 0,
		SalaryArray, sizeof(int), SalaryIndArray);
	rc = SQLBindParameter(hstmt, 5, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 6, 0,
		EmpIDArray, sizeof(int), EmpIDIndArray);

	rc = SQLPrepareW(hstmt, Statement, wcslen(Statement));
	if (rc != SQL_SUCCESS) {
		wprintf(L"SQLPrepare failed\n");
		displayError(SQL_HANDLE_STMT, hstmt);
		exit(255);
	}

	// Obtain start time
	_ftime(&timeBuffer);
	beginMSec = (unsigned long)(timeBuffer.time * 1000) + timeBuffer.millitm;

	NumRecords = NumberRecords;
	i = 0;

	while (NumRecords > 0) {

		if (NumRecords < (SQLINTEGER)SizeOfArray) {
			Loop = NumRecords;
		}
		else {
			Loop = SizeOfArray;
		}

		// Specify the number of elements in each parameter array
		rc = SQLSetStmtAttr(hstmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)Loop, 0);

		// Set values for update
		for (i = 0; i < Loop; i++) {
			strcpy((char *)FirstNameArray[i], "Barney");
			FirstNameIndArray[i] = strlen(FirstNameArray[i]);

			strcpy((char *)LastNameArray[i], "Rubble");
			LastNameIndArray[i] = strlen(LastNameArray[i]);

			HireDateArray[i].year = 2014;
			HireDateArray[i].month = 5;
			HireDateArray[i].day = 01;
			HireDateIndArray[i] = 0;

			SalaryArray[i] = 20000;
			SalaryIndArray[i] = 0;

			EmpID++;
			EmpIDArray[i] = EmpID;
			EmpIDIndArray[i] = 0;
			
			NumRecords--;
		}

		// Execute the statement
		rc = SQLExecute(hstmt);
		switch (rc) {
			case	SQL_SUCCESS:
				break;
			case	SQL_SUCCESS_WITH_INFO:
				wprintf(L"SQL_SUCCESS_WITH_INFO\n");
				displayError(SQL_HANDLE_STMT, hstmt);
				break;
			case	SQL_NEED_DATA:
				wprintf(L"SQL_NEED_DATA\n");
				break;
			case	SQL_STILL_EXECUTING:
				wprintf(L"SQL_STILL_EXECUTING\n");
				break;
			case	SQL_ERROR:
				wprintf(L"SQL_ERROR\n");
				displayError(SQL_HANDLE_STMT, hstmt);
				break;
			case	SQL_NO_DATA:
				wprintf(L"SQL_NO_DATA\n");
				break;
			case	SQL_INVALID_HANDLE:
				wprintf(L"SQL_INVALID_HANDLE\n");
				displayError(SQL_HANDLE_STMT, hstmt);
				break;
		}

		// Commit Transaction
		rc = SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_COMMIT);
		

		rc = SQLRowCount(hstmt, (SQLINTEGER *)&RowCount);
		NumRowsAffected = NumRowsAffected + RowCount;


	}

	// Obtain start time
	_ftime(&timeBuffer);
	endMSec = (unsigned long)(timeBuffer.time * 1000) + timeBuffer.millitm;

	//NumberRecords = (unsigned int)SelectCount(hstmt);
	wprintf(L"%17d	%10d	", NumRowsAffected, SizeOfArray);

	// Display time
	unsigned long totalMSec = endMSec - beginMSec;
	wprintf(L"%d msecs", totalMSec);

	// Set AUTOCOMMIT on
	rc = SQLSetConnectAttr (hdbc, SQL_ATTR_AUTOCOMMIT,(SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_IS_UINTEGER);

}



void Terminate(SQLHANDLE henv, SQLHANDLE hdbc, SQLHANDLE hstmt)
{
	SQLRETURN rc;

	rc = SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	if (rc != SQL_SUCCESS) {
		wprintf(L"SQLFreeHandle->SQL_HANDLE_STMT failed\n");
		displayError(SQL_HANDLE_STMT, hstmt);
		exit(255);
	}

	rc = SQLDisconnect(hdbc);
	if (rc != SQL_SUCCESS) {
		wprintf(L"SQLDisconnect failed\n");
		displayError(SQL_HANDLE_DBC, hdbc);
		exit(255);
	}

	rc = SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	if (rc != SQL_SUCCESS) {
		wprintf(L"SQLFreeHandle->SQL_HANDLE_DBC failed\n");
		displayError(SQL_HANDLE_DBC, hdbc);
		exit(255);
	}

	rc = SQLFreeHandle(SQL_HANDLE_ENV, henv);
	if (rc != SQL_SUCCESS) {
		wprintf(L"SQLFreeHandle->SQL_HANDLE_ENV failed\n");
		displayError(SQL_HANDLE_ENV, henv);
		exit(255);
	}
}

void PrintTableHeader()
{
	wprintf (L"Num Recs Updated 	Array Size	Time		\n");
	wprintf (L"-----------------	----------	------------\n");
}

void ClearTable(SQLHANDLE hstmt, wchar_t *TableName)
{

	wchar_t	DeleteStatement[50];

	RETCODE	rc;

	wcscpy(DeleteStatement, L"delete from ");
	wcscat(DeleteStatement, TableName);

	rc = SQLExecDirectW(hstmt, DeleteStatement, wcslen(DeleteStatement));
	if (rc != SQL_SUCCESS) {
		wprintf (L"...DELETE FROM %s statement failed\n", TableName);
		displayError(SQL_HANDLE_STMT, hstmt);
	}
}


void ArrayInsert(SQLHANDLE hdbc, SQLHANDLE hstmt, wchar_t *TableName, unsigned int SizeOfArray, unsigned int NumberRecords)
{

	SQLRETURN	rc;

	wchar_t	Statement[1000];
	int	EmpID = 10000;

	SQLCHAR	FirstNameArray[ARRAY_SIZE][DESC_LEN];
	SQLCHAR	LastNameArray[ARRAY_SIZE][DESC_LEN];
	int	EmpIDArray[ARRAY_SIZE];
	SQL_DATE_STRUCT		HireDateArray[ARRAY_SIZE];
	int		SalaryArray[ARRAY_SIZE];
	SQLCHAR	DeptArray[ARRAY_SIZE][DESC_LEN];
	int		ExemptArray[ARRAY_SIZE];


	SQLLEN	FirstNameIndArray[ARRAY_SIZE],
		LastNameIndArray[ARRAY_SIZE],
		EmpIDIndArray[ARRAY_SIZE],
		HireDateIndArray[ARRAY_SIZE],
		SalaryIndArray[ARRAY_SIZE],
		DeptIndArray[ARRAY_SIZE],
		ExemptIndArray[ARRAY_SIZE];

	SQLINTEGER	i,
		Loop,
		NumRecords;
	SQLUSMALLINT	ParamStatusArray[ARRAY_SIZE];
	SQLULEN	ParamsProcessed;

	wcscpy(Statement, L"insert into ");
	wcscat(Statement, TableName);
	wcscat(Statement, L" (FIRST_NAME, LAST_NAME, EMP_ID, HIRE_DATE, SALARY, DEPT, EXEMPT) ");
	wcscat(Statement, L"values (?, ?, ?, ?, ?, ?, ?)");

	// Set AUTOCOMMIT off
	rc = SQLSetConnectAttr(hdbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER);

	// Set the SQL_ATTR_PARAM_BIND_TYPE statement attrivute to use column-wise binding
	rc = SQLSetStmtAttr(hstmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0);

	// Specify the number of elements in each parameter array
	//fprintf(stderr, "Setting SQLSetStmtAttr->SQL_ATTR_PARAMSET_SIZE for %d row(s) in array...", ARRAY_SIZE);
	//rc = SQLSetStmtAttr (hstmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)ARRAY_SIZE, 0);
	//fprintf(stderr, "done!\n");

	// Specify an array in which to return the status of each set of parameters.
	rc = SQLSetStmtAttr(hstmt, SQL_ATTR_PARAM_STATUS_PTR, ParamStatusArray, 0);

	// Specify an SQLUINTEGER value in which to return the number of sets of parameters
	// processed.
	rc = SQLSetStmtAttr(hstmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &ParamsProcessed, 0);

	// Bind the parameters in column-wise fashion
	rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0,
		FirstNameArray, DESC_LEN, FirstNameIndArray);
	rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 30, 0,
		LastNameArray, DESC_LEN, LastNameIndArray);
	rc = SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 6, 0,
		EmpIDArray, sizeof(int), EmpIDIndArray);
	rc = SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_TYPE_DATE, SQL_TYPE_TIMESTAMP, 19, 0,
		HireDateArray, 19, HireDateIndArray);
	rc = SQLBindParameter(hstmt, 5, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 9, 0,
		SalaryArray, sizeof(int), SalaryIndArray);
	rc = SQLBindParameter(hstmt, 6, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 4, 0,
		DeptArray, DESC_LEN, DeptIndArray);
	rc = SQLBindParameter(hstmt, 7, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 3, 0,
		ExemptArray, sizeof(int), ExemptIndArray);

	rc = SQLPrepareW(hstmt, Statement, wcslen(Statement));
	if (rc != SQL_SUCCESS) {
		wprintf(L"SQLPREPARE failed!\n");
		displayError(SQL_HANDLE_STMT, hstmt);
		exit(255);
	}

	NumRecords = NumberRecords;
	i = 0;

	while (NumRecords > 0) {

		if (NumRecords < (SQLINTEGER)SizeOfArray) {
			Loop = NumRecords;
		}
		else {
			Loop = SizeOfArray;
		}

		// Specify the number of elements in each parameter array
		rc = SQLSetStmtAttr(hstmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)Loop, 0);

		// Set values for insert
		for (i = 0; i < Loop; i++) {
			strcpy((char *)FirstNameArray[i], "Gordon");
			FirstNameIndArray[i] = strlen(FirstNameArray[i]);

			strcpy((char *)LastNameArray[i], "Crenshaw");
			LastNameIndArray[i] = strlen(LastNameArray[i]);

			EmpID++;
			EmpIDArray[i] = EmpID;
			EmpIDIndArray[i] = 0;

			HireDateArray[i].year = 1995;
			HireDateArray[i].month = 9;
			HireDateArray[i].day = 06;
			HireDateIndArray[i] = 0;

			SalaryArray[i] = 1000;
			SalaryIndArray[i] = 0;

			strcpy((char *)DeptArray[i], "D101");
			DeptIndArray[i] = strlen(DeptArray[i]);

			ExemptArray[i] = 100;
			ExemptIndArray[i] = 0;

			NumRecords--;
		}

		// Execute the statement
		rc = SQLExecute(hstmt);
		switch (rc) {
			case	SQL_SUCCESS:
				break;
			case	SQL_SUCCESS_WITH_INFO:
				wprintf(L"SQL_SUCCESS_WITH_INFO\n");
				displayError(SQL_HANDLE_STMT, hstmt);
				break;
			case	SQL_NEED_DATA:
				wprintf(L"SQL_NEED_DATA\n");
				break;
			case	SQL_STILL_EXECUTING:
				wprintf(L"SQL_STILL_EXECUTING\n");
				break;
			case	SQL_ERROR:
				wprintf(L"SQL_ERROR\n");
				displayError(SQL_HANDLE_STMT, hstmt);
				break;
			case	SQL_NO_DATA:
				wprintf(L"SQL_NO_DATA\n");
				break;
			case	SQL_INVALID_HANDLE:
				wprintf(L"SQL_INVALID_HANDLE\n");
				displayError(SQL_HANDLE_STMT, hstmt);
				break;
		}

		// Commit Transaction
		rc = SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_COMMIT);


	}

	// Set AUTOCOMMIT on
	rc = SQLSetConnectAttr(hdbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_IS_UINTEGER);

}

unsigned int SelectCount(SQLHANDLE hstmt, wchar_t *TableName)
{
	wchar_t	SelCountStatement[50];
	SQLINTEGER		RecCount;
	SQLINTEGER		RecCountInd;

	RETCODE	rc;

	RecCount = 0;

	wcscpy(SelCountStatement, L"select count(*) from ");
	wcscat(SelCountStatement, TableName);

	rc = SQLExecDirectW(hstmt, SelCountStatement, wcslen(SelCountStatement));
	if (rc != SQL_SUCCESS) {
		wprintf(L"...select count(*) from %s statement failed\n", TableName);
		displayError(SQL_HANDLE_STMT, hstmt);
	}

	rc = SQLBindCol(hstmt, 1, SQL_C_ULONG, &RecCount, 0, &RecCountInd);
	if (rc != SQL_SUCCESS) {
		wprintf(L"...SQLBindCol failed\n");
		displayError(SQL_HANDLE_STMT, hstmt);
	}

	rc = SQLFetch(hstmt);

	rc = SQLFreeStmt(hstmt, SQL_CLOSE);

	return(RecCount);

}

void CreateTable(SQLHANDLE hstmt, wchar_t *TableName)
{

	RETCODE	rc;
	wchar_t	sqlStatement[300];

	wcscpy(sqlStatement, L"create table ");
	wcscat(sqlStatement, TableName);
	wcscat(sqlStatement, L" (FIRST_NAME CHAR(20), LAST_NAME CHAR(30), EMP_ID INT, ");
	wcscat(sqlStatement, L"HIRE_DATE DATE, SALARY DECIMAL(9,2), DEPT CHAR(4), EXEMPT INT)");

	rc = SQLExecDirectW(hstmt, sqlStatement, wcslen(sqlStatement));
	if (rc != SQL_SUCCESS) {
		displayError(SQL_HANDLE_STMT, hstmt);
	}

}

void DropTable(SQLHANDLE hstmt, wchar_t *TableName)
{

	RETCODE	rc;
	wchar_t	sqlStatement[50];

	wcscpy(sqlStatement, L"drop table ");
	wcscat(sqlStatement, TableName);

	rc = SQLExecDirectW(hstmt, sqlStatement, wcslen(sqlStatement));
	if (rc != SQL_SUCCESS) {
		displayError(SQL_HANDLE_STMT, hstmt);
	}

}
