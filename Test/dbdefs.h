#pragma once

#include <D3Types.h>

namespace D3
{
	struct MetaDictionaryDefinition : D3::MetaDatabaseDefinition
	{
		MetaDictionaryDefinition()
		{
			m_strDriver					= "SQL Server Native Client 10.0";
			m_strServer					= ".\\SQLExpress";
			m_strUserID					= "sa";
			m_strUserPWD				= "ElADNlIV";
			m_strName						= "SCHOOL_D3MDDB_6_07_0004";
			m_strAlias					= "D3MDDB";
			m_strDataFilePath		= "D:\\Data\\Databases\\SCHOOL\\SCHOOL_D3MDDB_6_07_0004.mdf";
			m_strLogFilePath		= "D:\\Data\\Databases\\SCHOOL\\SCHOOL_D3MDDB_6_07_0004.ldf";
			m_eTargetRDBMS			= SQLServer;
			m_iInitialSize			= 10;
			m_iMaxSize					= 2000;
			m_iGrowthSize				= 10;
			m_iVersionMajor			= 6;
			m_iVersionMinor			= 7;
			m_iVersionRevision	= 4;
			m_pInstanceClass		= Class::Find("ODBCDatabase");
			m_strTimeZone				= "NZST+12NZDT01:00:00,M9.5.0/02:00:00,M4.1.0/03:00:00";
		}
	};


	struct SchoolDatabaseDefinition : D3::MetaDatabaseDefinition
	{
		SchoolDatabaseDefinition()
		{
			m_strDriver					= "SQL Server Native Client 10.0";
			m_strServer					= "XE";
			m_strUserID					= "sa";
			m_strUserPWD				= "ElADNlIV";
			m_strName						= "SCHOOL_SCHOOLDB_6_07_0001";
			m_strAlias					= "SCHOOLDB";
			m_strDataFilePath		= "D:\\Data\\Databases\\SCHOOL\\SCHOOL_SCHOOLDB_6_07_0001.mdf";
			m_strLogFilePath		= "D:\\Data\\Databases\\SCHOOL\\SCHOOL_SCHOOLDB_6_07_0001.ldf";
			m_eTargetRDBMS			= SQLServer;
			m_iInitialSize			= 10;
			m_iMaxSize					= 2000;
			m_iGrowthSize				= 10;
			m_iVersionMajor			= 6;
			m_iVersionMinor			= 7;
			m_iVersionRevision	= 1;
			m_pInstanceClass		= Class::Find("ODBCDatabase");
			m_strTimeZone				= "NZST+12NZDT01:00:00,M9.5.0/02:00:00,M4.1.0/03:00:00";
		}
/*
		// ORACLE Version

		SchoolDatabaseDefinition()
		{
			m_strDriver					= "SQL Server Native Client 10.0";
			m_strServer					= "XE";
			m_strUserID					= "school";
			m_strUserPWD				= "V0YHPUtBFENF";
			m_strName						= "SCHOOL_SCHOOLDB_6_07_0001";
			m_strAlias					= "SCHOOLDB";
			m_strDataFilePath		= "D:\\Data\\Databases\\SCHOOL\\SCHOOL_SCHOOLDB_6_07_0001.mdf";
			m_strLogFilePath		= "D:\\Data\\Databases\\SCHOOL\\SCHOOL_SCHOOLDB_6_07_0001.ldf";
			m_eTargetRDBMS			= Oracle;
			m_iInitialSize			= 10;
			m_iMaxSize					= 2000;
			m_iGrowthSize				= 10;
			m_iVersionMajor			= 6;
			m_iVersionMinor			= 7;
			m_iVersionRevision	= 1;
			m_pInstanceClass		= Class::Find("OTLDatabase");
			m_strTimeZone				= "NZST+12NZDT01:00:00,M9.5.0/02:00:00,M4.1.0/03:00:00";
		}
*/
	};


	struct HelpDatabaseDefinition : D3::MetaDatabaseDefinition
	{
		HelpDatabaseDefinition()
		{
			m_strDriver					= "SQL Server Native Client 10.0";
			m_strServer					= ".\\SQLExpress";
			m_strUserID					= "sa";
			m_strUserPWD				= "ElADNlIV";
			m_strName						= "SCHOOL_D3HSDB_6_07_0002";
			m_strAlias					= "D3HSDB";
			m_strDataFilePath		= "D:\\Data\\Databases\\SCHOOL\\SCHOOL_D3HSDB_6_07_0002.mdf";
			m_strLogFilePath		= "D:\\Data\\Databases\\SCHOOL\\SCHOOL_D3HSDB_6_07_0002.ldf";
			m_eTargetRDBMS			= SQLServer;
			m_iInitialSize			= 10;
			m_iMaxSize					= 2000;
			m_iGrowthSize				= 10;
			m_iVersionMajor			= 6;
			m_iVersionMinor			= 7;
			m_iVersionRevision	= 2;
			m_pInstanceClass		= Class::Find("ODBCDatabase");
			m_strTimeZone				= "NZST+12NZDT01:00:00,M9.5.0/02:00:00,M4.1.0/03:00:00";
		}
	};
}