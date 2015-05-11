// MODULE: D3RoleUser source

#include "D3Types.h"

#include "D3User.h"
#include "D3Role.h"
#include "D3RoleUser.h"
#include "Session.h"

namespace D3
{
	D3_CLASS_IMPL(D3RoleUser, D3RoleUserBase);



	void D3RoleUser::ExportToJSON(std::ostream& ostrm)
	{
		ostrm << std::endl;
		ostrm << "\t\t\t{\n";
		ostrm << "\t\t\t\t\"RoleName\":\""	<< JSONEncode(GetD3Role()->GetName())	<< "\",\n";
		ostrm << "\t\t\t\t\"UserName\":\""	<< JSONEncode(GetD3User()->GetName())	<< "\"\n";
		ostrm << "\t\t\t}";
	}



	/* static */
	D3RoleUserPtr D3RoleUser::ImportFromJSON(DatabasePtr pDB, Json::Value & jsnValue)
	{
		D3RolePtr											pRole = D3Role::FindD3Role(pDB, jsnValue["RoleName"].asString());
		D3UserPtr											pUser = D3User::FindD3User(pDB, jsnValue["UserName"].asString());
		D3RoleUserPtr									pD3RoleUser = FindD3RoleUser(pDB, pRole, pUser);

		if (pD3RoleUser)
		{
			pD3RoleUser->UnMarkMarked();
		}
		else
		{
			// Won't do a thing unless we have both, pRole and pUser
			if (pRole && pUser)
			{
				pD3RoleUser = CreateD3RoleUser(pDB);
				pD3RoleUser->SetD3Role(pRole);
				pD3RoleUser->SetD3User(pUser);
			}
		}

		return pD3RoleUser;
	}



	// Find resident D3RoleUser by D3Role and D3User
	/* static */
	D3RoleUserPtr D3RoleUser::FindD3RoleUser(DatabasePtr pDatabase, D3RolePtr pRole, D3UserPtr pUser)
	{
		// Won't do a thing unless we have both, pRole and pUser
		if (pRole && pUser)
		{
			D3Role::D3RoleUsers::iterator		itrRoleUser;
			D3RoleUserPtr										pRoleUser;

			for ( itrRoleUser =  pRole->GetD3RoleUsers()->begin();
						itrRoleUser != pRole->GetD3RoleUsers()->end();
						itrRoleUser++)
			{
				pRoleUser = *itrRoleUser;

				if (pRoleUser->GetD3User() == pUser)
					return pRoleUser;
			}
		}

		return NULL;
	}



	bool D3RoleUser::On_BeforeUpdate(UpdateType eUT)
	{
		if (!D3RoleUserBase::On_BeforeUpdate(eUT))
			return false;

		if (!G_bImportingRBAC)
		{
			switch (eUT)
			{
				case SQL_Delete:
					// It is illegal to delete the role user D3_ADMIN_ROLE/D3_ADMIN_USER
					if (GetD3Role()->GetName() == D3_ADMIN_ROLE && GetD3User()->GetName() == D3_ADMIN_USER)
						throw std::runtime_error("The " D3_ADMIN_ROLE "/" D3_ADMIN_USER " role/user is required by the system and cannot be deleted");

					delete D3::RoleUser::GetRoleUserByID(GetID());
					break;
			}
		}

		return true;
	}



	bool D3RoleUser::On_AfterUpdate(UpdateType eUT)
	{
		if (!D3RoleUserBase::On_AfterUpdate(eUT))
			return false;

		if (!G_bImportingRBAC)
		{
			switch (eUT)
			{
				case SQL_Insert:
					new RoleUser(this);
					break;

				case SQL_Update:
					D3::RoleUser::GetRoleUserByID(GetID())->Refresh(this);
					break;
			}
		}

		return true;
	}









	//===========================================================================
	//
	// CK_D3RoleUser implementation
	//

	D3_CLASS_IMPL(CK_D3RoleUser, InstanceKey);


	std::string CK_D3RoleUser::AsString()
	{
		std::string			strKey;
		D3RoleUserPtr		pD3RU = (D3RoleUserPtr) GetEntity();

		strKey += "[";
		strKey += pD3RU->GetD3Role()->GetName();
		strKey += "][";
		strKey += pD3RU->GetD3User()->GetName();
		strKey += "]";

		return strKey;
	}

}
