// MODULE: D3User source

#include "D3Types.h"

#include "D3User.h"
#include "D3HistoricPassword.h"
#include "Session.h"
#include "D3Funcs.h"
#include "Codec.h"

namespace D3
{
	//===========================================================================
	//
	// UserCloningController implementation
	//

	bool UserCloningController::WantCloned(MetaRelationPtr pMetaRelation)
	{
		return (pMetaRelation == m_pOrigRoot->GetMetaEntity()->GetChildMetaRelation(D3MDDB_D3User_CR_D3RowLevelPermissions) ||
						pMetaRelation == m_pOrigRoot->GetMetaEntity()->GetChildMetaRelation(D3MDDB_D3User_CR_D3RoleUsers));
	}



	void UserCloningController::On_BeforeCloning()
	{
		D3UserPtr					pD3User = (D3UserPtr) m_pOrigRoot;

		// Make these resident
		pD3User->LoadAllD3RowLevelPermissions(m_pOrigRoot->GetDatabase());
		pD3User->LoadAllD3RoleUsers(m_pOrigRoot->GetDatabase());
	}




	//===========================================================================
	//
	// D3User implementation
	//

	D3_CLASS_IMPL(D3User, D3UserBase);



	void D3User::ExportToJSON(std::ostream& ostrm)
	{
		ostrm << std::endl;
		ostrm << "\t\t\t{\n";
		ostrm << "\t\t\t\t\"Name\":\""										<< JSONEncode(GetName())										<< "\",\n";
		ostrm << "\t\t\t\t\"Password\":\""								<< GetPassword().base64_encode()						<< "\",\n";
		ostrm << "\t\t\t\t\"Enabled\":"										<< (GetEnabled() ? "true" : "false")				<< ",\n";
		ostrm << "\t\t\t\t\"WarehouseAreaAccessList\":\""	<< JSONEncode(GetWarehouseAreaAccessList())	<< "\",\n";
		ostrm << "\t\t\t\t\"CustomerAccessList\":\""			<< JSONEncode(GetCustomerAccessList())			<< "\",\n";
		ostrm << "\t\t\t\t\"TransmodeAccessList\":\""			<< JSONEncode(GetTransmodeAccessList())			<< "\",\n";
		ostrm << "\t\t\t\t\"PWDAttempts\":"								<< GetPWDAttempts()													<< ",\n";
		ostrm << "\t\t\t\t\"PWDExpires\":"								<< Column(D3User_PWDExpires)->AsJSON(NULL)	<< ",\n";
		ostrm << "\t\t\t\t\"Temporary\":"									<< (GetTemporary() ? "true" : "false")			<< ",\n";
		ostrm << "\t\t\t\t\"Language\":\""								<< JSONEncode(GetLanguage())								<< "\"\n";
		ostrm << "\t\t\t}";
	}



	/* static */
	D3UserPtr D3User::ImportFromJSON(DatabasePtr pDB, Json::Value & jsnValue)
	{
		D3UserPtr											pD3User = NULL;
		std::string										strName = jsnValue["Name"].asString();
		std::string										strTmp;
		ColumnPtr											pCol;
		Data													data;

		// Lookup the user by name
		pD3User = FindD3User(pDB, strName);

		if (pD3User)
		{
			pD3User->UnMarkMarked();
		}
		else
		{
			pD3User = CreateD3User(pDB);
			pD3User->SetName(strName);
		}

		data.base64_decode(jsnValue["Password"].asString());
		pD3User->SetPassword(data);

		pD3User->SetEnabled(jsnValue["Enabled"].asBool());

		pCol = pD3User->Column(D3User_WarehouseAreaAccessList);
		strTmp = jsnValue["WarehouseAreaAccessList"].asString();
		if (strTmp.empty())
			pCol->SetNull();
		else
			pCol->SetValue(strTmp);

		pCol = pD3User->Column(D3User_CustomerAccessList);
		strTmp = jsnValue["CustomerAccessList"].asString();
		if (strTmp.empty())
			pCol->SetNull();
		else
			pCol->SetValue(strTmp);

		pCol = pD3User->Column(D3User_TransmodeAccessList);
		strTmp = jsnValue["TransmodeAccessList"].asString();
		if (strTmp.empty())
			pCol->SetNull();
		else
			pCol->SetValue(strTmp);

		if (jsnValue.isMember("PWDAttempts"))
			pD3User->SetPWDAttempts(jsnValue["PWDAttempts"].asInt());
		else
			pD3User->SetPWDAttempts(0);

		if (jsnValue.isMember("PWDExpires") && jsnValue["PWDExpires"].isString())
			pD3User->SetPWDExpires(D3Date(jsnValue["PWDExpires"].asString()));

		if (jsnValue.isMember("Temporary"))
			pD3User->SetTemporary(jsnValue["Temporary"].asBool());
		else
			pD3User->SetTemporary(false);

		pCol = pD3User->Column(D3User_Language);
		strTmp = jsnValue.isMember("Language") ? jsnValue["Language"].asString() : "";
		if (strTmp.empty())
			pCol->SetNull();
		else
			pCol->SetValue(strTmp);

		return pD3User;
	}



	// Make a specific instance resident
	/* static */
	D3UserPtr D3User::LoadByName(DatabasePtr pDB, const std::string & strName, bool bRefresh, bool bLazyFetch)
	{
		DatabasePtr		pDatabase = pDB;


		if (!pDatabase)
			return NULL;

		if (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase("D3MDDB"))
			pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase("D3MDDB"));

		if (!pDatabase)
			return NULL;

		MetaKeyPtr		pMK = pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3User)->GetMetaKey(D3MDDB_D3User_SK_D3User);
		TemporaryKey	tmpKey(*pMK);

		// Set all key column values
		//
		tmpKey.GetColumn(D3MDDB_D3User_Name)->SetValue(strName);

		return (D3UserPtr) pMK->LoadObject(&tmpKey, pDatabase, bRefresh, bLazyFetch);
	}



	// Find resident D3User by name
	/* static */
	D3UserPtr D3User::FindD3User(DatabasePtr pDatabase, const std::string & strName)
	{
		MetaKeyPtr										pMK;
		InstanceKeyPtr								pKey;
		D3UserPtr											pD3User = NULL;


		assert(pDatabase);
		assert(pDatabase->GetMetaDatabase() == MetaDatabase::GetMetaDictionary());

		pMK = pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3User)->GetMetaKey(D3MDDB_D3User_SK_D3User);

		assert(pMK);

		TemporaryKey		tmpKey(*pMK);
		tmpKey.GetColumn(D3MDDB_D3User_Name)->SetValue(strName);
		pKey = pMK->FindInstanceKey(&tmpKey, pDatabase);

		if (pKey)
			pD3User = (D3UserPtr) pKey->GetEntity();

		return pD3User;
	}



	bool D3User::On_BeforeUpdate(UpdateType eUT)
	{
		if (!D3UserBase::On_BeforeUpdate(eUT))
			return false;

		if (!G_bImportingRBAC)
		{
			switch (eUT)
			{
				case SQL_Delete:
					// It is illegal to delete the D3_ADMIN_USER
					if (GetName() == D3_ADMIN_USER)
						throw std::runtime_error("The " D3_ADMIN_USER " user is required by the system and cannot be deleted");

					delete D3::User::GetUserByID(GetID());
					break;
			}
		}

		return true;
	}



	bool D3User::On_AfterUpdate(UpdateType eUT)
	{
		if (!D3UserBase::On_AfterUpdate(eUT))
			return false;

		if (!G_bImportingRBAC)
		{
			switch (eUT)
			{
				case SQL_Insert:
					new User(this);
					break;

				case SQL_Update:
					D3::User::GetUserByID(GetID())->Refresh(this);
					break;
			}
		}

		return true;
	}



	void D3User::SetValues(RoleUserPtr pRoleUser, Json::Value & jsonChanges)
	{
		static int	s_pwdExpiresInDays =    Settings::Singleton().PasswordExpiresInDays();
		static int	s_allowReusingPasswords = Settings::Singleton().AllowReusingPasswords();

		if (!IsNew())
		{
			if (!m_pDatabase->HasTransaction())
				throw std::runtime_error("database must have a pending transaction");

			// only take action if Password is being modified
			if (jsonChanges.isMember("Password"))
			{
				std::string		strNewPWD = jsonChanges["Password"].asString();
				std::string		strCurrPWD = Column(D3User_Password)->IsNull() ? "" : GetPassword().base64_encode();

				if (pRoleUser && pRoleUser->GetUser()->GetName() != GetName())
				{
					// an administrator is changing a users password
					MakePasswordHistoric();

					SetTemporary(true);
					SetPWDExpires(D3Date() + boost::posix_time::time_duration(24, 0, 0));
					SetPWDAttempts(0);
				}
				else
				{
					// the user is changing his
					if (!s_allowReusingPasswords && (strCurrPWD == strNewPWD || IsHistoricPassword(strNewPWD)))
						throw std::runtime_error("cannot use password used in the past");

					MakePasswordHistoric();

					SetTemporary(false);

					if (s_pwdExpiresInDays)
						SetPWDExpires(D3Date() + boost::posix_time::time_duration(s_pwdExpiresInDays * 24, 0, 0));
					else
						SetPWDExpires(D3Date(D3Date::MaxISO8601DateTime));

					SetPWDAttempts(0);
				}
			}
		}

		Entity::SetValues(pRoleUser, jsonChanges);

		// in the case of insert, set PWDAttempts, PWDExpires and Temporary
		if (IsNew() && pRoleUser)
		{
			SetTemporary(true);
			SetPWDExpires(D3Date() + boost::posix_time::time_duration(24, 0, 0));
			SetPWDAttempts(0);
		}
	}



	bool D3User::IsHistoricPassword(const std::string& strBase64PWD)
	{
		static int						s_nRejectReusingMostRecentXPasswords = Settings::Singleton().RejectReusingMostRecentXPasswords();
		static int						s_nRejectReusingPasswordsUsedInPastXDays = Settings::Singleton().RejectReusingPasswordsUsedInPastXDays();

		const char *					pszSQL[] = // [0] = SQLServer, [1] = Oracle
		{
			"SELECT COUNT(*) FROM (SELECT Password FROM (SELECT Password, PWDExpired, ROW_NUMBER() OVER (ORDER BY PWDExpired desc) as ROWNUM FROM D3HistoricPassword WHERE %userid%) a WHERE %rowno% UNION SELECT Password FROM D3HistoricPassword WHERE %userid% AND %pwdage%) b WHERE %pwd%",
			"SELECT COUNT(*) FROM (SELECT Password FROM (SELECT Password FROM D3HistoricPassword WHERE %userid% ORDER BY PWDExpired desc) WHERE %rowno% UNION SELECT Password FROM D3HistoricPassword WHERE %userid% AND %pwdage%) WHERE %pwd%"
		};

		unsigned long						noRecs;
		std::ostringstream			ostrm;
		Data										password;
		TargetRDBMS							rdbms = m_pDatabase->GetMetaDatabase()->GetTargetRDBMS();
		std::string							strSQL(pszSQL[rdbms]);

		password.base64_decode(strBase64PWD);

		// substitute %userid%
		ostrm << "UserID=" << GetID();
		ReplaceSubstring(strSQL, "%userid%", ostrm.str());
		ostrm.str("");

		// substitute %rowno%
		if (s_nRejectReusingMostRecentXPasswords)
		{
			ostrm << "ROWNUM <= " << s_nRejectReusingMostRecentXPasswords;
			ReplaceSubstring(strSQL, "%rowno%", ostrm.str());
			ostrm.str("");
		}
		else
		{
			ReplaceSubstring(strSQL, "%rowno%", "1=2");
		}

		// substitute %pwdage%
		if (s_nRejectReusingPasswordsUsedInPastXDays)
		{
			switch (rdbms)
			{
				case SQLServer:
					ostrm << "PWDExpired > DATEADD(day, -" << s_nRejectReusingPasswordsUsedInPastXDays << ", GETDATE())";
					break;

				case Oracle:
					ostrm << "PWDExpired > (SYSDATE + -" << s_nRejectReusingPasswordsUsedInPastXDays << " * INTERVAL '1' DAY)";
					break;
			}

			ReplaceSubstring(strSQL, "%pwdage%", ostrm.str());
			ostrm.str("");
		}
		else
		{
			ReplaceSubstring(strSQL, "%pwdage%", "1=2");
		}

		// substitute %pwd%
		switch (rdbms)
		{
			case SQLServer:
				ostrm << "Password=0x" << charToHex(password, password.length());
				break;

			case Oracle:
				ostrm << "Password=hextoraw('" << charToHex(password, password.length()) << "')";
				break;
		}

		ReplaceSubstring(strSQL, "%pwd%", ostrm.str());
		ostrm.str("");

		m_pDatabase->ExecuteSingletonSQLCommand(strSQL, noRecs);

		if (noRecs > 0)
			return true;

		return false;
	}



	void D3User::MakePasswordHistoric()
	{
		ColumnPtr			pCol = Column(D3User_Password);

		if (!m_pDatabase->HasTransaction())
			throw std::runtime_error("database must have a pending transaction");

		// nothing to do if Temporary is true
		if (!GetTemporary())
		{
			if (!pCol->IsNull())
			{
				std::string		strOldPWD = pCol->AsString();
				if (!strOldPWD.empty())
				{
					if (!IsHistoricPassword(strOldPWD))
					{
						std::ostringstream		strmSQL;

						strmSQL << "INSERT INTO D3HistoricPassword (UserID, Password, PWDExpired) VALUES (" << GetID() << ",";
						strmSQL << pCol->AsSQLString();
						strmSQL << ",";

						switch (m_pDatabase->GetMetaDatabase()->GetTargetRDBMS())
						{
							case SQLServer:
								strmSQL << "GETDATE()";
								break;

							case Oracle:
								strmSQL << "SYSDATE";
								break;
						}

						strmSQL << ")";

						m_pDatabase->ExecuteSQLUpdateCommand(strmSQL.str());
					}
				}
			}
		}
	}




} /* end namespace D3 */
