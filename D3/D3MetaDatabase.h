#ifndef INC_D3MetaDatabase_H
#define INC_D3MetaDatabase_H

#include "D3MetaDatabaseBase.h"

namespace D3
{
	//! This deals specifically with the deep cloning of a D3MetaDatabase object
	class D3_API MetaDatabaseCloningController : public D3::CloningController
	{
		protected:
			bool		m_bAllowCloning;		//!< Set by constructor

		public:
			//! Sets m_bAllowCloning to true if no session is provided or if the session provided belongs to the sysadmin role
			MetaDatabaseCloningController(EntityPtr pOrigRoot, SessionPtr pSession, DatabaseWorkspacePtr pDBWS, Json::Value* pJSONChanges);

			//! Returns m_bAllowCloning
			virtual	bool							WantCloned(MetaRelationPtr pMetaRelation)					{ return m_bAllowCloning; }
			//! Returns m_bAllowCloning
			virtual	bool							WantCloned(EntityPtr pOrigCandidate)							{ return m_bAllowCloning; }

			//! Intercepted to make the D3MetaDatabase resident resident (does nothing if m_bAllowCloning is false))
			virtual	void							On_BeforeCloning();
	};





	//! D3MetaDatabase objects represent instances of D3MetaDatabase records from the D3 Meta Dictionary database
	/*! This class is based on the generated #D3MetaDatabaseBase class.
	*/
	class D3_API D3MetaDatabase : public D3MetaDatabaseBase
	{
		friend class D3::MetaDatabase;

		D3_CLASS_DECL(D3MetaDatabase);

		protected:
			D3MetaDatabase() {}
			D3MetaDatabase(DatabasePtr pDB, MetaDatabasePtr pMD);

		public:
			~D3MetaDatabase() {}

			//! Return the D3MetaEntity object with the specified name (throws a D3::Exception if the column does not exist)
			D3MetaEntityPtr						GetD3MetaEntity	(const std::string & strEntityName);

			//! Load and return the D3MetaDatabase instance matching the details passed in
			/*! The parameters m_strAlias, m_iVersionMajor, m_iVersionMinor and m_iVersionRevision
					consitute a unique secondary key. Therefore, make sure all values are populated.
			*/
			/* static */
			static D3MetaDatabasePtr	LoadD3MetaDatabase(DatabasePtr pDatabase, const std::string & strAlias, int iVersionMajor, int iVersionMinor, int iVersionRevision);

			//! Find unique D3MetaDatabase
			/*! This method looks up all resident D3MetaDatabase objects and returns an instance only
					if it matches the passed in strAlias and if it is the only one that matches it.

					The method throws a D3::Exception if multiple D3MetaDatabase instances matching strAlias exist.
			*/
			/* static */
			static D3MetaDatabasePtr	FindUniqueD3MetaDatabase(DatabasePtr pDatabase, const std::string & strAlias);

			//! Find resident D3MetaEntity
			/*! This method looks up all related D3MetaEntity objects and returns the instance
					matching the passed in name.
			*/
			D3MetaEntityPtr						FindD3MetaEntity(const std::string & strName);

			//! Makes the D3MetaDatabase instances matching the passed in MetaDatabaseDefinitions resident (i.e. loads the requested D3MetaDatabase objects and all their associated details)
			static void								MakeD3MetaDictionariesResident(DatabasePtr pDatabase, MetaDatabaseDefinitionList & listMDDefs);

			//! Returns the databases version info as a string in the format 0.00.0000
			std::string								GetVersionString();

			//! Returns this as a softlink like e.g.: (SELECT ID FROM D3MetaDatabase WHERE Alias=APAL3DB AND VersionMajor=8 AND VersionMinor=1 AND VersionRevision=1)
			std::string								IDAsSoftlink();

			//! Export this D3MetaDatabase record and its related dictionary objects as INSERT SQL script (this should be have been loaded via MakeD3MetaDictionariesResident)
			/*! Parameters:

					@strSQLFileName:	The name of the SQL script that is generated.

					Note: This method creates an SQL file containing statements like

									INSERT (col1, col2, ..., coln) VALUES (val1, val2, ..., valn)

					for each row related directly or indirectly with this in the following tables:

						D3MetaDatabase (this)
						D3MetaEntity
						D3MetaColumn
						D3MetaColumnChoice
						D3MetaKey
						D3MetaKeyColumn
						D3MetaRelation

					The INSERT statements will be built such that it runs on an already populated database by
					relating with other parent objects by name and version.
			*/
			virtual void							ExportAsSQLScript(const std::string & strSQLFileName);

		private:
			//! OTL version of MakeD3MetaDictionariesResident
			static void								MakeD3MetaDictionariesResidentConventional(DatabasePtr pDatabase, MetaDatabaseDefinitionList & listMDDefs);

			//! ODBC version of MakeD3MetaDictionariesResident
			static void								MakeD3MetaDictionariesResidentODBC(DatabasePtr pDatabase, MetaDatabaseDefinitionList & listMDDefs);

			//! This method creates the stored procedure makeMetaDictionaryResident which is created by MakeD3MetaDictionariesResidentODBC if it doesn't exist
			/*! Stored Procedure makeMetaDictionaryResident
				  -------------------------------------------

				 What we pass in is a semicolon delimited list of meta database identifiers in the form

						 alias;major;minor;revision

				 You can append multiple such strings and separate them using the bar | delimiter, e.g.:

						 APAL3DB;6;4;3|RTCISDB;6;0;1

				 The stored procedure returns the following result sets in exactly this order:

						 D3MetaDatabase
						 D3MetaEntity
						 D3MetaColumn
						 D3MetaColumnChoice
						 D3MetaKey
						 D3MetaRelation
						 D3Role
						 D3RoleUser
						 D3User
						 D3DatabasePermission
						 D3EntityPermission
						 D3ColumnPermission
						 D3KeyPermission
						 D3RelationPermission

				 Notes: The resultsets may be empty if the passed in string doesn't match existing records in D3MetaDatabase.
				 The result sets are populated with all the data related with any of the D3MetaDatabase identified by the
				 input parameter. It is possible that data related to other D3MetaDatabase objects is retrned. This is the case
				 if there are cross database relations and you do not specify all databases involved in cross relations.
			*/
			static void								CreateMakeDictionaryResidentStoredProcedure(DatabasePtr pDB);

		protected:
			//! Overload this in subclasses so that it can construct and return your own specific CloningController
			virtual CloningController*	CreateCloningController(SessionPtr pSession, DatabaseWorkspacePtr pDBWS, Json::Value* pJSONChanges)	{ return new MetaDatabaseCloningController(this, pSession, pDBWS, pJSONChanges); }
	};






	class D3_API CK_D3MetaDatabase : public InstanceKey
	{
		D3_CLASS_DECL(CK_D3MetaDatabase);

		public:
			CK_D3MetaDatabase() {};
			~CK_D3MetaDatabase() {};

			//! Returns a string reflecting the database and the entities name like [APAL3DB.6.06.0003]
			virtual std::string			AsString();
	};


} // end namespace D3

#endif /* INC_D3MetaDatabase_H */
