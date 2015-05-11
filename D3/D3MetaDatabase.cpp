// MODULE: D3MetaDatabase source
#include "D3Types.h"

#include "D3MetaDatabase.h"
#include <sstream>
#include "Session.h"

namespace D3
{
	// Helpers which return the stored procedure names (and cache these on first use)

	const std::string&	MakeMetaDictionaryResidentProcName()
	{
		static std::string S_strMakeMetaDictionaryResident(std::string("makeMetaDictionaryResident_V") + Settings::Singleton().GetDBVersion("D3MDDB").toString());
		return S_strMakeMetaDictionaryResident;
	}




	//===========================================================================
	//
	// MetaDatabaseCloningController implementation
	//

	MetaDatabaseCloningController::MetaDatabaseCloningController(EntityPtr pOrigRoot, SessionPtr pSession, DatabaseWorkspacePtr pDBWS, Json::Value* pJSONChanges)
		: CloningController(pOrigRoot, pSession, pDBWS, pJSONChanges),
			m_bAllowCloning(false)
	{
		if (!pSession || pSession->GetRoleUser()->GetRole() == D3::Role::M_sysadmin)
			m_bAllowCloning = true;
	}



	void MetaDatabaseCloningController::On_BeforeCloning()
	{
		if (m_bAllowCloning)
		{
			MetaDatabaseDefinitionList		listMDDefs;
			MetaDatabaseDefinition				md;
			D3MetaDatabasePtr							pD3MD = (D3MetaDatabasePtr) m_pOrigRoot;

			assert(pD3MD->GetDatabase() == m_pDBWS->GetDatabase(pD3MD->GetDatabase()->GetMetaDatabase()));

			// Throw if the user is not the sysadmin
			if (m_pSession && m_pSession->GetRoleUser()->GetUser() != User::M_sysadmin)
				throw std::string("Insufficient privileges to clone Meta Database definitions.");

			// Create the MetaDatabaseDefinitionList we need to load this
			md.m_strAlias = pD3MD->GetAlias();
			md.m_iVersionMajor = pD3MD->GetVersionMajor();
			md.m_iVersionMinor = pD3MD->GetVersionMinor();
			md.m_iVersionRevision = pD3MD->GetVersionRevision();

			listMDDefs.push_back(md);

			// Make this resident
			pD3MD->MakeD3MetaDictionariesResident(m_pOrigRoot->GetDatabase(), listMDDefs);
		}
	}




	//===========================================================================
	//
	// D3MetaDatabase implementation
	//

	D3_CLASS_IMPL(D3MetaDatabase, D3MetaDatabaseBase);




	// Return a new instance populated with the details from the MetaDatabase passed in
	D3MetaDatabase::D3MetaDatabase(DatabasePtr pDB, MetaDatabasePtr pMD)
	{
		MarkConstructing();
		m_pMetaEntity = MetaDatabase::GetMetaDictionary()->GetMetaEntity(D3MDDB_D3MetaDatabase);
		m_pDatabase = pDB;
		On_PostCreate();
		UnMarkConstructing();

		// Set the details from the meta type
		SetAlias(pMD->GetAlias());
		SetProsaName(pMD->GetProsaName());
		SetDescription(pMD->GetDescription());
		SetVersionMajor(pMD->GetVersionMajor());
		SetVersionMinor(pMD->GetVersionMinor());
		SetVersionRevision(pMD->GetVersionRevision());
		SetInstanceInterface(pMD->GetInstanceInterface());
		SetFlags(pMD->GetFlags().RawValue());
		SetAccessRights(pMD->GetPermissions().RawValue());
	}



	// Return the related entity with the given name
	D3MetaEntityPtr D3MetaDatabase::GetD3MetaEntity(const std::string & strEntityName)
	{
		D3MetaEntityPtr														pMC;
		D3MetaDatabase::D3MetaEntities::iterator	itr;


		for ( itr =  GetD3MetaEntities()->begin();
					itr != GetD3MetaEntities()->end();
					itr++)
		{
			pMC = *itr;

			if (pMC->GetName() == strEntityName)
				return pMC;
		}

		throw Exception(__FILE__, __LINE__, Exception_error, "D3MetaDatabase::GetMetaEntity(): D3MetaDatabase %s has no entity %s.", GetAlias().c_str(), strEntityName.c_str());

		return NULL;
	}



	// Load and return the D3MetaDatabase instance matching the passed in MetaDatabaseDefinition resident
	/* static */
	D3MetaDatabasePtr D3MetaDatabase::LoadD3MetaDatabase(DatabasePtr pDatabase, const std::string & strAlias, int iVersionMajor, int iVersionMinor, int iVersionRevision)
	{
		MetaDatabasePtr				pMDDict = MetaDatabase::GetMetaDictionary();
		MetaKeyPtr						pMK;
		D3MetaDatabasePtr			pD3MDB = NULL;


		assert(pDatabase);
		assert(pDatabase->GetMetaDatabase() == pMDDict);

		pMK = pMDDict->GetMetaEntity("D3MetaDatabase")->GetMetaKey("SK_D3MetaDatabase");

		if (pMK)
		{
			// Make a temporary key
			TemporaryKey	tmpKey(*pMK);

			// Initialise the key
			tmpKey.GetColumn("Alias")->SetValue(strAlias);
			tmpKey.GetColumn("VersionMajor")->SetValue(iVersionMajor);
			tmpKey.GetColumn("VersionMinor")->SetValue(iVersionMinor);
			tmpKey.GetColumn("VersionRevision")->SetValue(iVersionRevision);

			// Load the object
			pD3MDB = (D3MetaDatabasePtr) pMK->LoadObject(&tmpKey, pDatabase);
		}

		return pD3MDB;
	}



	/* static */
	D3MetaDatabasePtr D3MetaDatabase::FindUniqueD3MetaDatabase(DatabasePtr pDatabase, const std::string & strAlias)
	{
		MetaKeyPtr										pMK;
		InstanceKeyPtrSet::iterator		itrKey;
		KeyPtr												pKey;
		D3MetaDatabasePtr							pD3MDB = NULL;


		assert(pDatabase);
		assert(pDatabase->GetMetaDatabase() == MetaDatabase::GetMetaDictionary());

		pMK = pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaDatabase)->GetMetaKey(D3MDDB_D3MetaDatabase_SK_D3MetaDatabase);

		if (!pMK)
			throw Exception(__FILE__, __LINE__, Exception_error, "D3MetaDatabase::FindUniqueD3MetaDatabase: Failed location secondary key [D3MetaDatabase][SK_D3MetaDatabase]");

		// Iterate over all instance keys for this database
		for ( itrKey =  pMK->GetInstanceKeySet(pDatabase)->begin();
					itrKey != pMK->GetInstanceKeySet(pDatabase)->end();
					itrKey++)
		{
			pKey = *itrKey;

			if (pKey->GetColumn(D3MDDB_D3MetaDatabase_Alias)->GetString() == strAlias)
			{
				if (pD3MDB)
					throw Exception(__FILE__, __LINE__, Exception_error, "D3MetaDatabase::FindUniqueD3MetaDatabase: Found multiple instances of D3MetaDatabase with alias %s resident.", strAlias.c_str());

				pD3MDB = (D3MetaDatabasePtr) pKey->GetEntity();
			}
		}

		return pD3MDB;
	}



	D3MetaEntityPtr D3MetaDatabase::FindD3MetaEntity(const std::string & strName)
	{
		D3MetaEntities::iterator			itrD3ME;
		D3MetaEntityPtr								pD3ME;

		// Iterate over all instance keys for this database
		for ( itrD3ME =  this->GetD3MetaEntities()->begin();
					itrD3ME != this->GetD3MetaEntities()->end();
					itrD3ME++)
		{
			pD3ME = *itrD3ME;

			if (pD3ME->GetColumn(D3MDDB_D3MetaEntity_Name)->GetString() == strName)
				return pD3ME;
		}

		return NULL;
	}



	// Makes the D3MetaDatabase instance matching the passed in MetaDatabaseDefinition resident
	// (i.e. loads the correct D3MetaDatabase and all it's associated details)
	/* static */
	void D3MetaDatabase::MakeD3MetaDictionariesResident(DatabasePtr pDatabase, MetaDatabaseDefinitionList & listMDDefs)
	{
		switch (pDatabase->GetMetaDatabase()->GetTargetRDBMS())
		{
			case SQLServer:
				MakeD3MetaDictionariesResidentODBC(pDatabase, listMDDefs);
				break;
			case Oracle:
				MakeD3MetaDictionariesResidentConventional(pDatabase, listMDDefs);
				break;
			default:
	 			throw Exception(__FILE__, __LINE__, Exception_error, "D3MetaDatabase::MakeD3MetaDictionariesResident(): Unrecognised traget database");
		}
	}



	// Makes the D3MetaDatabase instances matching the passed in MetaDatabaseDefinitions resident
	// (i.e. loads the correct D3MetaDatabase objects and all their associated details)
	/* static */
	void D3MetaDatabase::MakeD3MetaDictionariesResidentConventional(DatabasePtr pDatabase, MetaDatabaseDefinitionList & listMDDefs)
	{
		MetaDatabasePtr									pMDDict = MetaDatabase::GetMetaDictionary();
		D3MetaDatabasePtr								pD3MDB = NULL;
		MetaDatabaseDefinitionListItr		itrMDDefs;
		std::string											strMDIDs;


		//*****************************************************************************
		//!!!! WARNING: WE NEED TO MAKE SURE THIS METHOD DOES THE RIGHT THING:
		//              - We need to load RBAC objects
		//              - We need to deal correctly with cross database relations
		std::cout << D3Date().AsString() << " - Loading meta dictionary data [WARNING: D3MetaDatabase::MakeD3MetaDictionariesResidentConventional NEEDS WORK!!!]...\n";
		//*****************************************************************************

		// A simple class internal structure used to make D3MetaDatabase resident
		struct MELoader
		{
			D3MDDB_Tables uID;
			std::string		strWHERE;
		};

		MELoader dependents[] = {
															{	D3MDDB_D3MetaEntity,				"MetaDatabaseID IN (%i) ORDER BY ID"},
															{	D3MDDB_D3MetaColumn,				"MetaEntityID IN (SELECT ID FROM D3MetaEntity WHERE MetaDatabaseID IN (%i)) ORDER BY ID"},
															{	D3MDDB_D3MetaColumnChoice,	"MetaColumnID IN (SELECT ID FROM D3MetaColumn WHERE MetaEntityID IN (SELECT ID FROM D3MetaEntity WHERE MetaDatabaseID IN (%i))) ORDER BY MetaColumnID, SequenceNo"},
															{	D3MDDB_D3MetaKey,						"MetaEntityID IN (SELECT ID FROM D3MetaEntity WHERE MetaDatabaseID IN (%i)) ORDER BY ID"},
															{	D3MDDB_D3MetaKeyColumn,			"MetaKeyID IN (SELECT ID FROM D3MetaKey WHERE MetaEntityID IN (SELECT ID FROM D3MetaEntity WHERE MetaDatabaseID IN (%i))) ORDER BY MetaKeyID, SequenceNo"},
															{	D3MDDB_D3MetaRelation,			"ParentKeyID IN (SELECT ID FROM D3MetaKey WHERE MetaEntityID IN (SELECT ID FROM D3MetaEntity WHERE MetaDatabaseID IN (%i))) OR ChildKeyID IN (SELECT ID FROM D3MetaKey WHERE MetaEntityID IN (SELECT ID FROM D3MetaEntity WHERE MetaDatabaseID IN (%i))) ORDER BY ID"},
															{	D3MDDB_D3Role,							"1=1"},
															{	D3MDDB_D3User,							"1=1"},
															{	D3MDDB_D3HistoricPassword,	"1=1"},
															{	D3MDDB_D3RoleUser,					"1=1"},
															{	D3MDDB_D3DatabasePermission,"MetaDatabaseID IN (%i)"},
															{	D3MDDB_D3EntityPermission,	"MetaEntityID IN (SELECT ID FROM D3MetaEntity WHERE MetaDatabaseID IN (%i))"},
															{	D3MDDB_D3ColumnPermission,	"MetaColumnID IN (SELECT ID FROM D3MetaColumn WHERE MetaEntityID IN (SELECT ID FROM D3MetaEntity WHERE MetaDatabaseID IN (%i)))"},
															{	D3MDDB_D3RowLevelPermission,"MetaEntityID IN (SELECT ID FROM D3MetaEntity WHERE MetaDatabaseID IN (%i))"}
														};


		assert(pDatabase);
		assert(pDatabase->GetMetaDatabase() == pMDDict);

		// For each MetaDatabaseDefinition, add the ID to the list of ID's of D3MetaDictionary records to load
		for ( itrMDDefs =  listMDDefs.begin();
					itrMDDefs != listMDDefs.end();
					itrMDDefs++)
		{
			// Let's load all the data in the correct sequence
			//
			pD3MDB = LoadD3MetaDatabase(pDatabase, (*itrMDDefs).m_strAlias, (*itrMDDefs).m_iVersionMajor, (*itrMDDefs).m_iVersionMinor, (*itrMDDefs).m_iVersionRevision);

			if (!pD3MDB)
	 			throw Exception(__FILE__, __LINE__, Exception_error, "D3MetaDatabase::MakeD3MetaDatabaseResident(): D3MetaDatabase %s %i.%02i.%04i does not exist", (*itrMDDefs).m_strAlias.c_str(), (*itrMDDefs).m_iVersionMajor, (*itrMDDefs).m_iVersionMinor, (*itrMDDefs).m_iVersionRevision);

      if (!strMDIDs.empty())
        strMDIDs += ", ";

      strMDIDs += pD3MDB->Column(D3MetaDatabase_ID)->AsString();
		}

    // Load all related objects
    for (unsigned int i = 0; i < sizeof(dependents)/sizeof(MELoader); i++)
    {
      std::string				strSQL;
      MetaEntityPtr			pME;
      EntityPtrListPtr	pEL;

      pME = pMDDict->GetMetaEntity(dependents[i].uID);

      strSQL  = "SELECT ";
      strSQL += pME->AsSQLSelectList();
      strSQL += " FROM ";
      strSQL += pME->GetName();
      strSQL += " WHERE ";
      strSQL += ReplaceAll(dependents[i].strWHERE, "%i", strMDIDs);

      std::cout << strSQL << std::endl;
      pEL = pDatabase->LoadObjects(pME, strSQL);
      delete pEL;
    }

		std::cout << D3Date().AsString() << " - Done!\n";
	}



	// Makes the D3MetaDatabase instances matching the passed in MetaDatabaseDefinitions resident
	// (i.e. loads the correct D3MetaDatabase objects and all their associated details)
	/* static */
	void D3MetaDatabase::MakeD3MetaDictionariesResidentODBC(DatabasePtr pDatabase, MetaDatabaseDefinitionList & listMDDefs)
	{
		MetaEntityPtrList								listME;
		MetaDatabaseDefinitionListItr		itrMDDefs;
		std::string											strDictionaries;
		char														szBuff[512];


		// We need a list of dictionaries
		for ( itrMDDefs =  listMDDefs.begin();
					itrMDDefs != listMDDefs.end();
					itrMDDefs++)
		{
			snprintf(szBuff,512,"%s;%i;%i;%i", (*itrMDDefs).m_strAlias.c_str(), (*itrMDDefs).m_iVersionMajor, (*itrMDDefs).m_iVersionMinor, (*itrMDDefs).m_iVersionRevision);

			if (!strDictionaries.empty())
				strDictionaries += '|';
			else
				strDictionaries += '\'';

			strDictionaries += szBuff;
		}

		strDictionaries += '\'';

		// We want to load these objects in this order
		listME.push_back(pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaDatabase));
		listME.push_back(pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaEntity));
		listME.push_back(pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaColumn));
		listME.push_back(pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaColumnChoice));
		listME.push_back(pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaKey));
		listME.push_back(pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaKeyColumn));
		listME.push_back(pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaRelation));
		listME.push_back(pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3Role));
		listME.push_back(pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3User));
		listME.push_back(pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3HistoricPassword));
		listME.push_back(pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3RoleUser));
		listME.push_back(pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3DatabasePermission));
		listME.push_back(pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3EntityPermission));
		listME.push_back(pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3ColumnPermission));
		listME.push_back(pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3RowLevelPermission));

		pDatabase->LoadObjectsThroughStoredProcedure(listME, D3MetaDatabase::CreateMakeDictionaryResidentStoredProcedure, MakeMetaDictionaryResidentProcName(), strDictionaries, false, false);
	}



	// This module creates the stored procedure makeMetaDictionaryResident
	/*
		 Stored Procedure makeMetaDictionaryResident
		 -------------------------------------------

		 What we pass in is a semicolon delimited list of meta database identifiers in the form

				 alias;major;minor;revision

		 You can append multiple such strings and separate them using the bar | delimiter, e.g.:

				 APAL3DB;6;4;3|RTCISDB;6;0;1

		 The stored procedure returns the following result sets in exactly this order:

				 D3MetaDatabase
				 D3MetaEntity
				 D3MetaColumn
				 D3MetaColumnChoice
				 D3MetaKey
				 D3MetaKeyColumn
				 D3MetaRelation
				 D3Role
				 D3User
				 D3HistoricPassword
				 D3RoleUser
				 D3DatabasePermission
				 D3EntityPermission
				 D3ColumnPermission
				 D3RowLevelPermission

		 Notes: The resultsets may be empty if the passed in string doesn't match existing records in D3MetaDatabase.
		 The result sets are populated with all the data related with any of the D3MetaDatabase identified by the
		 input parameter. It is possible that data related to other D3MetaDatabase objects is retrned. This is the case
		 if there are cross database relations and you do not specify all databases involved in cross relations.
	*/
	void D3MetaDatabase::CreateMakeDictionaryResidentStoredProcedure(DatabasePtr pDB)
	{
		std::ostringstream							osql;

		osql << "/* \n";
		osql << "   Stored Procedure " << MakeMetaDictionaryResidentProcName() << std::endl;
		osql << "   -----------------" << std::string(MakeMetaDictionaryResidentProcName().length(), '-') << std::endl;
		osql << std::endl;
		osql << "   What we pass in is a semicolon delimited list of meta database identifiers in the form\n";
		osql << std::endl;
		osql << "       alias;major;minor;revision\n";
		osql << std::endl;
		osql << "   You can append multiple such strings and separate them using the bar | delimiter, e.g.:\n";
		osql << std::endl;
		osql << "       APAL3DB;6;4;3|RTCISDB;6;0;1\n";
		osql << std::endl;
		osql << "   The stored procedure returns the following result sets in exactly this order:\n";
		osql << std::endl;
		osql << "       D3MetaDatabase\n";
		osql << "       D3MetaEntity\n";
		osql << "       D3MetaColumn\n";
		osql << "       D3MetaColumnChoice\n";
		osql << "       D3MetaKey\n";
		osql << "       D3MetaKeyColumn\n";
		osql << "       D3MetaRelation\n";
		osql << "       D3Role\n";
		osql << "       D3User\n";
		osql << "       D3HistoricPassword\n";
		osql << "       D3RoleUser\n";
		osql << "       D3DatabasePermission\n";
		osql << "       D3EntityPermission\n";
		osql << "       D3ColumnPermission\n";
		osql << "       D3RowLevelPermission\n";
		osql << std::endl;
		osql << "   Notes: The resultsets may be empty if the passed in string doesn't match existing records in D3MetaDatabase.\n";
		osql << "   The result sets are populated with all the data related with any of the D3MetaDatabase identified by the\n";
		osql << "   input parameter. It is possible that data related to other D3MetaDatabase objects is retrned. This is the case\n";
		osql << "   if there are cross database relations and you do not specify all databases involved in cross relations.\n";
		osql << "*/\n";
		osql << "    CREATE Procedure " << MakeMetaDictionaryResidentProcName() << "(\n";
		osql << "      @dictionaries varchar(512)\n";
		osql << "    )\n";
		osql << "    AS\n";
		osql << "    SET NOCOUNT ON\n";
		osql << std::endl;
		osql << "    DECLARE @tmpDictionaries  TABLE ( Alias varchar(64),\n";
		osql << "                                      VersionMajor smallint,\n";
		osql << "                                      VersionMinor smallint,\n";
		osql << "                                      VersionRevision smallint )\n";
		osql << "                                      \n";
		osql << "    DECLARE @tmpMetaDatabase  TABLE ( ID int PRIMARY KEY )\n";
		osql << "    DECLARE @tmpMetaEntity    TABLE ( ID int PRIMARY KEY )\n";
		osql << "    DECLARE @tmpMetaColumn    TABLE ( ID int PRIMARY KEY )\n";
		osql << "    DECLARE @tmpMetaKey       TABLE ( ID int PRIMARY KEY )\n";
		osql << "    DECLARE @tmpMetaRelation  TABLE ( ID int PRIMARY KEY )\n";
		osql << "    DECLARE @tmpCDBMetaKey    TABLE ( ID int PRIMARY KEY )\n";
		osql << "    DECLARE @tmpCDBMetaEntity TABLE ( ID int PRIMARY KEY )\n";
		osql << std::endl;
		osql << "    /* \n";
		osql << "      Parse the meta dictionaries input parameter\n";
		osql << std::endl;
		osql << "      The result of this process is that the @tmpDictionaries table is populated with data passed in\n";
		osql << "    */\n";
		osql << std::endl;
		osql << "    DECLARE @Alias varchar(64)\n";
		osql << "    DECLARE @VersionMajor varchar(4)\n";
		osql << "    DECLARE @VersionMinor varchar(4)\n";
		osql << "    DECLARE @VersionRevision varchar(4)\n";
		osql << std::endl;
		osql << "    DECLARE @metaDictionary varchar(124)\n";
		osql << std::endl;
		osql << "    /* The parser state\n";
		osql << "        0 = expect Alias\n";
		osql << "        1 = expect VersionMajor\n";
		osql << "        2 = expect VersionMinor\n";
		osql << "        3 = expect VersionRevision\n";
		osql << "    */\n";
		osql << "    DECLARE @state smallint\n";
		osql << std::endl;
		osql << "    DECLARE @pos1 int, @len1 int\n";
		osql << "    DECLARE @pos2 int, @len2 int\n";
		osql << "    DECLARE @sql nvarchar(256)\n";
		osql << std::endl;
		osql << "    WHILE LEN(@dictionaries) > 0 BEGIN\n";
		osql << "      SET @pos1 = CHARINDEX('|', @dictionaries)\n";
		osql << "      IF @pos1 < 0 SET @pos1 = 0\n";
		osql << "      SET @len1 = LEN(@dictionaries) - @pos1 - 1\n";
		osql << "      IF @len1 < 0 SET @len1 = 0\n";
		osql << std::endl;
		osql << "      IF @pos1 > 124 BEGIN\n";
		osql << "        RAISERROR ('Maximum length is 124 per bar | delimited meta dictionary in input string', 16, 1) WITH SETERROR\n";
		osql << "        RETURN\n";
		osql << "      END\n";
		osql << std::endl;
		osql << "      IF @pos1 > 0 BEGIN\n";
		osql << "        SET @metaDictionary = SUBSTRING(@dictionaries, 1, @pos1 - 1)\n";
		osql << "        SET @dictionaries = SUBSTRING(@dictionaries, @pos1 + 1, LEN(@dictionaries) - @pos1)\n";
		osql << "      END ELSE BEGIN\n";
		osql << "        SET @metaDictionary = @dictionaries\n";
		osql << "        SET @dictionaries = ''\n";
		osql << "      END\n";
		osql << std::endl;
		osql << "      SET @state = 0\n";
		osql << std::endl;
		osql << "      WHILE LEN(@metaDictionary) > 0 BEGIN\n";
		osql << "        SET @pos2 = CHARINDEX(';', @metaDictionary)\n";
		osql << "        IF @pos2 < 0 SET @pos2 = 0\n";
		osql << "        SET @len2 = LEN(@metaDictionary) - @pos2 - 1\n";
		osql << "        IF @len2 < 0 SET @len2 = 0\n";
		osql << std::endl;
		osql << "        IF @state=0 BEGIN\n";
		osql << "          IF @pos2 = 0  BEGIN\n";
		osql << "            RAISERROR ('Incomplete metadictionary definition, expected Alias', 16, 1) WITH SETERROR\n";
		osql << "            RETURN\n";
		osql << "          END\n";
		osql << "          IF @pos2 > 124 BEGIN\n";
		osql << "            RAISERROR ('Maximum length for Alias is 64', 16, 1) WITH SETERROR\n";
		osql << "            RETURN\n";
		osql << "          END\n";
		osql << "          SET @Alias = SUBSTRING(@metaDictionary, 1, @pos2 - 1)\n";
		osql << "          SET @state = @state + 1\n";
		osql << "        END ELSE BEGIN\n";
		osql << "          IF @state=1 BEGIN\n";
		osql << "            IF @pos2 = 0 BEGIN\n";
		osql << "              RAISERROR ('Incomplete metadictionary definition, expected VersionMajor', 16, 1) WITH SETERROR\n";
		osql << "              RETURN\n";
		osql << "            END\n";
		osql << "            SET @VersionMajor = SUBSTRING(@metaDictionary, 1, @pos2 - 1)\n";
		osql << "            SET @state = @state + 1\n";
		osql << "          END ELSE BEGIN\n";
		osql << "            IF @state=2 BEGIN\n";
		osql << "              IF @pos2 = 0 BEGIN\n";
		osql << "                RAISERROR ('Incomplete metadictionary definition, expected VersionMinor', 16, 1) WITH SETERROR\n";
		osql << "                RETURN\n";
		osql << "              END\n";
		osql << "              SET @VersionMinor = SUBSTRING(@metaDictionary, 1, @pos2 - 1)\n";
		osql << "              SET @state = @state + 1\n";
		osql << "            END ELSE BEGIN\n";
		osql << "              IF @state=3 BEGIN\n";
		osql << "                IF @pos2 = 0 BEGIN\n";
		osql << "                  SET @VersionRevision = @metaDictionary\n";
		osql << "                END ELSE BEGIN\n";
		osql << "                  SET @VersionRevision = SUBSTRING(@metaDictionary, 1, @pos2 - 1)\n";
		osql << "                END\n";
		osql << std::endl;
		osql << "                INSERT INTO @tmpDictionaries (Alias,VersionMajor,VersionMinor,VersionRevision) VALUES (@Alias,@VersionMajor,@VersionMinor,@VersionRevision)\n";
		osql << std::endl;
		osql << "                SET @state = 0\n";
		osql << "              END ELSE BEGIN\n";
		osql << "                RAISERROR ('Internal error', 16, 1) WITH SETERROR\n";
		osql << "                RETURN\n";
		osql << "              END\n";
		osql << "            END\n";
		osql << "          END\n";
		osql << "        END\n";
		osql << std::endl;
		osql << "        IF @pos2 > 0 BEGIN\n";
		osql << "          SET @metaDictionary = SUBSTRING(@metaDictionary, @pos2 + 1, LEN(@metaDictionary) - @pos2)\n";
		osql << "        END ELSE BEGIN\n";
		osql << "          SET @metaDictionary = ''\n";
		osql << "        END\n";
		osql << "      END\n";
		osql << "    END\n";
		osql << std::endl;
		osql << "    /* Make sure that all passed in dictionaries actually exist */\n";
		osql << "    IF (SELECT COUNT(*) FROM @tmpDictionaries t WHERE NOT EXISTS(SELECT ID FROM D3MetaDatabase md WHERE md.Alias=t.Alias AND md.VersionMajor=t.VersionMajor AND md.VersionMinor=t.VersionMinor AND md.VersionRevision=t.VersionRevision)) > 0 BEGIN\n";
		osql << "      RAISERROR ('Not all dictionaries requested exist', 16, 1) WITH SETERROR\n";
		osql << "      RETURN\n";
		osql << "    END\n";
		osql << std::endl;
		osql << "    /* remember the D3MetaDatabase records */\n";
		osql << "    INSERT INTO @tmpMetaDatabase    SELECT ID FROM D3MetaDatabase md  WHERE EXISTS(SELECT *  FROM @tmpDictionaries t  WHERE md.Alias=t.Alias AND md.VersionMajor=t.VersionMajor AND md.VersionMinor=t.VersionMinor AND md.VersionRevision=t.VersionRevision)\n";
		osql << "    /* remember all related D3MetaEntity records */\n";
		osql << "    INSERT INTO @tmpMetaEntity      SELECT ID FROM D3MetaEntity me    WHERE EXISTS(SELECT ID FROM @tmpMetaDatabase t  WHERE t.ID=me.MetaDatabaseID)\n";
		osql << "    /* remember all related D3MetaColumn records */\n";
		osql << "    INSERT INTO @tmpMetaColumn      SELECT ID FROM D3MetaColumn mc    WHERE EXISTS(SELECT ID FROM @tmpMetaEntity t    WHERE t.ID=mc.MetaEntityID)\n";
		osql << "    /* remember all related D3MetaKey records */\n";
		osql << "    INSERT INTO @tmpMetaKey         SELECT ID FROM D3MetaKey mk       WHERE EXISTS(SELECT ID FROM @tmpMetaEntity t    WHERE t.ID=mk.MetaEntityID)\n";
		osql << "    /* remember all related D3MetaRelation records */\n";
		osql << "    INSERT INTO @tmpMetaRelation    SELECT ID FROM D3MetaRelation mr  WHERE EXISTS(SELECT ID FROM @tmpMetaKey t       WHERE t.ID=mr.ParentKeyID OR t.ID=mr.ChildKeyID)\n";
		osql << "    /* Deal with cross database relations */\n";
		osql << "    INSERT INTO @tmpCDBMetaKey      SELECT ParentKeyID  FROM D3MetaRelation mr WHERE EXISTS(SELECT ID FROM @tmpMetaKey t WHERE t.ID=mr.ChildKeyID)  AND NOT EXISTS(SELECT ID FROM @tmpMetaKey t WHERE t.ID=mr.ParentKeyID)\n";
		osql << "    INSERT INTO @tmpCDBMetaKey      SELECT ChildKeyID   FROM D3MetaRelation mr WHERE EXISTS(SELECT ID FROM @tmpMetaKey t WHERE t.ID=mr.ParentKeyID) AND NOT EXISTS(SELECT ID FROM @tmpMetaKey t WHERE t.ID=mr.ChildKeyID)\n";
		osql << "    INSERT INTO @tmpCDBMetaEntity   SELECT MetaEntityID FROM D3MetaKey mk      WHERE EXISTS(SELECT ID FROM @tmpCDBMetaKey t WHERE t.ID=mk.ID)\n";
		osql << std::endl;
		osql << "    INSERT INTO @tmpMetaDatabase    SELECT me.MetaDatabaseID FROM @tmpCDBMetaEntity te INNER JOIN D3MetaEntity me ON te.ID=me.ID GROUP BY me.MetaDatabaseID\n";
		osql << "    INSERT INTO @tmpMetaEntity      SELECT ID                FROM @tmpCDBMetaEntity\n";
		osql << "    INSERT INTO @tmpMetaKey         SELECT ID                FROM @tmpCDBMetaKey\n";
		osql << "    INSERT INTO @tmpMetaColumn      SELECT ID                FROM D3MetaColumn mc WHERE EXISTS(SELECT ID FROM @tmpCDBMetaEntity t WHERE t.ID=mc.MetaEntityID)\n";
		osql << std::endl;
		osql << "    /* return all D3MetaDatabase */\n";
		osql << "    SELECT ";
		osql << pDB->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaDatabase)->AsSQLSelectList(false, "md");
		osql << " FROM D3MetaDatabase md WHERE EXISTS(SELECT ID FROM @tmpMetaDatabase WHERE ID=md.ID) ORDER BY ID\n";
		osql << std::endl;
		osql << "    /* return all related D3MetaEntity records */\n";
		osql << "    SELECT ";
		osql << pDB->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaEntity)->AsSQLSelectList(false, "me");
		osql << " FROM D3MetaEntity me WHERE EXISTS(SELECT ID FROM @tmpMetaEntity WHERE ID=me.ID) ORDER BY ID\n";
		osql << std::endl;
		osql << "    /* remember all related D3MetaColumn records and return them */\n";
		osql << "    SELECT ";
		osql << pDB->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaColumn)->AsSQLSelectList(false, "mc");
		osql << " FROM D3MetaColumn mc WHERE EXISTS(SELECT ID FROM @tmpMetaColumn WHERE ID=mc.ID) ORDER BY ID\n";
		osql << std::endl;
		osql << "    /* return all related D3MetaColumnChoice records */\n";
		osql << "    SELECT ";
		osql << pDB->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaColumnChoice)->AsSQLSelectList(false, "mcc");
		osql << " FROM D3MetaColumnChoice mcc WHERE EXISTS(SELECT ID FROM @tmpMetaColumn WHERE ID=mcc.MetaColumnID) ORDER BY MetaColumnID, SequenceNo\n";
		osql << std::endl;
		osql << "    /* return all related D3MetaKey records */\n";
		osql << "    SELECT ";
		osql << pDB->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaKey)->AsSQLSelectList(false, "mk");
		osql << " FROM D3MetaKey mk WHERE EXISTS(SELECT ID FROM @tmpMetaKey WHERE ID=mk.ID) ORDER BY ID\n";
		osql << std::endl;
		osql << "    /* return all related D3MetaKeyColumn records */\n";
		osql << "    SELECT ";
		osql << pDB->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaKeyColumn)->AsSQLSelectList(false, "mkc");
		osql << " FROM D3MetaKeyColumn mkc WHERE EXISTS(SELECT ID FROM @tmpMetaKey WHERE ID=mkc.MetaKeyID) ORDER BY MetaKeyID, SequenceNo\n";
		osql << std::endl;
		osql << "    /* return all related D3MetaRelation records */\n";
		osql << "    SELECT ";
		osql << pDB->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaRelation)->AsSQLSelectList(false, "mk");
		osql << " FROM D3MetaRelation mk WHERE EXISTS(SELECT ID FROM @tmpMetaRelation WHERE ID=mk.ID) ORDER BY ID\n";
		osql << std::endl;
		osql << "    /* return all D3Role records */\n";
		osql << "    SELECT ";
		osql << pDB->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3Role)->AsSQLSelectList(false);
		osql << " FROM D3Role\n";
		osql << std::endl;
		osql << "    /* return all D3User records */\n";
		osql << "    SELECT ";
		osql << pDB->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3User)->AsSQLSelectList(false);
		osql << " FROM D3User\n";
		osql << std::endl;
		osql << "    /* return all D3HistoricPassword records */\n";
		osql << "    SELECT ";
		osql << pDB->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3HistoricPassword)->AsSQLSelectList(false);
		osql << " FROM D3HistoricPassword\n";
		osql << std::endl;
		osql << "    /* return all D3RoleUser records */\n";
		osql << "    SELECT ";
		osql << pDB->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3RoleUser)->AsSQLSelectList(false);
		osql << " FROM D3RoleUser\n";
		osql << std::endl;
		osql << "    /* return all D3DatabasePermission records */\n";
		osql << "    SELECT ";
		osql << pDB->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3DatabasePermission)->AsSQLSelectList(false, "p");
		osql << " FROM D3DatabasePermission p WHERE EXISTS(SELECT ID FROM @tmpMetaDatabase WHERE ID=p.MetaDatabaseID)\n";
		osql << std::endl;
		osql << "    /* return all D3EntityPermission records */\n";
		osql << "    SELECT ";
		osql << pDB->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3EntityPermission)->AsSQLSelectList(false, "p");
		osql << " FROM D3EntityPermission p WHERE EXISTS(SELECT ID FROM @tmpMetaEntity WHERE ID=p.MetaEntityID)\n";
		osql << std::endl;
		osql << "    /* return all D3ColumnPermission records */\n";
		osql << "    SELECT ";
		osql << pDB->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3ColumnPermission)->AsSQLSelectList(false, "p");
		osql << " FROM D3ColumnPermission p WHERE EXISTS(SELECT ID FROM @tmpMetaColumn WHERE ID=p.MetaColumnID)\n";
		osql << std::endl;
		osql << "    /* return all D3RowLevelPermission records */\n";
		osql << "    SELECT ";
		osql << pDB->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3RowLevelPermission)->AsSQLSelectList(false, "p");
		osql << " FROM D3RowLevelPermission p WHERE EXISTS(SELECT ID FROM @tmpMetaEntity WHERE ID=p.MetaEntityID)\n";
		osql << std::endl;

		pDB->ExecuteSQLReadCommand(osql.str());
	}




	std::string D3MetaDatabase::GetVersionString()
	{
		char					buff[64];
		std::string		strVersionString;

		snprintf(buff, 64, "%i.%02i.%04i", GetVersionMajor(), GetVersionMinor(), GetVersionRevision());
		strVersionString = buff;

		return strVersionString;
	}




	std::string D3MetaDatabase::IDAsSoftlink()
	{
		std::ostringstream					ostrm;

		ostrm << "(SELECT ID FROM D3MetaDatabase WHERE ";
		ostrm << "Alias='" << GetAlias() << "'  AND ";
		ostrm << "VersionMajor=" << GetVersionMajor() << " AND ";
		ostrm << "VersionMinor=" << GetVersionMinor() << " AND ";
		ostrm << "VersionRevision=" << GetVersionRevision() << ')';

		return ostrm.str();
	}




	void D3MetaDatabase::ExportAsSQLScript(const std::string & strSQLFileName)
	{
		std::ofstream															sqlout;
		MetaColumnPtr															pMC;
		ColumnIndex																idx;
		bool																			bFirst;
		D3MetaDatabase::D3MetaEntities::iterator	itrEntity;
		D3MetaEntityPtr														pD3MetaEntityPtr;
		int																				i;
		std::string																strVals;

		std::cout << "Export data from " << GetAlias() << '(' << GetVersionString() << ") as SQL INSERT script to file " << strSQLFileName << std::endl;

		// Open the output file
		sqlout.open(strSQLFileName.c_str());

		if (!sqlout.is_open())
		{
			std::cout << "Failed to open " << strSQLFileName << " for writing!\n";
			return;
		}

		sqlout << "DECLARE @iMDID As int" << std::endl;
		sqlout << "DECLARE @iMEID As int" << std::endl;
		sqlout << "DECLARE @iMCID As int" << std::endl;
		sqlout << "DECLARE @iMKID As int" << std::endl;
		sqlout << "DECLARE @iParentMKID As int" << std::endl;
		sqlout << "DECLARE @iChildMKID As int" << std::endl;
		sqlout << "DECLARE @iSwitchMCID As int" << std::endl;

		// Lets Declare constants for D3::MetaDatabase::Flags so we can be more expressive
		sqlout << std::endl << "-- D3MetaDatabase Flags" << std::endl;
		for (i=0; i<sizeof(MetaDatabase::Flags::BitMaskT::M_primitiveMasks)/sizeof(const char*); i++)
		{
			if (D3::MetaDatabase::Flags::BitMaskT::M_primitiveMasks[i])
				sqlout << "DECLARE @iMDF" << D3::MetaDatabase::Flags::BitMaskT::M_primitiveMasks[i] << " int; SET @iMDF" << D3::MetaDatabase::Flags::BitMaskT::M_primitiveMasks[i] << " = " << (1u << i) << ";" << std::endl;
		}

		// Lets Declare constants for D3::MetaDatabase::Permissions so we can be more expressive
		sqlout << std::endl << "-- D3MetaDatabase Permissions" << std::endl;
		for (i=0; i<sizeof(D3::MetaDatabase::Permissions::BitMaskT::M_primitiveMasks)/sizeof(const char*); i++)
		{
			if (D3::MetaDatabase::Permissions::BitMaskT::M_primitiveMasks[i])
				sqlout << "DECLARE @iMDP" << D3::MetaDatabase::Permissions::BitMaskT::M_primitiveMasks[i] << " int; SET @iMDP" << D3::MetaDatabase::Permissions::BitMaskT::M_primitiveMasks[i] << " = " << (1u << i) << ";" << std::endl;
		}

		// Lets Declare constants for D3::MetaEntity::Flags so we can be more expressive
		sqlout << std::endl << "-- D3MetaEntity Flags" << std::endl;
		for (i=0; i<sizeof(D3::MetaEntity::Flags::BitMaskT::M_primitiveMasks)/sizeof(const char*); i++)
		{
			if (D3::MetaEntity::Flags::BitMaskT::M_primitiveMasks[i])
				sqlout << "DECLARE @iMEF" << D3::MetaEntity::Flags::BitMaskT::M_primitiveMasks[i] << " int; SET @iMEF" << D3::MetaEntity::Flags::BitMaskT::M_primitiveMasks[i] << " = " << (1u << i) << ";" << std::endl;
		}

		// Lets Declare constants for D3::MetaEntity::Permissions so we can be more expressive
		sqlout << std::endl << "-- D3MetaEntity Permissions" << std::endl;
		for (i=0; i<sizeof(D3::MetaEntity::Permissions::BitMaskT::M_primitiveMasks)/sizeof(const char*); i++)
		{
			if (D3::MetaEntity::Permissions::BitMaskT::M_primitiveMasks[i])
				sqlout << "DECLARE @iMEP" << D3::MetaEntity::Permissions::BitMaskT::M_primitiveMasks[i] << " int; SET @iMEP" << D3::MetaEntity::Permissions::BitMaskT::M_primitiveMasks[i] << " = " << (1u << i) << ";" << std::endl;
		}

		// Lets Declare constants for D3::MetaColumn::Flags so we can be more expressive
		sqlout << std::endl << "-- D3MetaColumn Flags" << std::endl;
		for (i=0; i<sizeof(D3::MetaColumn::Flags::BitMaskT::M_primitiveMasks)/sizeof(const char*); i++)
		{
			if (D3::MetaColumn::Flags::BitMaskT::M_primitiveMasks[i])
				sqlout << "DECLARE @iMCF" << D3::MetaColumn::Flags::BitMaskT::M_primitiveMasks[i] << " int; SET @iMCF" << D3::MetaColumn::Flags::BitMaskT::M_primitiveMasks[i] << " = " << (1u << i) << ";" << std::endl;
		}

		// Lets Declare constants for D3::MetaColumn::Permissions so we can be more expressive
		sqlout << std::endl << "-- D3MetaColumn Permissions" << std::endl;
		for (i=0; i<sizeof(D3::MetaColumn::Permissions::BitMaskT::M_primitiveMasks)/sizeof(const char*); i++)
		{
			if (D3::MetaColumn::Permissions::BitMaskT::M_primitiveMasks[i])
				sqlout << "DECLARE @iMCP" << D3::MetaColumn::Permissions::BitMaskT::M_primitiveMasks[i] << " int; SET @iMCP" << D3::MetaColumn::Permissions::BitMaskT::M_primitiveMasks[i] << " = " << (1u << i) << ";" << std::endl;
		}

		// Lets Declare constants for D3::MetaKey::Flags so we can be more expressive
		sqlout << std::endl << "-- D3MetaKey Flags" << std::endl;
		for (i=0; i<sizeof(D3::MetaKey::Flags::BitMaskT::M_primitiveMasks)/sizeof(const char*); i++)
		{
			if (D3::MetaKey::Flags::BitMaskT::M_primitiveMasks[i])
				sqlout << "DECLARE @iMKF" << D3::MetaKey::Flags::BitMaskT::M_primitiveMasks[i] << " int; SET @iMKF" << D3::MetaKey::Flags::BitMaskT::M_primitiveMasks[i] << " = " << (1u << i) << ";" << std::endl;
		}

		// Lets Declare constants for D3::MetaRelation::Flags so we can be more expressive
		sqlout << std::endl << "-- D3MetaRelation Flags" << std::endl;
		for (i=0; i<sizeof(D3::MetaRelation::Flags::BitMaskT::M_primitiveMasks)/sizeof(const char*); i++)
		{
			if (D3::MetaRelation::Flags::BitMaskT::M_primitiveMasks[i])
				sqlout << "DECLARE @iMRF" << D3::MetaRelation::Flags::BitMaskT::M_primitiveMasks[i] << " int; SET @iMRF" << D3::MetaRelation::Flags::BitMaskT::M_primitiveMasks[i] << " = " << (1u << i) << ";" << std::endl;
		}

		sqlout << std::endl;
		sqlout << "BEGIN TRANSACTION" << std::endl;
		sqlout << std::endl;
		sqlout << "-- INSERT D3MetaDatabase " << GetAlias() << ' ' << GetVersionString() << std::endl;
		sqlout << "INSERT INTO D3MetaDatabase (";

		for (bFirst = true, idx = 0; idx < GetMetaEntity()->GetMetaColumnCount(); idx++)
		{
			pMC = GetMetaEntity()->GetMetaColumn(idx);

			if (pMC->IsAutoNum())
				continue;

			if (bFirst)
				bFirst = false;
			else
				sqlout << ", ";

			sqlout << pMC->GetName();
		}

		sqlout << ") VALUES (" << std::endl;

		for (bFirst = true, idx = 0; idx < GetMetaEntity()->GetMetaColumnCount(); idx++)
		{
			pMC = GetMetaEntity()->GetMetaColumn(idx);

			if (pMC->IsAutoNum())
				continue;

			if (bFirst)
				bFirst = false;
			else
				sqlout << ", " << std::endl;

			sqlout << "\t\t/*" << std::setw(30) << pMC->GetName() << " */ ";

			if (pMC->GetName() == "Flags")
			{
				for (strVals="", i=0; i<sizeof(MetaDatabase::Flags::BitMaskT::M_primitiveMasks)/sizeof(const char*); i++)
				{
					if ((GetFlags() & (1u << i)) && D3::MetaDatabase::Flags::BitMaskT::M_primitiveMasks[i])
					{
						if (!strVals.empty())	strVals += " | ";
						strVals += "@iMDF";
						strVals += D3::MetaDatabase::Flags::BitMaskT::M_primitiveMasks[i];
					}
				}

				if (strVals.empty())
					sqlout << "(0)";
				else
					sqlout << '(' + strVals + ')';
			}
			else if (pMC->GetName() == "AccessRights")
			{
				for (strVals="", i=0; i<sizeof(MetaDatabase::Permissions::BitMaskT::M_primitiveMasks)/sizeof(const char*); i++)
				{
					if ((GetAccessRights() & (1u << i)) && D3::MetaDatabase::Permissions::BitMaskT::M_primitiveMasks[i])
					{
						if (!strVals.empty())	strVals += " | ";
						strVals += "@iMDP";
						strVals += D3::MetaDatabase::Permissions::BitMaskT::M_primitiveMasks[i];
					}
				}

				if (strVals.empty())
					sqlout << "(0)";
				else
					sqlout << '(' + strVals + ')';
			}
			else
				sqlout << this->GetColumn(pMC)->AsSQLString();
		}

		sqlout << ");" << std::endl;

		sqlout << "SET @iMDID=" << IDAsSoftlink() << ';' << std::endl;

		// Entities first
		for ( itrEntity =  GetD3MetaEntities()->begin();
					itrEntity != GetD3MetaEntities()->end();
					itrEntity++)
		{
			pD3MetaEntityPtr = *itrEntity;
			pD3MetaEntityPtr->ExportAsSQLScript(sqlout);
		}

		// Relations last
		for ( itrEntity =  GetD3MetaEntities()->begin();
					itrEntity != GetD3MetaEntities()->end();
					itrEntity++)
		{
			pD3MetaEntityPtr = *itrEntity;
			pD3MetaEntityPtr->ExportRelationsAsSQLScript(sqlout);
		}

		sqlout << std::endl << std::endl << "COMMIT TRANSACTION" << std::endl;
	}





	//===========================================================================
	//
	// CK_D3MetaDatabase implementation
	//

	D3_CLASS_IMPL(CK_D3MetaDatabase, InstanceKey);


	std::string CK_D3MetaDatabase::AsString()
	{
		std::string				strKey;
		D3MetaDatabasePtr	pD3MD = (D3MetaDatabasePtr) GetEntity();

		strKey += "[";
		strKey += pD3MD->GetAlias();
		strKey += " ";
		strKey += pD3MD->GetVersionString();
		strKey += "]";

		return strKey;
	}

}
