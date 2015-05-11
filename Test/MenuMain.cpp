#include "types.h"
#include "MenuMain.h"
#include "MenuMDLoaded.h"

bool s_regD3 = D3::MetaDatabase::RegisterClassFactories();

#include "dbdefs.h"


using namespace D3;
using namespace std;


// helpers

void ShowDatabaseSpecs(MetaDatabaseDefinition& defMD)
{
	char											szVersion[128];

	cout << "Driver.............: " << defMD.m_strDriver << endl;
	cout << "Server.............: " << defMD.m_strServer << endl;
	cout << "User ID............: " << defMD.m_strUserID << endl;
	cout << "Password...........: " << defMD.m_strUserPWD << endl;
	cout << "Name...............: " << defMD.m_strName << endl;
	cout << "Data File Path.....: " << defMD.m_strDataFilePath << endl;
	cout << "Data Logfile Path..: " << defMD.m_strLogFilePath << endl;
	cout << "Target RDBMS.......: " << (defMD.m_eTargetRDBMS == SQLServer ? "SQLServer" : "Oracle") << endl;
	cout << "Instance Class.....: " << (defMD.m_pInstanceClass ? defMD.m_pInstanceClass->Name() : "undefined") << endl;
	cout << "TimeZone Details...: " << defMD.m_strTimeZone << endl;

	_snprintf(szVersion, 128, "%i.%02i.%04i", defMD.m_iVersionMajor, defMD.m_iVersionMinor, defMD.m_iVersionRevision);

	cout << "Version............: " << szVersion << endl;
	cout << endl;
}



/*
		cout << "1  View D3 Class Hierarchy" << endl;
		cout << "2  Create Meta Dictionary Database" << endl;
		cout << "3  Create Meta Dictionary Source Code" << endl;
		cout << "4  Import Meta Dictionary" << endl;
		cout << "5  Import RBAC" << endl;
		cout << "6  Load Meta Dictionary" << endl;
*/


class MOViewClassHierarchy : public MenuOption
{
	public:
		MOViewClassHierarchy(Menu* pMenu) : MenuOption("View D3 Class Hierarchy", pMenu) {}

		virtual void			invoke()
		{
			Class::DumpHierarchy();
		}
};


class MOCreateMetaDictionaryDatabase : public MenuOption
{
	public:
		MOCreateMetaDictionaryDatabase(Menu* pMenu) : MenuOption("Create Meta Dictionary Database", pMenu) {}

		virtual void			invoke()
		{
			// User must confirm these
			cout << "WARNING: This action will destroy the existing meta dictionary database!\n\nType YES to confirm: ";

			if (boost::to_upper_copy(getStringInput()) != "YES")
			{
				cout << "No action taken\n";
			}
			else
			{
				MetaDatabasePtr	 pMD = MetaDatabase::GetMetaDictionary();

				if (!pMD)
				{
					cout << "Create Meta Dictionary database failed because no MetaDatabase object exists that defines the Meta Dictionary - check if it has been initialised!\n";
				}
				else
				{
					if (!pMD->CreatePhysicalDatabase())
					{
						cout << pMD->GetName() << ": Error occurred creating physical database!" << endl;
					}

					cout << pMD->GetName() << ": Successfully created physical database!" << endl;
				}
			}
		}
};


class MOCreateMetaDictionarySourceCode : public MenuOption
{
	public:
		MOCreateMetaDictionarySourceCode(Menu* pMenu) : MenuOption("Create Meta Dictionary Source Code", pMenu) {}

		virtual void			invoke()
		{
			MetaDatabasePtr	 pMD = MetaDatabase::GetMetaDictionary();

			if (!pMD)
			{
				cout << "Create C++ files failed because D3::MetaDictionary does not exist - check if it has been initialised!" << std::endl;
			}
			else
			{
				if (pMD->GenerateSourceCode())
					std::cout << pMD->GetName() << ": Successfully generated C++ files!" << std::endl;
				else
					std::cout << pMD->GetName() << ": Error occurred generating C++ files!" << std::endl;
			}
		}
};




class MOImportMetaDictionary : public MenuOption
{
	public:
		MOImportMetaDictionary(Menu* pMenu) : MenuOption("Import Meta Dictionary", pMenu) {}

		virtual void			invoke()
		{
			MetaDatabasePtr				pMD = MetaDatabase::GetMetaDictionary();
			MetaEntityPtrList			listME;
			D3MDDB_Tables					aryEntityID[] = { D3MDDB_D3MetaDatabase,
																							D3MDDB_D3MetaEntity,
																							D3MDDB_D3MetaColumn,
																							D3MDDB_D3MetaColumnChoice,
																							D3MDDB_D3MetaKey,
																							D3MDDB_D3MetaKeyColumn,
																							D3MDDB_D3MetaRelation
																						};
			DatabaseWorkspace			dbWS;
			DatabasePtr						pDB;
			string								strComment;

			if (!pMD)
			{
				cout << "Create Meta Dictionary database failed because no MetaDatabase object exists that defines the Meta Dictionary - check if it has been initialised!" << endl;
			}
			else
			{
				// Set MetaEntity list content
				for (unsigned int idx = 0; idx < sizeof(aryEntityID)/sizeof(D3MDDB_Tables); idx++)
					listME.push_back(pMD->GetMetaEntity(aryEntityID[idx]));

				// Get the instance we need to work with
				pDB = dbWS.GetDatabase(pMD);

				if (pDB->ImportFromXML(APP_NAME, APP_NAME "MD.xml", &listME) > 0)
					cout << "Importing Meta Dictionary succeeded!" << endl;
				else
					cout << "Importing Meta Dictionary failed!" << endl;
			}
		}
};




class MOImportRBAC : public MenuOption
{
	public:
		MOImportRBAC(Menu* pMenu) : MenuOption("Import RBAC", pMenu) {}

		virtual void			invoke()
		{
			cout << "Not yet implemented\n";
		}
};




class MOLoadMetaDictionary : public MenuOption
{
	public:
		MOLoadMetaDictionary(Menu* pMenu) : MenuOption("Load Meta Dictionary", pMenu) {}

		virtual void			invoke()
		{
			MetaDatabasePtr							pMD = MetaDatabase::GetMetaDictionary();
			MetaDatabaseDefinitionList	listMDDefs;
			SchoolDatabaseDefinition		defSCHOOLDB;
			HelpDatabaseDefinition			defD3HSDB;

			if (!pMD)
			{
				cout << "Create Meta Dictionary database failed because no MetaDatabase object exists that defines the Meta Dictionary - check if it has been initialised!" << endl;
			}
			else
			{
				cout << "SCHOOL Database Details:" << endl;
				cout << "------------------------" << endl;
				ShowDatabaseSpecs(defSCHOOLDB);
				cout << endl;

				Settings::Singleton().RegisterDBVersion(defSCHOOLDB.m_strAlias, Settings::DBVersion(defSCHOOLDB.m_iVersionMajor, defSCHOOLDB.m_iVersionMinor, defSCHOOLDB.m_iVersionRevision));
				listMDDefs.push_back(defSCHOOLDB);

				cout << "Help Database Details:" << endl;
				cout << "----------------------" << endl;
				ShowDatabaseSpecs(defD3HSDB);
				cout << endl;

				Settings::Singleton().RegisterDBVersion(defD3HSDB.m_strAlias, Settings::DBVersion(defD3HSDB.m_iVersionMajor, defD3HSDB.m_iVersionMinor, defD3HSDB.m_iVersionRevision));
				listMDDefs.push_back(defD3HSDB);

				MetaDatabase::LoadMetaDictionary(listMDDefs, false);

				MenuMDLoaded		menuMDLoaded;
				menuMDLoaded.run();

				m_pMenu->shouldExit(true);
			}
		}
};




MenuMain::MenuMain()
{
	addOption(MenuOptionPtr(new MOViewClassHierarchy(this)));
	addOption(MenuOptionPtr(new MOCreateMetaDictionaryDatabase(this)));
	addOption(MenuOptionPtr(new MOCreateMetaDictionarySourceCode(this)));
	addOption(MenuOptionPtr(new MOImportMetaDictionary(this)));
	addOption(MenuOptionPtr(new MOImportRBAC(this)));
	addOption(MenuOptionPtr(new MOLoadMetaDictionary(this)));
};




void MenuMain::run()
{
	MetaDictionaryDefinition	defMD;

	try
	{
		cout << "Meta Dictionary Details:" << endl;
		cout << "------------------------" << endl;
		ShowDatabaseSpecs(defMD);
		Settings::Singleton().RegisterDBVersion(defMD.m_strAlias, Settings::DBVersion(defMD.m_iVersionMajor, defMD.m_iVersionMinor, defMD.m_iVersionRevision));

		// We must have a valid instance class
		//
		if (!defMD.m_pInstanceClass)
		{
			ReportError("Initialise meta dictionary failed. \"InstanceClass\" is undefined.");
		}
		else
		{
			// Now let's initialise the MetaDictionary
			MetaDatabase::Initialise(defMD);
			Menu::run();
		}
	}
	catch(...)
	{
		string		strError = D3::ShortGenericExceptionHandler();

		D3::ReportError(strError.c_str());
		cout << strError << endl;
	}
}