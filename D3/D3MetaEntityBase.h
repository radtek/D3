#ifndef INC_D3MetaEntityBase_H
#define INC_D3MetaEntityBase_H

// WARNING: This file has been generated by D3. You should NEVER change this file.
//          Please refer to the documentation of MetaEntity::CreateSpecialisedCPPHeader()
//          which explains in detail how you can gegenerate this file.
//
// The file declares methods which simplify client interactions with objects of type 
// D3MetaEntity representing instances of D3MDDB.D3MetaEntity.
//
//       For D3 to work correctly, you must implement your own class as follows:
//
//       #include "D3MetaEntityBase.h"
//
//       namespace D3
//       {
//         class D3MetaEntity : public D3MetaEntityBase
//         {
//           D3_CLASS_DECL(D3MetaEntity);
//
//           protected:
//             D3MetaEntity() {}
//
//           public:
//             ~D3MetaEntity() {}
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

	//! Use these enums to access columns through D3MetaEntity::Column() method
	enum D3MetaEntity_Fields
	{
		D3MetaEntity_ID,
		D3MetaEntity_MetaDatabaseID,
		D3MetaEntity_SequenceNo,
		D3MetaEntity_Name,
		D3MetaEntity_MetaClass,
		D3MetaEntity_InstanceClass,
		D3MetaEntity_InstanceInterface,
		D3MetaEntity_JSMetaClass,
		D3MetaEntity_JSInstanceClass,
		D3MetaEntity_JSViewClass,
		D3MetaEntity_JSDetailViewClass,
		D3MetaEntity_ProsaName,
		D3MetaEntity_Description,
		D3MetaEntity_Alias,
		D3MetaEntity_Flags,
		D3MetaEntity_AccessRights
	};


	//! D3MetaEntityBase is a base class that \b MUST be subclassed through a class called \a D3MetaEntity.
	/*! The purpose of this class to provide more natural access to related objects as well as this' columns.
			This class is only usefull if it is subclassed by a class called D3MetaEntity
			Equally important is that the meta dictionary knows of the existence of your subclass as well as
			specialised Relation classes implemented herein. Only once these details have been added to the
			dictionary will D3 instantiate objects of type \a D3MetaEntity representing rows from the table \a D3MDDB.D3MetaEntity.
	*/
	class D3_API D3MetaEntityBase: public Entity
	{
		D3_CLASS_DECL(D3MetaEntityBase);

		public:
			//! Enable iterating over all instances of this
			class D3_API iterator : public InstanceKeyPtrSetItr
			{
				public:
					iterator() {}
					iterator(const InstanceKeyPtrSetItr& itr) : InstanceKeyPtrSetItr(itr) {}

					//! De-reference operator*()
					virtual D3MetaEntityPtr         operator*();
					//! Assignment operator=()
					virtual iterator&               operator=(const iterator& itr);
			};

			static unsigned int                 size(DatabasePtr pDB)         { return GetAll(pDB)->size(); }
			static bool                         empty(DatabasePtr pDB)        { return GetAll(pDB)->empty(); }
			static iterator                     begin(DatabasePtr pDB)        { return iterator(GetAll(pDB)->begin()); }
			static iterator                     end(DatabasePtr pDB)          { return iterator(GetAll(pDB)->end()); }

			//! Enable iterating the relation D3MetaColumns to access related D3MetaColumn objects
			class D3_API D3MetaColumns : public Relation
			{
				D3_CLASS_DECL(D3MetaColumns);

				protected:
					D3MetaColumns() {}

				public:
					class D3_API iterator : public Relation::iterator
					{
						public:
							iterator() {}
							iterator(const InstanceKeyPtrSetItr& itr) : Relation::iterator(itr) {}

							//! De-reference operator*()
							virtual D3MetaColumnPtr     operator*();
							//! Assignment operator=()
							virtual iterator&           operator=(const iterator& itr);
					};

					//! front() method
					virtual D3MetaColumnPtr         front();
					//! back() method
					virtual D3MetaColumnPtr         back();
			};

			//! Enable iterating the relation D3MetaKeys to access related D3MetaKey objects
			class D3_API D3MetaKeys : public Relation
			{
				D3_CLASS_DECL(D3MetaKeys);

				protected:
					D3MetaKeys() {}

				public:
					class D3_API iterator : public Relation::iterator
					{
						public:
							iterator() {}
							iterator(const InstanceKeyPtrSetItr& itr) : Relation::iterator(itr) {}

							//! De-reference operator*()
							virtual D3MetaKeyPtr        operator*();
							//! Assignment operator=()
							virtual iterator&           operator=(const iterator& itr);
					};

					//! front() method
					virtual D3MetaKeyPtr            front();
					//! back() method
					virtual D3MetaKeyPtr            back();
			};

			//! Enable iterating the relation ChildD3MetaRelations to access related D3MetaRelation objects
			class D3_API ChildD3MetaRelations : public Relation
			{
				D3_CLASS_DECL(ChildD3MetaRelations);

				protected:
					ChildD3MetaRelations() {}

				public:
					class D3_API iterator : public Relation::iterator
					{
						public:
							iterator() {}
							iterator(const InstanceKeyPtrSetItr& itr) : Relation::iterator(itr) {}

							//! De-reference operator*()
							virtual D3MetaRelationPtr   operator*();
							//! Assignment operator=()
							virtual iterator&           operator=(const iterator& itr);
					};

					//! front() method
					virtual D3MetaRelationPtr       front();
					//! back() method
					virtual D3MetaRelationPtr       back();
			};

			//! Enable iterating the relation D3RolePermissions to access related D3EntityPermission objects
			class D3_API D3RolePermissions : public Relation
			{
				D3_CLASS_DECL(D3RolePermissions);

				protected:
					D3RolePermissions() {}

				public:
					class D3_API iterator : public Relation::iterator
					{
						public:
							iterator() {}
							iterator(const InstanceKeyPtrSetItr& itr) : Relation::iterator(itr) {}

							//! De-reference operator*()
							virtual D3EntityPermissionPtr operator*();
							//! Assignment operator=()
							virtual iterator&           operator=(const iterator& itr);
					};

					//! front() method
					virtual D3EntityPermissionPtr   front();
					//! back() method
					virtual D3EntityPermissionPtr   back();
			};

			//! Enable iterating the relation D3RowLevelPermissions to access related D3RowLevelPermission objects
			class D3_API D3RowLevelPermissions : public Relation
			{
				D3_CLASS_DECL(D3RowLevelPermissions);

				protected:
					D3RowLevelPermissions() {}

				public:
					class D3_API iterator : public Relation::iterator
					{
						public:
							iterator() {}
							iterator(const InstanceKeyPtrSetItr& itr) : Relation::iterator(itr) {}

							//! De-reference operator*()
							virtual D3RowLevelPermissionPtr operator*();
							//! Assignment operator=()
							virtual iterator&           operator=(const iterator& itr);
					};

					//! front() method
					virtual D3RowLevelPermissionPtr front();
					//! back() method
					virtual D3RowLevelPermissionPtr back();
			};



		protected:
			D3MetaEntityBase() {}

		public:
			~D3MetaEntityBase() {}

			//! Create a new D3MetaEntity
			static D3MetaEntityPtr              CreateD3MetaEntity(DatabasePtr pDB)		{ return (D3MetaEntityPtr) pDB->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaEntity)->CreateInstance(pDB); }

			//! Return a collection of all instances of this
			static InstanceKeyPtrSetPtr					GetAll(DatabasePtr pDB);

			//! Load all instances of this
			static void													LoadAll(DatabasePtr pDB, bool bRefresh = false, bool bLazyFetch = true);

			//! Load a particular instance of this
			static D3MetaEntityPtr              Load(DatabasePtr pDB, long lID, bool bRefresh = false, bool bLazyFetch = true);

			//! Get related parent D3MetaDatabase object
			virtual D3MetaDatabasePtr           GetD3MetaDatabase();
			//! Set related parent D3MetaDatabase object
			virtual void												SetD3MetaDatabase(D3MetaDatabasePtr pD3MetaDatabase);

			//! Load all D3MetaColumns objects. The objects loaded are of type D3MetaColumn.
			virtual void												LoadAllD3MetaColumns(bool bRefresh = false, bool bLazyFetch = true);
			//! Get the relation D3MetaColumns collection which contains objects of type D3MetaColumn.
			virtual D3MetaColumns*              GetD3MetaColumns();
			//! Load all D3MetaKeys objects. The objects loaded are of type D3MetaKey.
			virtual void												LoadAllD3MetaKeys(bool bRefresh = false, bool bLazyFetch = true);
			//! Get the relation D3MetaKeys collection which contains objects of type D3MetaKey.
			virtual D3MetaKeys*                 GetD3MetaKeys();
			//! Load all ChildD3MetaRelations objects. The objects loaded are of type D3MetaRelation.
			virtual void												LoadAllChildD3MetaRelations(bool bRefresh = false, bool bLazyFetch = true);
			//! Get the relation ChildD3MetaRelations collection which contains objects of type D3MetaRelation.
			virtual ChildD3MetaRelations*       GetChildD3MetaRelations();
			//! Load all D3RolePermissions objects. The objects loaded are of type D3EntityPermission.
			virtual void												LoadAllD3RolePermissions(bool bRefresh = false, bool bLazyFetch = true);
			//! Get the relation D3RolePermissions collection which contains objects of type D3EntityPermission.
			virtual D3RolePermissions*          GetD3RolePermissions();
			//! Load all D3RowLevelPermissions objects. The objects loaded are of type D3RowLevelPermission.
			virtual void												LoadAllD3RowLevelPermissions(bool bRefresh = false, bool bLazyFetch = true);
			//! Get the relation D3RowLevelPermissions collection which contains objects of type D3RowLevelPermission.
			virtual D3RowLevelPermissions*      GetD3RowLevelPermissions();

			/*! @name Get Column Values
			    \note These accessors do not throw even if the column's value is NULL.
			           Therefore, you should use these methods only if you're sure the
			           column's value is NOT NULL before using these.
			*/
			//@{
			//! ID
			virtual long                        GetID()                        { return Column(D3MetaEntity_ID)->GetLong(); }
			//! MetaDatabaseID
			virtual long                        GetMetaDatabaseID()            { return Column(D3MetaEntity_MetaDatabaseID)->GetLong(); }
			//! SequenceNo
			virtual float                       GetSequenceNo()                { return Column(D3MetaEntity_SequenceNo)->GetFloat(); }
			//! Name
			virtual const std::string&          GetName()                      { return Column(D3MetaEntity_Name)->GetString(); }
			//! MetaClass
			virtual const std::string&          GetMetaClass()                 { return Column(D3MetaEntity_MetaClass)->GetString(); }
			//! InstanceClass
			virtual const std::string&          GetInstanceClass()             { return Column(D3MetaEntity_InstanceClass)->GetString(); }
			//! InstanceInterface
			virtual const std::string&          GetInstanceInterface()         { return Column(D3MetaEntity_InstanceInterface)->GetString(); }
			//! JSMetaClass
			virtual const std::string&          GetJSMetaClass()               { return Column(D3MetaEntity_JSMetaClass)->GetString(); }
			//! JSInstanceClass
			virtual const std::string&          GetJSInstanceClass()           { return Column(D3MetaEntity_JSInstanceClass)->GetString(); }
			//! JSViewClass
			virtual const std::string&          GetJSViewClass()               { return Column(D3MetaEntity_JSViewClass)->GetString(); }
			//! JSDetailViewClass
			virtual const std::string&          GetJSDetailViewClass()         { return Column(D3MetaEntity_JSDetailViewClass)->GetString(); }
			//! ProsaName
			virtual const std::string&          GetProsaName()                 { return Column(D3MetaEntity_ProsaName)->GetString(); }
			//! Description
			virtual const std::string&          GetDescription()               { return Column(D3MetaEntity_Description)->GetString(); }
			//! Alias
			virtual const std::string&          GetAlias()                     { return Column(D3MetaEntity_Alias)->GetString(); }
			//! Flags
			virtual long                        GetFlags()                     { return Column(D3MetaEntity_Flags)->GetLong(); }
			//! AccessRights
			virtual long                        GetAccessRights()              { return Column(D3MetaEntity_AccessRights)->GetLong(); }
			//@}

			/*! @name Set Column Values
			*/
			//@{
			//! Set ID
			virtual bool												SetID(long val)                { return Column(D3MetaEntity_ID)->SetValue(val); }
			//! Set MetaDatabaseID
			virtual bool												SetMetaDatabaseID(long val)    { return Column(D3MetaEntity_MetaDatabaseID)->SetValue(val); }
			//! Set SequenceNo
			virtual bool												SetSequenceNo(float val)       { return Column(D3MetaEntity_SequenceNo)->SetValue(val); }
			//! Set Name
			virtual bool												SetName(const std::string& val){ return Column(D3MetaEntity_Name)->SetValue(val); }
			//! Set MetaClass
			virtual bool												SetMetaClass(const std::string& val){ return Column(D3MetaEntity_MetaClass)->SetValue(val); }
			//! Set InstanceClass
			virtual bool												SetInstanceClass(const std::string& val){ return Column(D3MetaEntity_InstanceClass)->SetValue(val); }
			//! Set InstanceInterface
			virtual bool												SetInstanceInterface(const std::string& val){ return Column(D3MetaEntity_InstanceInterface)->SetValue(val); }
			//! Set JSMetaClass
			virtual bool												SetJSMetaClass(const std::string& val){ return Column(D3MetaEntity_JSMetaClass)->SetValue(val); }
			//! Set JSInstanceClass
			virtual bool												SetJSInstanceClass(const std::string& val){ return Column(D3MetaEntity_JSInstanceClass)->SetValue(val); }
			//! Set JSViewClass
			virtual bool												SetJSViewClass(const std::string& val){ return Column(D3MetaEntity_JSViewClass)->SetValue(val); }
			//! Set JSDetailViewClass
			virtual bool												SetJSDetailViewClass(const std::string& val){ return Column(D3MetaEntity_JSDetailViewClass)->SetValue(val); }
			//! Set ProsaName
			virtual bool												SetProsaName(const std::string& val){ return Column(D3MetaEntity_ProsaName)->SetValue(val); }
			//! Set Description
			virtual bool												SetDescription(const std::string& val){ return Column(D3MetaEntity_Description)->SetValue(val); }
			//! Set Alias
			virtual bool												SetAlias(const std::string& val){ return Column(D3MetaEntity_Alias)->SetValue(val); }
			//! Set Flags
			virtual bool												SetFlags(long val)             { return Column(D3MetaEntity_Flags)->SetValue(val); }
			//! Set AccessRights
			virtual bool												SetAccessRights(long val)      { return Column(D3MetaEntity_AccessRights)->SetValue(val); }
			//@}

			//! A column accessor provided mainly for backwards compatibility
			virtual ColumnPtr										Column(D3MetaEntity_Fields eCol){ return Entity::GetColumn((unsigned int) eCol); }
	};



} // end namespace D3


#endif /* INC_D3MetaEntityBase_H */