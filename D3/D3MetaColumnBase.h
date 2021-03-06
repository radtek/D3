#ifndef INC_D3MetaColumnBase_H
#define INC_D3MetaColumnBase_H

// WARNING: This file has been generated by D3. You should NEVER change this file.
//          Please refer to the documentation of MetaEntity::CreateSpecialisedCPPHeader()
//          which explains in detail how you can gegenerate this file.
//
// The file declares methods which simplify client interactions with objects of type 
// D3MetaColumn representing instances of D3MDDB.D3MetaColumn.
//
//       For D3 to work correctly, you must implement your own class as follows:
//
//       #include "D3MetaColumnBase.h"
//
//       namespace D3
//       {
//         class D3MetaColumn : public D3MetaColumnBase
//         {
//           D3_CLASS_DECL(D3MetaColumn);
//
//           protected:
//             D3MetaColumn() {}
//
//           public:
//             ~D3MetaColumn() {}
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

	//! Use these enums to access columns through D3MetaColumn::Column() method
	enum D3MetaColumn_Fields
	{
		D3MetaColumn_ID,
		D3MetaColumn_MetaEntityID,
		D3MetaColumn_SequenceNo,
		D3MetaColumn_Name,
		D3MetaColumn_ProsaName,
		D3MetaColumn_Description,
		D3MetaColumn_Type,
		D3MetaColumn_Unit,
		D3MetaColumn_FloatPrecision,
		D3MetaColumn_MaxLength,
		D3MetaColumn_Flags,
		D3MetaColumn_AccessRights,
		D3MetaColumn_MetaClass,
		D3MetaColumn_InstanceClass,
		D3MetaColumn_InstanceInterface,
		D3MetaColumn_JSMetaClass,
		D3MetaColumn_JSInstanceClass,
		D3MetaColumn_JSViewClass,
		D3MetaColumn_DefaultValue
	};


	//! D3MetaColumnBase is a base class that \b MUST be subclassed through a class called \a D3MetaColumn.
	/*! The purpose of this class to provide more natural access to related objects as well as this' columns.
			This class is only usefull if it is subclassed by a class called D3MetaColumn
			Equally important is that the meta dictionary knows of the existence of your subclass as well as
			specialised Relation classes implemented herein. Only once these details have been added to the
			dictionary will D3 instantiate objects of type \a D3MetaColumn representing rows from the table \a D3MDDB.D3MetaColumn.
	*/
	class D3_API D3MetaColumnBase: public Entity
	{
		D3_CLASS_DECL(D3MetaColumnBase);

		public:
			//! Enable iterating over all instances of this
			class D3_API iterator : public InstanceKeyPtrSetItr
			{
				public:
					iterator() {}
					iterator(const InstanceKeyPtrSetItr& itr) : InstanceKeyPtrSetItr(itr) {}

					//! De-reference operator*()
					virtual D3MetaColumnPtr         operator*();
					//! Assignment operator=()
					virtual iterator&               operator=(const iterator& itr);
			};

			static unsigned int                 size(DatabasePtr pDB)         { return GetAll(pDB)->size(); }
			static bool                         empty(DatabasePtr pDB)        { return GetAll(pDB)->empty(); }
			static iterator                     begin(DatabasePtr pDB)        { return iterator(GetAll(pDB)->begin()); }
			static iterator                     end(DatabasePtr pDB)          { return iterator(GetAll(pDB)->end()); }

			//! Enable iterating the relation D3MetaKeyColumns to access related D3MetaKeyColumn objects
			class D3_API D3MetaKeyColumns : public Relation
			{
				D3_CLASS_DECL(D3MetaKeyColumns);

				protected:
					D3MetaKeyColumns() {}

				public:
					class D3_API iterator : public Relation::iterator
					{
						public:
							iterator() {}
							iterator(const InstanceKeyPtrSetItr& itr) : Relation::iterator(itr) {}

							//! De-reference operator*()
							virtual D3MetaKeyColumnPtr  operator*();
							//! Assignment operator=()
							virtual iterator&           operator=(const iterator& itr);
					};

					//! front() method
					virtual D3MetaKeyColumnPtr      front();
					//! back() method
					virtual D3MetaKeyColumnPtr      back();
			};

			//! Enable iterating the relation D3MetaColumnChoices to access related D3MetaColumnChoice objects
			class D3_API D3MetaColumnChoices : public Relation
			{
				D3_CLASS_DECL(D3MetaColumnChoices);

				protected:
					D3MetaColumnChoices() {}

				public:
					class D3_API iterator : public Relation::iterator
					{
						public:
							iterator() {}
							iterator(const InstanceKeyPtrSetItr& itr) : Relation::iterator(itr) {}

							//! De-reference operator*()
							virtual D3MetaColumnChoicePtr operator*();
							//! Assignment operator=()
							virtual iterator&           operator=(const iterator& itr);
					};

					//! front() method
					virtual D3MetaColumnChoicePtr   front();
					//! back() method
					virtual D3MetaColumnChoicePtr   back();
			};

			//! Enable iterating the relation D3MetaRelationSwitches to access related D3MetaRelation objects
			class D3_API D3MetaRelationSwitches : public Relation
			{
				D3_CLASS_DECL(D3MetaRelationSwitches);

				protected:
					D3MetaRelationSwitches() {}

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

			//! Enable iterating the relation D3RolePermissions to access related D3ColumnPermission objects
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
							virtual D3ColumnPermissionPtr operator*();
							//! Assignment operator=()
							virtual iterator&           operator=(const iterator& itr);
					};

					//! front() method
					virtual D3ColumnPermissionPtr   front();
					//! back() method
					virtual D3ColumnPermissionPtr   back();
			};



		protected:
			D3MetaColumnBase() {}

		public:
			~D3MetaColumnBase() {}

			//! Create a new D3MetaColumn
			static D3MetaColumnPtr              CreateD3MetaColumn(DatabasePtr pDB)		{ return (D3MetaColumnPtr) pDB->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaColumn)->CreateInstance(pDB); }

			//! Return a collection of all instances of this
			static InstanceKeyPtrSetPtr					GetAll(DatabasePtr pDB);

			//! Load all instances of this
			static void													LoadAll(DatabasePtr pDB, bool bRefresh = false, bool bLazyFetch = true);

			//! Load a particular instance of this
			static D3MetaColumnPtr              Load(DatabasePtr pDB, long lID, bool bRefresh = false, bool bLazyFetch = true);

			//! Get related parent D3MetaEntity object
			virtual D3MetaEntityPtr             GetD3MetaEntity();
			//! Set related parent D3MetaEntity object
			virtual void												SetD3MetaEntity(D3MetaEntityPtr pD3MetaEntity);

			//! Load all D3MetaKeyColumns objects. The objects loaded are of type D3MetaKeyColumn.
			virtual void												LoadAllD3MetaKeyColumns(bool bRefresh = false, bool bLazyFetch = true);
			//! Get the relation D3MetaKeyColumns collection which contains objects of type D3MetaKeyColumn.
			virtual D3MetaKeyColumns*           GetD3MetaKeyColumns();
			//! Load all D3MetaColumnChoices objects. The objects loaded are of type D3MetaColumnChoice.
			virtual void												LoadAllD3MetaColumnChoices(bool bRefresh = false, bool bLazyFetch = true);
			//! Get the relation D3MetaColumnChoices collection which contains objects of type D3MetaColumnChoice.
			virtual D3MetaColumnChoices*        GetD3MetaColumnChoices();
			//! Load all D3MetaRelationSwitches objects. The objects loaded are of type D3MetaRelation.
			virtual void												LoadAllD3MetaRelationSwitches(bool bRefresh = false, bool bLazyFetch = true);
			//! Get the relation D3MetaRelationSwitches collection which contains objects of type D3MetaRelation.
			virtual D3MetaRelationSwitches*     GetD3MetaRelationSwitches();
			//! Load all D3RolePermissions objects. The objects loaded are of type D3ColumnPermission.
			virtual void												LoadAllD3RolePermissions(bool bRefresh = false, bool bLazyFetch = true);
			//! Get the relation D3RolePermissions collection which contains objects of type D3ColumnPermission.
			virtual D3RolePermissions*          GetD3RolePermissions();

			/*! @name Get Column Values
			    \note These accessors do not throw even if the column's value is NULL.
			           Therefore, you should use these methods only if you're sure the
			           column's value is NOT NULL before using these.
			*/
			//@{
			//! ID
			virtual long                        GetID()                        { return Column(D3MetaColumn_ID)->GetLong(); }
			//! MetaEntityID
			virtual long                        GetMetaEntityID()              { return Column(D3MetaColumn_MetaEntityID)->GetLong(); }
			//! SequenceNo
			virtual float                       GetSequenceNo()                { return Column(D3MetaColumn_SequenceNo)->GetFloat(); }
			//! Name
			virtual const std::string&          GetName()                      { return Column(D3MetaColumn_Name)->GetString(); }
			//! ProsaName
			virtual const std::string&          GetProsaName()                 { return Column(D3MetaColumn_ProsaName)->GetString(); }
			//! Description
			virtual const std::string&          GetDescription()               { return Column(D3MetaColumn_Description)->GetString(); }
			//! Type
			virtual char                        GetType()                      { return Column(D3MetaColumn_Type)->GetChar(); }
			//! Unit
			virtual char                        GetUnit()                      { return Column(D3MetaColumn_Unit)->GetChar(); }
			//! FloatPrecision
			virtual short                       GetFloatPrecision()            { return Column(D3MetaColumn_FloatPrecision)->GetShort(); }
			//! MaxLength
			virtual int                         GetMaxLength()                 { return Column(D3MetaColumn_MaxLength)->GetInt(); }
			//! Flags
			virtual long                        GetFlags()                     { return Column(D3MetaColumn_Flags)->GetLong(); }
			//! AccessRights
			virtual long                        GetAccessRights()              { return Column(D3MetaColumn_AccessRights)->GetLong(); }
			//! MetaClass
			virtual const std::string&          GetMetaClass()                 { return Column(D3MetaColumn_MetaClass)->GetString(); }
			//! InstanceClass
			virtual const std::string&          GetInstanceClass()             { return Column(D3MetaColumn_InstanceClass)->GetString(); }
			//! InstanceInterface
			virtual const std::string&          GetInstanceInterface()         { return Column(D3MetaColumn_InstanceInterface)->GetString(); }
			//! JSMetaClass
			virtual const std::string&          GetJSMetaClass()               { return Column(D3MetaColumn_JSMetaClass)->GetString(); }
			//! JSInstanceClass
			virtual const std::string&          GetJSInstanceClass()           { return Column(D3MetaColumn_JSInstanceClass)->GetString(); }
			//! JSViewClass
			virtual const std::string&          GetJSViewClass()               { return Column(D3MetaColumn_JSViewClass)->GetString(); }
			//! DefaultValue
			virtual const std::string&          GetDefaultValue()              { return Column(D3MetaColumn_DefaultValue)->GetString(); }
			//@}

			/*! @name Set Column Values
			*/
			//@{
			//! Set ID
			virtual bool												SetID(long val)                { return Column(D3MetaColumn_ID)->SetValue(val); }
			//! Set MetaEntityID
			virtual bool												SetMetaEntityID(long val)      { return Column(D3MetaColumn_MetaEntityID)->SetValue(val); }
			//! Set SequenceNo
			virtual bool												SetSequenceNo(float val)       { return Column(D3MetaColumn_SequenceNo)->SetValue(val); }
			//! Set Name
			virtual bool												SetName(const std::string& val){ return Column(D3MetaColumn_Name)->SetValue(val); }
			//! Set ProsaName
			virtual bool												SetProsaName(const std::string& val){ return Column(D3MetaColumn_ProsaName)->SetValue(val); }
			//! Set Description
			virtual bool												SetDescription(const std::string& val){ return Column(D3MetaColumn_Description)->SetValue(val); }
			//! Set Type
			virtual bool												SetType(char val)              { return Column(D3MetaColumn_Type)->SetValue(val); }
			//! Set Unit
			virtual bool												SetUnit(char val)              { return Column(D3MetaColumn_Unit)->SetValue(val); }
			//! Set FloatPrecision
			virtual bool												SetFloatPrecision(short val)   { return Column(D3MetaColumn_FloatPrecision)->SetValue(val); }
			//! Set MaxLength
			virtual bool												SetMaxLength(int val)          { return Column(D3MetaColumn_MaxLength)->SetValue(val); }
			//! Set Flags
			virtual bool												SetFlags(long val)             { return Column(D3MetaColumn_Flags)->SetValue(val); }
			//! Set AccessRights
			virtual bool												SetAccessRights(long val)      { return Column(D3MetaColumn_AccessRights)->SetValue(val); }
			//! Set MetaClass
			virtual bool												SetMetaClass(const std::string& val){ return Column(D3MetaColumn_MetaClass)->SetValue(val); }
			//! Set InstanceClass
			virtual bool												SetInstanceClass(const std::string& val){ return Column(D3MetaColumn_InstanceClass)->SetValue(val); }
			//! Set InstanceInterface
			virtual bool												SetInstanceInterface(const std::string& val){ return Column(D3MetaColumn_InstanceInterface)->SetValue(val); }
			//! Set JSMetaClass
			virtual bool												SetJSMetaClass(const std::string& val){ return Column(D3MetaColumn_JSMetaClass)->SetValue(val); }
			//! Set JSInstanceClass
			virtual bool												SetJSInstanceClass(const std::string& val){ return Column(D3MetaColumn_JSInstanceClass)->SetValue(val); }
			//! Set JSViewClass
			virtual bool												SetJSViewClass(const std::string& val){ return Column(D3MetaColumn_JSViewClass)->SetValue(val); }
			//! Set DefaultValue
			virtual bool												SetDefaultValue(const std::string& val){ return Column(D3MetaColumn_DefaultValue)->SetValue(val); }
			//@}

			//! A column accessor provided mainly for backwards compatibility
			virtual ColumnPtr										Column(D3MetaColumn_Fields eCol){ return Entity::GetColumn((unsigned int) eCol); }
	};



} // end namespace D3


#endif /* INC_D3MetaColumnBase_H */
