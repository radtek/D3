// MODULE: D3DatabasePermission source

#include "D3Types.h"

#include "D3DatabasePermission.h"
#include "D3Role.h"
#include "D3MetaDatabase.h"
#include "Session.h"

namespace D3
{
	D3_CLASS_IMPL(D3DatabasePermission, D3DatabasePermissionBase);



	void D3DatabasePermission::ExportToJSON(std::ostream& ostrm)
	{
		ostrm << std::endl;
		ostrm << "\t\t\t{\n";
		ostrm << "\t\t\t\t\"RoleName\":\""					<< JSONEncode(GetD3Role()->GetName())						<< "\",\n";
		ostrm << "\t\t\t\t\"MetaDatabaseAlias\":\""	<< JSONEncode(GetD3MetaDatabase()->GetAlias())	<< "\",\n";
		ostrm << "\t\t\t\t\"AccessRights\":"				<< GetAccessRights()														<< std::endl;
		ostrm << "\t\t\t}";
	}



	/* static */
	D3DatabasePermissionPtr D3DatabasePermission::ImportFromJSON(DatabasePtr pDatabase, Json::Value & jsnValue)
	{
		D3RolePtr											pRole = D3Role::FindD3Role(pDatabase, jsnValue["RoleName"].asString());
		D3MetaDatabasePtr							pD3MetaDatabase = D3MetaDatabase::FindUniqueD3MetaDatabase(pDatabase, jsnValue["MetaDatabaseAlias"].asString());
		D3DatabasePermissionPtr				pD3DatabasePermission = FindD3DatabasePermission(pDatabase, pRole, pD3MetaDatabase);

		if (pD3DatabasePermission)
		{
			pD3DatabasePermission->UnMarkMarked();
		}
		else
		{
			// Won't do a thing unless we have a D3MetaDatabase
			if (pD3MetaDatabase)
			{
				pD3DatabasePermission = CreateD3DatabasePermission(pDatabase);
				pD3DatabasePermission->SetD3Role(pRole);
				pD3DatabasePermission->SetD3MetaDatabase(pD3MetaDatabase);
			}
		}

		if (pD3DatabasePermission)
		{
			pD3DatabasePermission->SetAccessRights(jsnValue["AccessRights"].asInt());
		}

		return pD3DatabasePermission;
	}



	// Find resident D3DatabasePermission by D3Role and D3MetaDatabase
	/* static */
	D3DatabasePermissionPtr D3DatabasePermission::FindD3DatabasePermission(DatabasePtr pDatabase, D3RolePtr pRole, D3MetaDatabasePtr pD3MetaDatabase)
	{
		// Won't do a thing unless we have both, pRole and pD3ME
		if (pRole && pD3MetaDatabase)
		{
			D3Role::D3DatabasePermissions::iterator		itrDatabasePermission;
			D3DatabasePermissionPtr										pDatabasePermission;

			for ( itrDatabasePermission =  pRole->GetD3DatabasePermissions()->begin();
						itrDatabasePermission != pRole->GetD3DatabasePermissions()->end();
						itrDatabasePermission++)
			{
				pDatabasePermission = *itrDatabasePermission;

				if (pDatabasePermission->GetD3MetaDatabase() == pD3MetaDatabase)
					return pDatabasePermission;
			}
		}

		return NULL;
	}



	bool D3DatabasePermission::On_BeforeUpdate(UpdateType eUT)
	{
		if (!D3DatabasePermissionBase::On_BeforeUpdate(eUT))
			return false;

		if (!G_bImportingRBAC)
		{
			D3::RolePtr			pRole = D3::Role::GetRoleByID(GetRoleID());

			switch (eUT)
			{
				case SQL_Delete:
					pRole->DeletePermissions(this);
					break;
			}
		}

		return true;
	}



	bool D3DatabasePermission::On_AfterUpdate(UpdateType eUT)
	{
		if (!D3DatabasePermissionBase::On_AfterUpdate(eUT))
			return false;

		if (!G_bImportingRBAC)
		{
			D3::RolePtr			pRole = D3::Role::GetRoleByID(GetRoleID());

			switch (eUT)
			{
				case SQL_Insert:
				case SQL_Update:
					pRole->SetPermissions(this);
					break;
			}
		}

		return true;
	}
}
