#ifndef INC_D3Role_H
#define INC_D3Role_H

#include "D3RoleBase.h"

namespace D3
{
	//! This deals specifically with the deep cloning of a D3Role object
	class D3_API RoleCloningController : public D3::CloningController
	{
		public:
			//! Sets m_bAllowCloning to true if no session is provided or if the session provided belongs to the sysadmin role
			RoleCloningController(EntityPtr pOrigRoot, SessionPtr pSession, DatabaseWorkspacePtr pDBWS, Json::Value* pJSONChanges) : CloningController(pOrigRoot, pSession, pDBWS, pJSONChanges) {}

			//! Returns m_bAllowCloning
			virtual	bool							WantCloned(MetaRelationPtr pMetaRelation);
			//! Returns m_bAllowCloning
			virtual	bool							WantCloned(EntityPtr pOrigCandidate)							{ return true; }

			//! Intercepted to make the D3MetaDatabase resident resident (does nothing if m_bAllowCloning is false))
			virtual	void							On_BeforeCloning();
	};





	//! RBAC Role
	class D3_API D3Role : public D3RoleBase
  {
    D3_CLASS_DECL(D3Role);

    protected:
			//! Only system can create instances
      D3Role() {};

    public:
			//! Simple dtor
			~D3Role() {};

			//! Export this to JSON
			void										ExportToJSON(std::ostream& ostrm);

			//! Import this from JSON (returns NULL if nothing was imported)
			static D3RolePtr				ImportFromJSON(DatabasePtr pDB, Json::Value & jsnValue);

			//! Find D3Role
			/*! This method looks up all resident D3Role objects and returns the one with
					with a matching name or NULL if non can be found
			*/
			static D3RolePtr				FindD3Role(DatabasePtr pDatabase, const std::string & strName);

    protected:
			//! Propagates changes to D3::Role
			virtual bool						On_BeforeUpdate(UpdateType eUT);
			//! Propagates changes to D3::Role
			virtual bool						On_AfterUpdate(UpdateType eUT);

			//! Overload this in subclasses so that it can construct and return your own specific CloningController
			virtual CloningController*	CreateCloningController(SessionPtr pSession, DatabaseWorkspacePtr pDBWS, Json::Value* pJSONChanges)	{ return new RoleCloningController(this, pSession, pDBWS, pJSONChanges); }
  };
}

#endif /* INC_D3Role_H */
