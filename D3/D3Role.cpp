// MODULE: D3Role source

#include "D3Types.h"

#include "D3Role.h"
#include "Session.h"

namespace D3
{
	//===========================================================================
	//
	// RoleCloningController implementation
	//

	bool RoleCloningController::WantCloned(MetaRelationPtr pMetaRelation)
	{
		return (pMetaRelation == m_pOrigRoot->GetMetaEntity()->GetChildMetaRelation(D3MDDB_D3Role_CR_D3DatabasePermissions) ||
						pMetaRelation == m_pOrigRoot->GetMetaEntity()->GetChildMetaRelation(D3MDDB_D3Role_CR_D3EntityPermissions) ||
						pMetaRelation == m_pOrigRoot->GetMetaEntity()->GetChildMetaRelation(D3MDDB_D3Role_CR_D3ColumnPermissions));
	}



	void RoleCloningController::On_BeforeCloning()
	{
		D3RolePtr					pD3Role = (D3RolePtr) m_pOrigRoot;

		// Make this resident
		pD3Role->LoadAllD3DatabasePermissions(m_pOrigRoot->GetDatabase());
		pD3Role->LoadAllD3EntityPermissions(m_pOrigRoot->GetDatabase());
		pD3Role->LoadAllD3ColumnPermissions(m_pOrigRoot->GetDatabase());
	}




	//===========================================================================
	//
	// D3Role implementation
	//

	D3_CLASS_IMPL(D3Role, D3RoleBase);



	void D3Role::ExportToJSON(std::ostream& ostrm)
	{
		ostrm << std::endl;
		ostrm << "\t\t\t{\n";
		ostrm << "\t\t\t\t\"Name\":\""					<< JSONEncode(GetName())							<< "\",\n";
		ostrm << "\t\t\t\t\"Enabled\":"					<< (GetEnabled() ? "true" : "false")	<< ",\n";
		ostrm << "\t\t\t\t\"Features\":"				<< GetFeatures()											<< ",\n";
		ostrm << "\t\t\t\t\"IRSSettings\":"			<< GetIRSSettings()										<< ",\n";
		ostrm << "\t\t\t\t\"V3ParamSettings\":"	<< GetV3ParamSettings()								<< ",\n";
		ostrm << "\t\t\t\t\"T3ParamSettings\":"	<< GetT3ParamSettings()								<< ",\n";
		ostrm << "\t\t\t\t\"P3ParamSettings\":"	<< GetP3ParamSettings()								<< std::endl;
		ostrm << "\t\t\t}";
	}



	/* static */
	D3RolePtr D3Role::ImportFromJSON(DatabasePtr pDB, Json::Value & jsnValue)
	{
		D3RolePtr											pD3Role = NULL;
		std::string										strName = jsnValue["Name"].asString();

		// Lookup the user by name
		pD3Role = FindD3Role(pDB, strName);

		if (pD3Role)
		{
			pD3Role->UnMarkMarked();
		}
		else
		{
			pD3Role = CreateD3Role(pDB);
			pD3Role->SetName(strName);
		}

		pD3Role->SetEnabled(jsnValue["Enabled"].asBool());
		pD3Role->SetFeatures(jsnValue["Features"].asInt());

		if (jsnValue.isMember("IRSSettings"))
			pD3Role->SetIRSSettings(jsnValue["IRSSettings"].asInt());

		if (jsnValue.isMember("V3ParamSettings"))
			pD3Role->SetV3ParamSettings(jsnValue["V3ParamSettings"].asInt());

		if (jsnValue.isMember("T3ParamSettings"))
			pD3Role->SetT3ParamSettings(jsnValue["T3ParamSettings"].asInt());

		if (jsnValue.isMember("P3ParamSettings"))
			pD3Role->SetP3ParamSettings(jsnValue["P3ParamSettings"].asInt());

		return pD3Role;
	}



	// Find resident D3Role by name
	/* static */
	D3RolePtr D3Role::FindD3Role(DatabasePtr pDatabase, const std::string & strName)
	{
		MetaKeyPtr										pMK;
		InstanceKeyPtr								pKey;
		D3RolePtr											pD3Role = NULL;


		assert(pDatabase);
		assert(pDatabase->GetMetaDatabase() == MetaDatabase::GetMetaDictionary());

		pMK = pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3Role)->GetMetaKey(D3MDDB_D3Role_SK_D3Role);

		assert(pMK);

		TemporaryKey		tmpKey(*pMK);
		tmpKey.GetColumn(D3MDDB_D3Role_Name)->SetValue(strName);
		pKey = pMK->FindInstanceKey(&tmpKey, pDatabase);

		if (pKey)
			pD3Role = (D3RolePtr) pKey->GetEntity();

		return pD3Role;
	}



	bool D3Role::On_BeforeUpdate(UpdateType eUT)
	{
		if (!D3RoleBase::On_BeforeUpdate(eUT))
			return false;

		if (!G_bImportingRBAC)
		{
			switch (eUT)
			{
				case SQL_Delete:
					// It is illegal to delete the D3_ADMIN_ROLE
					if (GetName() == D3_ADMIN_ROLE)
						throw std::runtime_error("The " D3_ADMIN_ROLE " role is required by the system and cannot be deleted");

					delete D3::Role::GetRoleByID(GetID());
					break;
			}
		}

		return true;
	}



	bool D3Role::On_AfterUpdate(UpdateType eUT)
	{
		if (!D3RoleBase::On_AfterUpdate(eUT))
			return false;

		if (!G_bImportingRBAC)
		{
			RolePtr		pRole;

			switch (eUT)
			{
				case SQL_Insert:
					pRole = new Role(this);
					pRole->AddDefaultPermissions();
					break;

				case SQL_Update:
					D3::Role::GetRoleByID(GetID())->Refresh(this);
					break;
			}
		}

		return true;
	}
}
