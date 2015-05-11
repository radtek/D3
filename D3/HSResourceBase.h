#ifndef INC_HSResourceBase_H
#define INC_HSResourceBase_H

// WARNING: This file has been generated by D3. You should NEVER change this file.
//          Please refer to the documentation of MetaEntity::CreateSpecialisedCPPHeader()
//          which explains in detail how you can gegenerate this file.
//
// The file declares methods which simplify client interactions with objects of type 
// HSResource representing instances of D3HSDB.HSResource.
//
//       For D3 to work correctly, you must implement your own class as follows:
//
//       #include "HSResourceBase.h"
//
//       namespace D3
//       {
//         class HSResource : public HSResourceBase
//         {
//           D3_CLASS_DECL(HSResource);
//
//           protected:
//             HSResource() {}
//
//           public:
//             ~HSResource() {}
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
#include "D3HSDB.h"

namespace D3
{

	//! Use these enums to access columns through HSResource::Column() method
	enum HSResource_Fields
	{
		HSResource_ID,
		HSResource_Name,
		HSResource_Description,
		HSResource_RawData,
		HSResource_Type
	};


	//! HSResourceBase is a base class that \b MUST be subclassed through a class called \a HSResource.
	/*! The purpose of this class to provide more natural access to related objects as well as this' columns.
			This class is only usefull if it is subclassed by a class called HSResource
			Equally important is that the meta dictionary knows of the existence of your subclass as well as
			specialised Relation classes implemented herein. Only once these details have been added to the
			dictionary will D3 instantiate objects of type \a HSResource representing rows from the table \a D3HSDB.HSResource.
	*/
	class D3_API HSResourceBase: public Entity
	{
		D3_CLASS_DECL(HSResourceBase);

		public:
			//! Enable iterating over all instances of this
			class D3_API iterator : public InstanceKeyPtrSetItr
			{
				public:
					iterator() {}
					iterator(const InstanceKeyPtrSetItr& itr) : InstanceKeyPtrSetItr(itr) {}

					//! De-reference operator*()
					virtual HSResourcePtr           operator*();
					//! Assignment operator=()
					virtual iterator&               operator=(const iterator& itr);
			};

			static unsigned int                 size(DatabasePtr pDB)         { return GetAll(pDB)->size(); }
			static bool                         empty(DatabasePtr pDB)        { return GetAll(pDB)->empty(); }
			static iterator                     begin(DatabasePtr pDB)        { return iterator(GetAll(pDB)->begin()); }
			static iterator                     end(DatabasePtr pDB)          { return iterator(GetAll(pDB)->end()); }

			//! Enable iterating the relation ResourceUsages to access related HSResourceUsage objects
			class D3_API ResourceUsages : public Relation
			{
				D3_CLASS_DECL(ResourceUsages);

				protected:
					ResourceUsages() {}

				public:
					class D3_API iterator : public Relation::iterator
					{
						public:
							iterator() {}
							iterator(const InstanceKeyPtrSetItr& itr) : Relation::iterator(itr) {}

							//! De-reference operator*()
							virtual HSResourceUsagePtr  operator*();
							//! Assignment operator=()
							virtual iterator&           operator=(const iterator& itr);
					};

					//! front() method
					virtual HSResourceUsagePtr      front();
					//! back() method
					virtual HSResourceUsagePtr      back();
			};



		protected:
			HSResourceBase() {}

		public:
			~HSResourceBase() {}

			//! Create a new HSResource
			static HSResourcePtr                CreateHSResource(DatabasePtr pDB)		{ return (HSResourcePtr) pDB->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSResource)->CreateInstance(pDB); }

			//! Return a collection of all instances of this
			static InstanceKeyPtrSetPtr					GetAll(DatabasePtr pDB);

			//! Load all instances of this
			static void													LoadAll(DatabasePtr pDB, bool bRefresh = false, bool bLazyFetch = true);

			//! Load a particular instance of this
			static HSResourcePtr                Load(DatabasePtr pDB, long lID, bool bRefresh = false, bool bLazyFetch = true);


			//! Load all ResourceUsages objects. The objects loaded are of type HSResourceUsage.
			virtual void												LoadAllResourceUsages(bool bRefresh = false, bool bLazyFetch = true);
			//! Get the relation ResourceUsages collection which contains objects of type HSResourceUsage.
			virtual ResourceUsages*             GetResourceUsages();

			/*! @name Get Column Values
			    \note These accessors do not throw even if the column's value is NULL.
			           Therefore, you should use these methods only if you're sure the
			           column's value is NOT NULL before using these.
			*/
			//@{
			//! ID
			virtual long                        GetID()                        { return Column(HSResource_ID)->GetLong(); }
			//! Name
			virtual const std::string&          GetName()                      { return Column(HSResource_Name)->GetString(); }
			//! Description
			virtual const std::string&          GetDescription()               { return Column(HSResource_Description)->GetString(); }
			//! RawData
			virtual const std::stringstream&    GetRawData()                   { return Column(HSResource_RawData)->GetBlob(); }
			//! Type
			virtual long                        GetType()                      { return Column(HSResource_Type)->GetLong(); }
			//@}

			/*! @name Set Column Values
			*/
			//@{
			//! Set ID
			virtual bool												SetID(long val)                { return Column(HSResource_ID)->SetValue(val); }
			//! Set Name
			virtual bool												SetName(const std::string& val){ return Column(HSResource_Name)->SetValue(val); }
			//! Set Description
			virtual bool												SetDescription(const std::string& val){ return Column(HSResource_Description)->SetValue(val); }
			//! Set RawData
			virtual bool												SetRawData(const std::stringstream& val){ return Column(HSResource_RawData)->SetValue(val); }
			//! Set Type
			virtual bool												SetType(long val)              { return Column(HSResource_Type)->SetValue(val); }
			//@}

			//! A column accessor provided mainly for backwards compatibility
			virtual ColumnPtr										Column(HSResource_Fields eCol){ return Entity::GetColumn((unsigned int) eCol); }
	};



} // end namespace D3


#endif /* INC_HSResourceBase_H */