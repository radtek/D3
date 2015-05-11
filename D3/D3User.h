#ifndef INC_D3User_H
#define INC_D3User_H

#include "D3UserBase.h"
#include <json/json.h>

namespace D3
{
	//! This deals specifically with the deep cloning of a D3User object
	class UserCloningController : public D3::CloningController
	{
		public:
			//! Sets m_bAllowCloning to true if no session is provided or if the session provided belongs to the sysadmin role
			UserCloningController(EntityPtr pOrigRoot, SessionPtr pSession, DatabaseWorkspacePtr pDBWS, Json::Value* pJSONChanges) : CloningController(pOrigRoot, pSession, pDBWS, pJSONChanges) {}

			//! Returns m_bAllowCloning
			virtual	bool							WantCloned(MetaRelationPtr pMetaRelation);
			//! Returns m_bAllowCloning
			virtual	bool							WantCloned(EntityPtr pOrigCandidate)							{ return true; }

			//! Intercepted to make the D3MetaDatabase resident resident (does nothing if m_bAllowCloning is false))
			virtual	void							On_BeforeCloning();
	};





	//! RBAC User
	class D3_API D3User : public D3UserBase
  {
    D3_CLASS_DECL(D3User);

    protected:
			//! Only system can create instances
      D3User() {};

    public:
			//! Simple dtor
      ~D3User() {};

			//! Export this to JSON
			void										ExportToJSON(std::ostream& ostrm);

			//! Import this from JSON (returns NULL if nothing was imported)
			static D3UserPtr				ImportFromJSON(DatabasePtr pDB, Json::Value & jsnValue);

			//! Load D3User
			/*! This method looks up all resident D3User objects and returns the one with
					with a matching name. If none is found it attempts to do load the object from the
					database and if it exists return it. Null is return is none exists.
			*/
			static D3UserPtr				LoadByName(DatabasePtr pDatabase, const std::string & strName, bool bRefresh = false, bool bLazyFetch = true);

			//! Find D3User
			/*! This method looks up all resident D3User objects and returns the one with
					with a matching name or NULL if non can be found
			*/
			static D3UserPtr				FindD3User(DatabasePtr pDatabase, const std::string & strName);

			//! Updates the values of this columns from a json structure
			/*! The JSON structure must be in the form {"Name":val_0,"Name":val_1,...,"Name":val_n}
					The method:
						- iterates over all columns and looks if a value for a given column was supplied
						- if a value was supplied, it sets the value (might be null)
						- columns supplied which are not known are ignored
					This method can throw std::string exceptions with info on the problem
			*/
			virtual void						SetValues(RoleUserPtr pRoleUser, Json::Value & jsonValues);

    protected:
			//! Propagates changes to D3::User
			virtual bool						On_BeforeUpdate(UpdateType eUT);
			//! Propagates changes to D3::User
			virtual bool						On_AfterUpdate(UpdateType eUT);

			//! Overload this in subclasses so that it can construct and return your own specific CloningController
			virtual CloningController*	CreateCloningController(SessionPtr pSession, DatabaseWorkspacePtr pDBWS, Json::Value* pJSONChanges)	{ return new UserCloningController(this, pSession, pDBWS, pJSONChanges); }

			//! Returns true if strBase64PWD is a historic password
			bool										IsHistoricPassword(const std::string& strBase64PWD);

			//! Saves the current password to the D3HistoricPassword table
			void										MakePasswordHistoric();
  };
}

#endif /* INC_D3User_H */
