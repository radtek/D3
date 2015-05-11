#ifndef INC_HSResourceUsageBase_H
#define INC_HSResourceUsageBase_H

// WARNING: This file has been generated by D3. You should NEVER change this file.
//          Please refer to the documentation of MetaEntity::CreateSpecialisedCPPHeader()
//          which explains in detail how you can gegenerate this file.
//
// The file declares methods which simplify client interactions with objects of type 
// HSResourceUsage representing instances of D3HSDB.HSResourceUsage.
//
//       For D3 to work correctly, you must implement your own class as follows:
//
//       #include "HSResourceUsageBase.h"
//
//       namespace D3
//       {
//         class HSResourceUsage : public HSResourceUsageBase
//         {
//           D3_CLASS_DECL(HSResourceUsage);
//
//           protected:
//             HSResourceUsage() {}
//
//           public:
//             ~HSResourceUsage() {}
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

	//! Use these enums to access columns through HSResourceUsage::Column() method
	enum HSResourceUsage_Fields
	{
		HSResourceUsage_TopicID,
		HSResourceUsage_ResourceID
	};


	//! HSResourceUsageBase is a base class that \b MUST be subclassed through a class called \a HSResourceUsage.
	/*! The purpose of this class to provide more natural access to related objects as well as this' columns.
			This class is only usefull if it is subclassed by a class called HSResourceUsage
			Equally important is that the meta dictionary knows of the existence of your subclass as well as
			specialised Relation classes implemented herein. Only once these details have been added to the
			dictionary will D3 instantiate objects of type \a HSResourceUsage representing rows from the table \a D3HSDB.HSResourceUsage.
	*/
	class D3_API HSResourceUsageBase: public Entity
	{
		D3_CLASS_DECL(HSResourceUsageBase);

		public:
			//! Enable iterating over all instances of this
			class D3_API iterator : public InstanceKeyPtrSetItr
			{
				public:
					iterator() {}
					iterator(const InstanceKeyPtrSetItr& itr) : InstanceKeyPtrSetItr(itr) {}

					//! De-reference operator*()
					virtual HSResourceUsagePtr      operator*();
					//! Assignment operator=()
					virtual iterator&               operator=(const iterator& itr);
			};

			static unsigned int                 size(DatabasePtr pDB)         { return GetAll(pDB)->size(); }
			static bool                         empty(DatabasePtr pDB)        { return GetAll(pDB)->empty(); }
			static iterator                     begin(DatabasePtr pDB)        { return iterator(GetAll(pDB)->begin()); }
			static iterator                     end(DatabasePtr pDB)          { return iterator(GetAll(pDB)->end()); }



		protected:
			HSResourceUsageBase() {}

		public:
			~HSResourceUsageBase() {}

			//! Create a new HSResourceUsage
			static HSResourceUsagePtr           CreateHSResourceUsage(DatabasePtr pDB)		{ return (HSResourceUsagePtr) pDB->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSResourceUsage)->CreateInstance(pDB); }

			//! Return a collection of all instances of this
			static InstanceKeyPtrSetPtr					GetAll(DatabasePtr pDB);

			//! Load all instances of this
			static void													LoadAll(DatabasePtr pDB, bool bRefresh = false, bool bLazyFetch = true);

			//! Load a particular instance of this
			static HSResourceUsagePtr           Load(DatabasePtr pDB, long lTopicID, long lResourceID, bool bRefresh = false, bool bLazyFetch = true);

			//! Get related parent HSTopic object
			virtual HSTopicPtr                  GetTopic();
			//! Set related parent HSTopic object
			virtual void												SetTopic(HSTopicPtr pHSTopic);
			//! Get related parent HSResource object
			virtual HSResourcePtr               GetResource();
			//! Set related parent HSResource object
			virtual void												SetResource(HSResourcePtr pHSResource);


			/*! @name Get Column Values
			    \note These accessors do not throw even if the column's value is NULL.
			           Therefore, you should use these methods only if you're sure the
			           column's value is NOT NULL before using these.
			*/
			//@{
			//! TopicID
			virtual long                        GetTopicID()                   { return Column(HSResourceUsage_TopicID)->GetLong(); }
			//! ResourceID
			virtual long                        GetResourceID()                { return Column(HSResourceUsage_ResourceID)->GetLong(); }
			//@}

			/*! @name Set Column Values
			*/
			//@{
			//! Set TopicID
			virtual bool												SetTopicID(long val)           { return Column(HSResourceUsage_TopicID)->SetValue(val); }
			//! Set ResourceID
			virtual bool												SetResourceID(long val)        { return Column(HSResourceUsage_ResourceID)->SetValue(val); }
			//@}

			//! A column accessor provided mainly for backwards compatibility
			virtual ColumnPtr										Column(HSResourceUsage_Fields eCol){ return Entity::GetColumn((unsigned int) eCol); }
	};



} // end namespace D3


#endif /* INC_HSResourceUsageBase_H */
