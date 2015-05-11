#ifndef INC_HSMetaEntityTopicBase_H
#define INC_HSMetaEntityTopicBase_H

// WARNING: This file has been generated by D3. You should NEVER change this file.
//          Please refer to the documentation of MetaEntity::CreateSpecialisedCPPHeader()
//          which explains in detail how you can gegenerate this file.
//
// The file declares methods which simplify client interactions with objects of type 
// HSMetaEntityTopic representing instances of D3HSDB.HSMetaEntityTopic.
//
//       For D3 to work correctly, you must implement your own class as follows:
//
//       #include "HSMetaEntityTopicBase.h"
//
//       namespace D3
//       {
//         class HSMetaEntityTopic : public HSMetaEntityTopicBase
//         {
//           D3_CLASS_DECL(HSMetaEntityTopic);
//
//           protected:
//             HSMetaEntityTopic() {}
//
//           public:
//             ~HSMetaEntityTopic() {}
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

	//! Use these enums to access columns through HSMetaEntityTopic::Column() method
	enum HSMetaEntityTopic_Fields
	{
		HSMetaEntityTopic_TopicID,
		HSMetaEntityTopic_MetaDatabaseAlias,
		HSMetaEntityTopic_MetaEntityName
	};


	//! HSMetaEntityTopicBase is a base class that \b MUST be subclassed through a class called \a HSMetaEntityTopic.
	/*! The purpose of this class to provide more natural access to related objects as well as this' columns.
			This class is only usefull if it is subclassed by a class called HSMetaEntityTopic
			Equally important is that the meta dictionary knows of the existence of your subclass as well as
			specialised Relation classes implemented herein. Only once these details have been added to the
			dictionary will D3 instantiate objects of type \a HSMetaEntityTopic representing rows from the table \a D3HSDB.HSMetaEntityTopic.
	*/
	class D3_API HSMetaEntityTopicBase: public Entity
	{
		D3_CLASS_DECL(HSMetaEntityTopicBase);

		public:
			//! Enable iterating over all instances of this
			class D3_API iterator : public InstanceKeyPtrSetItr
			{
				public:
					iterator() {}
					iterator(const InstanceKeyPtrSetItr& itr) : InstanceKeyPtrSetItr(itr) {}

					//! De-reference operator*()
					virtual HSMetaEntityTopicPtr    operator*();
					//! Assignment operator=()
					virtual iterator&               operator=(const iterator& itr);
			};

			static unsigned int                 size(DatabasePtr pDB)         { return GetAll(pDB)->size(); }
			static bool                         empty(DatabasePtr pDB)        { return GetAll(pDB)->empty(); }
			static iterator                     begin(DatabasePtr pDB)        { return iterator(GetAll(pDB)->begin()); }
			static iterator                     end(DatabasePtr pDB)          { return iterator(GetAll(pDB)->end()); }



		protected:
			HSMetaEntityTopicBase() {}

		public:
			~HSMetaEntityTopicBase() {}

			//! Create a new HSMetaEntityTopic
			static HSMetaEntityTopicPtr         CreateHSMetaEntityTopic(DatabasePtr pDB)		{ return (HSMetaEntityTopicPtr) pDB->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSMetaEntityTopic)->CreateInstance(pDB); }

			//! Return a collection of all instances of this
			static InstanceKeyPtrSetPtr					GetAll(DatabasePtr pDB);

			//! Load all instances of this
			static void													LoadAll(DatabasePtr pDB, bool bRefresh = false, bool bLazyFetch = true);

			//! Load a particular instance of this
			static HSMetaEntityTopicPtr         Load(DatabasePtr pDB, const std::string& strMetaDatabaseAlias, const std::string& strMetaEntityName, long lTopicID, bool bRefresh = false, bool bLazyFetch = true);

			//! Get related parent HSTopic object
			virtual HSTopicPtr                  GetTopic();
			//! Set related parent HSTopic object
			virtual void												SetTopic(HSTopicPtr pHSTopic);


			/*! @name Get Column Values
			    \note These accessors do not throw even if the column's value is NULL.
			           Therefore, you should use these methods only if you're sure the
			           column's value is NOT NULL before using these.
			*/
			//@{
			//! TopicID
			virtual long                        GetTopicID()                   { return Column(HSMetaEntityTopic_TopicID)->GetLong(); }
			//! MetaDatabaseAlias
			virtual const std::string&          GetMetaDatabaseAlias()         { return Column(HSMetaEntityTopic_MetaDatabaseAlias)->GetString(); }
			//! MetaEntityName
			virtual const std::string&          GetMetaEntityName()            { return Column(HSMetaEntityTopic_MetaEntityName)->GetString(); }
			//@}

			/*! @name Set Column Values
			*/
			//@{
			//! Set TopicID
			virtual bool												SetTopicID(long val)           { return Column(HSMetaEntityTopic_TopicID)->SetValue(val); }
			//! Set MetaDatabaseAlias
			virtual bool												SetMetaDatabaseAlias(const std::string& val){ return Column(HSMetaEntityTopic_MetaDatabaseAlias)->SetValue(val); }
			//! Set MetaEntityName
			virtual bool												SetMetaEntityName(const std::string& val){ return Column(HSMetaEntityTopic_MetaEntityName)->SetValue(val); }
			//@}

			//! A column accessor provided mainly for backwards compatibility
			virtual ColumnPtr										Column(HSMetaEntityTopic_Fields eCol){ return Entity::GetColumn((unsigned int) eCol); }
	};



} // end namespace D3


#endif /* INC_HSMetaEntityTopicBase_H */