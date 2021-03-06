#ifndef INC_D3RowLevelPermissionBase_H
#define INC_D3RowLevelPermissionBase_H

// WARNING: This file has been generated by D3. You should NEVER change this file.
//          Please refer to the documentation of MetaEntity::CreateSpecialisedCPPHeader()
//          which explains in detail how you can gegenerate this file.
//
// The file declares methods which simplify client interactions with objects of type 
// D3RowLevelPermission representing instances of D3MDDB.D3RowLevelPermission.
//
//       For D3 to work correctly, you must implement your own class as follows:
//
//       #include "D3RowLevelPermissionBase.h"
//
//       namespace D3
//       {
//         class D3RowLevelPermission : public D3RowLevelPermissionBase
//         {
//           D3_CLASS_DECL(D3RowLevelPermission);
//
//           protected:
//             D3RowLevelPermission() {}
//
//           public:
//             ~D3RowLevelPermission() {}
//
//             // Insert your specialised member functions here...
//
//         };
//       } // end namespace D3
//

#include "Entity.h"
#include "Column.h"
#include "Key.h"
#include "Relation.h"
#include "D3MDDB.h"

namespace D3
{

	//! Use these enums to access columns through D3RowLevelPermission::Column() method
	enum D3RowLevelPermission_Fields
	{
		D3RowLevelPermission_UserID,
		D3RowLevelPermission_MetaEntityID,
		D3RowLevelPermission_AccessRights,
		D3RowLevelPermission_PKValueList
	};


	//! D3RowLevelPermissionBase is a base class that \b MUST be subclassed through a class called \a D3RowLevelPermission.
	/*! The purpose of this class to provide more natural access to related objects as well as this' columns.
			This class is only usefull if it is subclassed by a class called D3RowLevelPermission
			Equally important is that the meta dictionary knows of the existence of your subclass as well as
			specialised Relation classes implemented herein. Only once these details have been added to the
			dictionary will D3 instantiate objects of type \a D3RowLevelPermission representing rows from the table \a D3MDDB.D3RowLevelPermission.
	*/
	class D3_API D3RowLevelPermissionBase: public Entity
	{
		D3_CLASS_DECL(D3RowLevelPermissionBase);

		public:
			//! Enable iterating over all instances of this
			class D3_API iterator : public InstanceKeyPtrSetItr
			{
				public:
					iterator() {}
					iterator(const InstanceKeyPtrSetItr& itr) : InstanceKeyPtrSetItr(itr) {}

					//! De-reference operator*()
					virtual D3RowLevelPermissionPtr operator*();
					//! Assignment operator=()
					virtual iterator&               operator=(const iterator& itr);
			};

			static unsigned int                 size(DatabasePtr pDB)         { return GetAll(pDB)->size(); }
			static bool                         empty(DatabasePtr pDB)        { return GetAll(pDB)->empty(); }
			static iterator                     begin(DatabasePtr pDB)        { return iterator(GetAll(pDB)->begin()); }
			static iterator                     end(DatabasePtr pDB)          { return iterator(GetAll(pDB)->end()); }



		protected:
			D3RowLevelPermissionBase() {}

		public:
			~D3RowLevelPermissionBase() {}

			//! Create a new D3RowLevelPermission
			static D3RowLevelPermissionPtr      CreateD3RowLevelPermission(DatabasePtr pDB)		{ return (D3RowLevelPermissionPtr) pDB->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3RowLevelPermission)->CreateInstance(pDB); }

			//! Return a collection of all instances of this
			static InstanceKeyPtrSetPtr					GetAll(DatabasePtr pDB);

			//! Load all instances of this
			static void													LoadAll(DatabasePtr pDB, bool bRefresh = false, bool bLazyFetch = true);

			//! Load a particular instance of this
			static D3RowLevelPermissionPtr      Load(DatabasePtr pDB, long lUserID, long lMetaEntityID, bool bRefresh = false, bool bLazyFetch = true);

			//! Get related parent D3User object
			virtual D3UserPtr                   GetD3User();
			//! Set related parent D3User object
			virtual void												SetD3User(D3UserPtr pD3User);
			//! Get related parent D3MetaEntity object
			virtual D3MetaEntityPtr             GetD3MetaEntity();
			//! Set related parent D3MetaEntity object
			virtual void												SetD3MetaEntity(D3MetaEntityPtr pD3MetaEntity);


			/*! @name Get Column Values
			    \note These accessors do not throw even if the column's value is NULL.
			           Therefore, you should use these methods only if you're sure the
			           column's value is NOT NULL before using these.
			*/
			//@{
			//! UserID
			virtual long                        GetUserID()                    { return Column(D3RowLevelPermission_UserID)->GetLong(); }
			//! MetaEntityID
			virtual long                        GetMetaEntityID()              { return Column(D3RowLevelPermission_MetaEntityID)->GetLong(); }
			//! AccessRights
			virtual long                        GetAccessRights()              { return Column(D3RowLevelPermission_AccessRights)->GetLong(); }
			//! PKValueList
			virtual const std::string&          GetPKValueList()               { return Column(D3RowLevelPermission_PKValueList)->GetString(); }
			//@}

			/*! @name Set Column Values
			*/
			//@{
			//! Set UserID
			virtual bool												SetUserID(long val)            { return Column(D3RowLevelPermission_UserID)->SetValue(val); }
			//! Set MetaEntityID
			virtual bool												SetMetaEntityID(long val)      { return Column(D3RowLevelPermission_MetaEntityID)->SetValue(val); }
			//! Set AccessRights
			virtual bool												SetAccessRights(long val)      { return Column(D3RowLevelPermission_AccessRights)->SetValue(val); }
			//! Set PKValueList
			virtual bool												SetPKValueList(const std::string& val){ return Column(D3RowLevelPermission_PKValueList)->SetValue(val); }
			//@}

			//! A column accessor provided mainly for backwards compatibility
			virtual ColumnPtr										Column(D3RowLevelPermission_Fields eCol){ return Entity::GetColumn((unsigned int) eCol); }
	};



} // end namespace D3


#endif /* INC_D3RowLevelPermissionBase_H */
