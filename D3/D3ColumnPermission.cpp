// MODULE: D3ColumnPermission source

#include "D3Types.h"

#include "D3ColumnPermission.h"
#include "D3Role.h"
#include "D3MetaDatabase.h"
#include "D3MetaEntity.h"
#include "D3MetaColumn.h"
#include "Session.h"

namespace D3
{
	D3_CLASS_IMPL(D3ColumnPermission, D3ColumnPermissionBase);



	void D3ColumnPermission::ExportToJSON(std::ostream& ostrm)
	{
		ostrm << std::endl;
		ostrm << "\t\t\t{\n";
		ostrm << "\t\t\t\t\"RoleName\":\""					<< JSONEncode(GetD3Role()->GetName())																									<< "\",\n";
		ostrm << "\t\t\t\t\"MetaDatabaseAlias\":\""	<< JSONEncode(GetD3MetaColumn()->GetD3MetaEntity()->GetD3MetaDatabase()->GetAlias())	<< "\",\n";
		ostrm << "\t\t\t\t\"MetaEntityName\":\""		<< JSONEncode(GetD3MetaColumn()->GetD3MetaEntity()->GetName())												<< "\",\n";
		ostrm << "\t\t\t\t\"MetaColumnName\":\""		<< JSONEncode(GetD3MetaColumn()->GetName())																						<< "\",\n";
		ostrm << "\t\t\t\t\"AccessRights\":"				<< GetAccessRights()																																	<< std::endl;
		ostrm << "\t\t\t}";
	}



	/* static */
	D3ColumnPermissionPtr D3ColumnPermission::ImportFromJSON(DatabasePtr pDatabase, Json::Value & jsnValue)
	{
		D3RolePtr											pRole = D3Role::FindD3Role(pDatabase, jsnValue["RoleName"].asString());
		D3MetaDatabasePtr							pD3MetaDatabase = D3MetaDatabase::FindUniqueD3MetaDatabase(pDatabase, jsnValue["MetaDatabaseAlias"].asString());
		D3ColumnPermissionPtr					pD3ColumnPermission = NULL;

		// Won't do a thing unless we have both, a D3Role and a D3MetaDatabase
		if (pRole && pD3MetaDatabase)
		{
			D3MetaEntityPtr			pD3MetaEntity = pD3MetaDatabase->FindD3MetaEntity(jsnValue["MetaEntityName"].asString());

			if (pD3MetaEntity)
			{
				D3MetaColumnPtr			pD3MetaColumn = pD3MetaEntity->FindD3MetaColumn(jsnValue["MetaColumnName"].asString());

				pD3ColumnPermission = FindD3ColumnPermission(pDatabase, pRole, pD3MetaColumn);

				if (pD3ColumnPermission)
				{
					pD3ColumnPermission->UnMarkMarked();
				}
				else
				{
					// Won't do a thing unless we have a D3MetaDatabase
					if (pD3MetaColumn)
					{
						pD3ColumnPermission = CreateD3ColumnPermission(pDatabase);
						pD3ColumnPermission->SetD3Role(pRole);
						pD3ColumnPermission->SetD3MetaColumn(pD3MetaColumn);
					}
				}

				if (pD3ColumnPermission)
				{
					pD3ColumnPermission->SetAccessRights(jsnValue["AccessRights"].asInt());
				}
			}
		}

		return pD3ColumnPermission;
	}



	// Find resident D3ColumnPermission by D3Role and D3MetaDatabase
	/* static */
	D3ColumnPermissionPtr D3ColumnPermission::FindD3ColumnPermission(DatabasePtr pDatabase, D3RolePtr pRole, D3MetaColumnPtr pD3MetaColumn)
	{
		// Won't do a thing unless we have both, a D3Role and a D3MetaColumn
		if (pRole && pD3MetaColumn)
		{
			D3Role::D3ColumnPermissions::iterator		itrColumnPermission;
			D3ColumnPermissionPtr										pColumnPermission;

			for ( itrColumnPermission =  pRole->GetD3ColumnPermissions()->begin();
						itrColumnPermission != pRole->GetD3ColumnPermissions()->end();
						itrColumnPermission++)
			{
				pColumnPermission = *itrColumnPermission;

				if (pColumnPermission->GetD3MetaColumn() == pD3MetaColumn)
					return pColumnPermission;
			}
		}

		return NULL;
	}



	bool D3ColumnPermission::On_BeforeUpdate(UpdateType eUT)
	{
		if (!D3ColumnPermissionBase::On_BeforeUpdate(eUT))
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



	bool D3ColumnPermission::On_AfterUpdate(UpdateType eUT)
	{
		if (!D3ColumnPermissionBase::On_AfterUpdate(eUT))
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
