#ifndef INC_D3DatabasePermission_H
#define INC_D3DatabasePermission_H

#include "D3DatabasePermissionBase.h"

namespace D3
{
	//! RBAC DatabasePermissions
	class D3_API D3DatabasePermission : public D3DatabasePermissionBase
  {
    D3_CLASS_DECL(D3DatabasePermission);

    protected:
			//! Only system can create instances
      D3DatabasePermission() {};

    public:
			//! Simple dtor
			~D3DatabasePermission() {};

			//! Export this to JSON
			void										ExportToJSON(std::ostream& ostrm);

			//! Import this from JSON (returns NULL if nothing was imported)
			static D3DatabasePermissionPtr		ImportFromJSON(DatabasePtr pDatabase, Json::Value & jsnValue);

			//! Find D3DatabasePermission
			/*! This method looks up all resident D3DatabasePermission objects and returns the one with
					with a matching D3Role and a matching D3metaDatabase or NULL if none can be found
			*/
			static D3DatabasePermissionPtr		FindD3DatabasePermission(DatabasePtr pDatabase, D3RolePtr pRole, D3MetaDatabasePtr pD3MetaDatabase);

    protected:
			//! Propagates changes to D3::Role
			virtual bool						On_BeforeUpdate(UpdateType eUT);
			//! Propagates changes to D3::Role
			virtual bool						On_AfterUpdate(UpdateType eUT);
  };
}

#endif /* INC_D3DatabasePermission_H */
