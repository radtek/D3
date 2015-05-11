#ifndef INC_D3EntityPermission_H
#define INC_D3EntityPermission_H

#include "D3EntityPermissionBase.h"

namespace D3
{
	//! RBAC DatabasePermissions
	class D3_API D3EntityPermission : public D3EntityPermissionBase
  {
    D3_CLASS_DECL(D3EntityPermission);

    protected:
			//! Only system can create instances
      D3EntityPermission() {};

    public:
			//! Simple dtor
			~D3EntityPermission() {};

			//! Export this to JSON
			void										ExportToJSON(std::ostream& ostrm);

			//! Import this from JSON (returns NULL if nothing was imported)
			static D3EntityPermissionPtr		ImportFromJSON(DatabasePtr pDatabase, Json::Value & jsnValue);

			//! Find D3EntityPermission
			/*! This method looks up all resident D3EntityPermission objects and returns the one with
					with a matching D3Role and a matching D3MetaEntity or NULL if none can be found
			*/
			static D3EntityPermissionPtr		FindD3EntityPermission(DatabasePtr pDatabase, D3RolePtr pRole, D3MetaEntityPtr pD3MetaEntity);

    protected:
			//! Propagates changes to D3::Role
			virtual bool						On_BeforeUpdate(UpdateType eUT);
			//! Propagates changes to D3::Role
			virtual bool						On_AfterUpdate(UpdateType eUT);
  };
}

#endif /* INC_D3EntityPermission_H */
