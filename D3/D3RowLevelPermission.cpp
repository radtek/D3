// MODULE: D3RowLevelPermission source

#include "D3Types.h"

#include "D3RowLevelPermission.h"
#include "D3User.h"
#include "D3MetaDatabase.h"
#include "D3MetaEntity.h"
#include "Session.h"

namespace D3
{
	D3_CLASS_IMPL(D3RowLevelPermission, D3RowLevelPermissionBase);



	void D3RowLevelPermission::ExportToJSON(std::ostream& ostrm)
	{
		ostrm << std::endl;
		ostrm << "\t\t\t{\n";
		ostrm << "\t\t\t\t\"UserName\":\""					<< JSONEncode(GetD3User()->GetName())															<< "\",\n";
		ostrm << "\t\t\t\t\"MetaDatabaseAlias\":\""	<< JSONEncode(GetD3MetaEntity()->GetD3MetaDatabase()->GetAlias())	<< "\",\n";
		ostrm << "\t\t\t\t\"MetaEntityName\":\""		<< JSONEncode(GetD3MetaEntity()->GetName())												<< "\",\n";
		ostrm << "\t\t\t\t\"AccessRights\":"				<< GetAccessRights()																							<< ",\n";
		ostrm << "\t\t\t\t\"PKValueList\":\""				<< JSONEncode(GetPKValueList())																		<< "\"\n";
		ostrm << "\t\t\t}";
	}



	/* static */
	D3RowLevelPermissionPtr D3RowLevelPermission::ImportFromJSON(DatabasePtr pDB, Json::Value & jsnValue)
	{
		D3UserPtr											pUser = D3User::FindD3User(pDB, jsnValue["UserName"].asString());
		D3MetaDatabasePtr							pD3MD = D3MetaDatabase::FindUniqueD3MetaDatabase(pDB, jsnValue["MetaDatabaseAlias"].asString());
		D3RowLevelPermissionPtr				pD3RowLevelPermission = NULL;

		// Won't do a thing unless we have both, a D3User and a D3MetaDatabase
		if (pUser && pD3MD)
		{
			D3MetaEntityPtr						pD3ME = pD3MD->FindD3MetaEntity(jsnValue["MetaEntityName"].asString());

			pD3RowLevelPermission = FindD3RowLevelPermission(pDB, pUser, pD3ME);

			if (pD3RowLevelPermission)
			{
				pD3RowLevelPermission->UnMarkMarked();
			}
			else
			{
				// Won't do a thing unless we have a D3MetaEntity
				if (pD3ME)
				{
					pD3RowLevelPermission = CreateD3RowLevelPermission(pDB);
					pD3RowLevelPermission->SetD3User(pUser);
					pD3RowLevelPermission->SetD3MetaEntity(pD3ME);
				}
			}

			if (pD3RowLevelPermission)
			{
				pD3RowLevelPermission->SetAccessRights(jsnValue["AccessRights"].asInt());
				pD3RowLevelPermission->SetPKValueList(jsnValue["PKValueList"].asString());
			}
		}

		return pD3RowLevelPermission;
	}



	// Find resident D3RowLevelPermission by D3User and D3MetaEntity
	/* static */
	D3RowLevelPermissionPtr D3RowLevelPermission::FindD3RowLevelPermission(DatabasePtr pDatabase, D3UserPtr pUser, D3MetaEntityPtr pD3ME)
	{
		// Won't do a thing unless we have both, pUser and pD3ME
		if (pUser && pD3ME)
		{
			D3User::D3RowLevelPermissions::iterator		itrRowLevelPermission;
			D3RowLevelPermissionPtr										pRowLevelPermission;

			for ( itrRowLevelPermission =  pUser->GetD3RowLevelPermissions()->begin();
						itrRowLevelPermission != pUser->GetD3RowLevelPermissions()->end();
						itrRowLevelPermission++)
			{
				pRowLevelPermission = *itrRowLevelPermission;

				if (pRowLevelPermission->GetD3MetaEntity() == pD3ME)
					return pRowLevelPermission;
			}
		}

		return NULL;
	}



	bool D3RowLevelPermission::On_BeforeUpdate(UpdateType eUT)
	{
		if (!D3RowLevelPermissionBase::On_BeforeUpdate(eUT))
			return false;

		if (!G_bImportingRBAC)
		{
			D3::UserPtr			pUser = D3::User::GetUserByID(GetUserID());

			switch (eUT)
			{
				case SQL_Delete:
					pUser->DeletePermissions(this);
					break;
			}
		}

		return true;
	}



	bool D3RowLevelPermission::On_AfterUpdate(UpdateType eUT)
	{
		if (!D3RowLevelPermissionBase::On_AfterUpdate(eUT))
			return false;

		if (!G_bImportingRBAC)
		{
			D3::UserPtr			pUser = D3::User::GetUserByID(GetUserID());

			switch (eUT)
			{
				case SQL_Insert:
				case SQL_Update:
					pUser->SetPermissions(this);
					break;
			}
		}

		return true;
	}
}
