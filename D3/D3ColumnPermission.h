#ifndef INC_D3ColumnPermission_H
#define INC_D3ColumnPermission_H

#include "D3ColumnPermissionBase.h"

namespace D3
{
	//! RBAC ColumnPermissions
	class D3_API D3ColumnPermission : public D3ColumnPermissionBase
  {
    D3_CLASS_DECL(D3ColumnPermission);

    protected:
			//! Only system can create instances
      D3ColumnPermission() {};

    public:
			//! Simple dtor
			~D3ColumnPermission() {};

			//! Export this to JSON
			void										ExportToJSON(std::ostream& ostrm);

			//! Import this from JSON (returns NULL if nothing was imported)
			static D3ColumnPermissionPtr		ImportFromJSON(DatabasePtr pDatabase, Json::Value & jsnValue);

			//! Find D3ColumnPermission
			/*! This method looks up all resident D3ColumnPermission objects and returns the one with
					with a matching D3Role and a matching D3MetaColumn or NULL if none can be found
			*/
			static D3ColumnPermissionPtr		FindD3ColumnPermission(DatabasePtr pDatabase, D3RolePtr pRole, D3MetaColumnPtr pD3MetaColumn);

    protected:
			//! Propagates changes to D3::Role
			virtual bool						On_BeforeUpdate(UpdateType eUT);
			//! Propagates changes to D3::Role
			virtual bool						On_AfterUpdate(UpdateType eUT);
  };
}

#endif /* INC_D3ColumnPermission_H */
