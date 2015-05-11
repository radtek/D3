// MODULE: D3EntityPermission source

#include "D3Types.h"

#include "D3EntityPermission.h"
#include "D3Role.h"
#include "D3MetaDatabase.h"
#include "D3MetaEntity.h"
#include "Session.h"

namespace D3
{
	D3_CLASS_IMPL(D3EntityPermission, D3EntityPermissionBase);



	void D3EntityPermission::ExportToJSON(std::ostream& ostrm)
	{
		ostrm << std::endl;
		ostrm << "\t\t\t{\n";
		ostrm << "\t\t\t\t\"RoleName\":\""					<< JSONEncode(GetD3Role()->GetName())															<< "\",\n";
		ostrm << "\t\t\t\t\"MetaDatabaseAlias\":\""	<< JSONEncode(GetD3MetaEntity()->GetD3MetaDatabase()->GetAlias())	<< "\",\n";
		ostrm << "\t\t\t\t\"MetaEntityName\":\""		<< JSONEncode(GetD3MetaEntity()->GetName())												<< "\",\n";
		ostrm << "\t\t\t\t\"AccessRights\":"				<< GetAccessRights()																							<< std::endl;
		ostrm << "\t\t\t}";
	}



	/* static */
	D3EntityPermissionPtr D3EntityPermission::ImportFromJSON(DatabasePtr pDatabase, Json::Value & jsnValue)
	{
		D3RolePtr											pRole = D3Role::FindD3Role(pDatabase, jsnValue["RoleName"].asString());
		D3MetaDatabasePtr							pD3MetaDatabase = D3MetaDatabase::FindUniqueD3MetaDatabase(pDatabase, jsnValue["MetaDatabaseAlias"].asString());
		D3EntityPermissionPtr					pD3EntityPermission = NULL;

		// Won't do a thing unless we have both, a D3Role and a D3MetaDatabase
		if (pRole && pD3MetaDatabase)
		{
			D3MetaEntityPtr						pD3MetaEntity = pD3MetaDatabase->FindD3MetaEntity(jsnValue["MetaEntityName"].asString());

			pD3EntityPermission = FindD3EntityPermission(pDatabase, pRole, pD3MetaEntity);

			if (pD3EntityPermission)
			{
				pD3EntityPermission->UnMarkMarked();
			}
			else
			{
				// Won't do a thing unless we have a D3MetaDatabase
				if (pD3MetaEntity)
				{
					pD3EntityPermission = CreateD3EntityPermission(pDatabase);
					pD3EntityPermission->SetD3Role(pRole);
					pD3EntityPermission->SetD3MetaEntity(pD3MetaEntity);
				}
			}

			if (pD3EntityPermission)
			{
				pD3EntityPermission->SetAccessRights(jsnValue["AccessRights"].asInt());
			}
		}

		return pD3EntityPermission;
	}



	// Find resident D3EntityPermission by D3Role and D3MetaDatabase
	/* static */
	D3EntityPermissionPtr D3EntityPermission::FindD3EntityPermission(DatabasePtr pDatabase, D3RolePtr pRole, D3MetaEntityPtr pD3MetaEntity)
	{
		// Won't do a thing unless we have both, a D3Role and a D3MetaEntity
		if (pRole && pD3MetaEntity)
		{
			D3Role::D3EntityPermissions::iterator		itrEntityPermission;
			D3EntityPermissionPtr										pEntityPermission;

			for ( itrEntityPermission =  pRole->GetD3EntityPermissions()->begin();
						itrEntityPermission != pRole->GetD3EntityPermissions()->end();
						itrEntityPermission++)
			{
				pEntityPermission = *itrEntityPermission;

				if (pEntityPermission->GetD3MetaEntity() == pD3MetaEntity)
					return pEntityPermission;
			}
		}

		return NULL;
	}



	bool D3EntityPermission::On_BeforeUpdate(UpdateType eUT)
	{
		if (!D3EntityPermissionBase::On_BeforeUpdate(eUT))
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



	bool D3EntityPermission::On_AfterUpdate(UpdateType eUT)
	{
		if (!D3EntityPermissionBase::On_AfterUpdate(eUT))
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
