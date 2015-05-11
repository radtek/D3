#ifndef INC_D3EntityPermissionBase_H
#define INC_D3EntityPermissionBase_H

// WARNING: This file has been generated by D3. You should NEVER change this file.
//          Please refer to the documentation of MetaEntity::CreateSpecialisedCPPHeader()
//          which explains in detail how you can gegenerate this file.
//
// The file declares methods which simplify client interactions with objects of type 
// D3EntityPermission representing instances of D3MDDB.D3EntityPermission.
//
//       For D3 to work correctly, you must implement your own class as follows:
//
//       #include "D3EntityPermissionBase.h"
//
//       namespace D3
//       {
//         class D3EntityPermission : public D3EntityPermissionBase
//         {
//           D3_CLASS_DECL(D3EntityPermission);
//
//           protected:
//             D3EntityPermission() {}
//
//           public:
//             ~D3EntityPermission() {}
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

	//! Use these enums to access columns through D3EntityPermission::Column() method
	enum D3EntityPermission_Fields
	{
		D3EntityPermission_RoleID,
		D3EntityPermission_MetaEntityID,
		D3EntityPermission_AccessRights
	};


	//! D3EntityPermissionBase is a base class that \b MUST be subclassed through a class called \a D3EntityPermission.
	/*! The purpose of this class to provide more natural access to related objects as well as this' columns.
			This class is only usefull if it is subclassed by a class called D3EntityPermission
			Equally important is that the meta dictionary knows of the existence of your subclass as well as
			specialised Relation classes implemented herein. Only once these details have been added to the
			dictionary will D3 instantiate objects of type \a D3EntityPermission representing rows from the table \a D3MDDB.D3EntityPermission.
	*/
	class D3_API D3EntityPermissionBase: public Entity
	{
		D3_CLASS_DECL(D3EntityPermissionBase);

		public:
			//! Enable iterating over all instances of this
			class D3_API iterator : public InstanceKeyPtrSetItr
			{
				public:
					iterator() {}
					iterator(const InstanceKeyPtrSetItr& itr) : InstanceKeyPtrSetItr(itr) {}

					//! De-reference operator*()
					virtual D3EntityPermissionPtr   operator*();
					//! Assignment operator=()
					virtual iterator&               operator=(const iterator& itr);
			};

			static unsigned int                 size(DatabasePtr pDB)         { return GetAll(pDB)->size(); }
			static bool                         empty(DatabasePtr pDB)        { return GetAll(pDB)->empty(); }
			static iterator                     begin(DatabasePtr pDB)        { return iterator(GetAll(pDB)->begin()); }
			static iterator                     end(DatabasePtr pDB)          { return iterator(GetAll(pDB)->end()); }



		protected:
			D3EntityPermissionBase() {}

		public:
			~D3EntityPermissionBase() {}

			//! Create a new D3EntityPermission
			static D3EntityPermissionPtr        CreateD3EntityPermission(DatabasePtr pDB)		{ return (D3EntityPermissionPtr) pDB->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3EntityPermission)->CreateInstance(pDB); }

			//! Return a collection of all instances of this
			static InstanceKeyPtrSetPtr					GetAll(DatabasePtr pDB);

			//! Load all instances of this
			static void													LoadAll(DatabasePtr pDB, bool bRefresh = false, bool bLazyFetch = true);

			//! Load a particular instance of this
			static D3EntityPermissionPtr        Load(DatabasePtr pDB, long lRoleID, long lMetaEntityID, bool bRefresh = false, bool bLazyFetch = true);

			//! Get related parent D3Role object
			virtual D3RolePtr                   GetD3Role();
			//! Set related parent D3Role object
			virtual void												SetD3Role(D3RolePtr pD3Role);
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
			//! RoleID
			virtual long                        GetRoleID()                    { return Column(D3EntityPermission_RoleID)->GetLong(); }
			//! MetaEntityID
			virtual long                        GetMetaEntityID()              { return Column(D3EntityPermission_MetaEntityID)->GetLong(); }
			//! AccessRights
			virtual long                        GetAccessRights()              { return Column(D3EntityPermission_AccessRights)->GetLong(); }
			//@}

			/*! @name Set Column Values
			*/
			//@{
			//! Set RoleID
			virtual bool												SetRoleID(long val)            { return Column(D3EntityPermission_RoleID)->SetValue(val); }
			//! Set MetaEntityID
			virtual bool												SetMetaEntityID(long val)      { return Column(D3EntityPermission_MetaEntityID)->SetValue(val); }
			//! Set AccessRights
			virtual bool												SetAccessRights(long val)      { return Column(D3EntityPermission_AccessRights)->SetValue(val); }
			//@}

			//! A column accessor provided mainly for backwards compatibility
			virtual ColumnPtr										Column(D3EntityPermission_Fields eCol){ return Entity::GetColumn((unsigned int) eCol); }
	};



} // end namespace D3


#endif /* INC_D3EntityPermissionBase_H */
