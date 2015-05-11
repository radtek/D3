#ifndef INC_D3RowLevelPermission_H
#define INC_D3RowLevelPermission_H

#include "D3RowLevelPermissionBase.h"

namespace D3
{
	//! RBAC RowLevelPermission
	class D3_API D3RowLevelPermission : public D3RowLevelPermissionBase
  {
    D3_CLASS_DECL(D3RowLevelPermission);

    protected:
			//! Only system can create instances
      D3RowLevelPermission() {};

    public:
			//! Simple dtor
			~D3RowLevelPermission() {};

			//! Export this to JSON
			void										ExportToJSON(std::ostream& ostrm);

			//! Import this from JSON (returns NULL if nothing was imported)
			static D3RowLevelPermissionPtr		ImportFromJSON(DatabasePtr pDB, Json::Value & jsnValue);

			//! Find D3RowLevelPermission
			/*! This method looks up all resident D3RowLevelPermission objects and returns the one with
					with a matching user and a matching meta entity or NULL if none can be found
			*/
			static D3RowLevelPermissionPtr		FindD3RowLevelPermission(DatabasePtr pDatabase, D3UserPtr pUser, D3MetaEntityPtr pD3ME);

    protected:
			//! Propagates changes to D3::Role
			virtual bool						On_BeforeUpdate(UpdateType eUT);
			//! Propagates changes to D3::Role
			virtual bool						On_AfterUpdate(UpdateType eUT);
  };
}

#endif /* INC_D3RowLevelPermission_H */
