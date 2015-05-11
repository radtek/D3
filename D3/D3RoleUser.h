#ifndef INC_D3RoleUser_H
#define INC_D3RoleUser_H

#include "D3RoleUserBase.h"

namespace D3
{
	//! RBAC RoleUser
	class D3_API D3RoleUser : public D3RoleUserBase
  {
    D3_CLASS_DECL(D3RoleUser);

    protected:
			//! Only system can create instances
      D3RoleUser() {};

    public:
			//! Simple dtor
			~D3RoleUser() {};

			//! Export this to JSON
			void										ExportToJSON(std::ostream& ostrm);

			//! Import this from JSON (returns NULL if nothing was imported)
			static D3RoleUserPtr		ImportFromJSON(DatabasePtr pDB, Json::Value & jsnValue);

			//! Find D3RoleUser
			/*! This method looks up all resident D3RoleUser objects and returns the one with
					with a matching pRole and pUser or NULL if none can be found
			*/
			static D3RoleUserPtr		FindD3RoleUser(DatabasePtr pDatabase, D3RolePtr pRole, D3UserPtr pUser);

    protected:
			//! Propagates changes to D3::RoleUser
			virtual bool						On_BeforeUpdate(UpdateType eUT);
			//! Propagates changes to D3::RoleUser
			virtual bool						On_AfterUpdate(UpdateType eUT);
  };






	class D3_API CK_D3RoleUser : public InstanceKey
	{
		D3_CLASS_DECL(CK_D3RoleUser);

		public:
			CK_D3RoleUser() {};
			~CK_D3RoleUser() {};

			//! Returns a string reflecting the database and the role/user name like [role][user]
			virtual std::string			AsString();
	};


}

#endif /* INC_D3RoleUser_H */
