#ifndef INC_HSTopicAssociationBase_H
#define INC_HSTopicAssociationBase_H

// WARNING: This file has been generated by D3. You should NEVER change this file.
//          Please refer to the documentation of MetaEntity::CreateSpecialisedCPPHeader()
//          which explains in detail how you can gegenerate this file.
//
// The file declares methods which simplify client interactions with objects of type 
// HSTopicAssociation representing instances of D3HSDB.HSTopicAssociation.
//
//       For D3 to work correctly, you must implement your own class as follows:
//
//       #include "HSTopicAssociationBase.h"
//
//       namespace D3
//       {
//         class HSTopicAssociation : public HSTopicAssociationBase
//         {
//           D3_CLASS_DECL(HSTopicAssociation);
//
//           protected:
//             HSTopicAssociation() {}
//
//           public:
//             ~HSTopicAssociation() {}
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

	//! Use these enums to access columns through HSTopicAssociation::Column() method
	enum HSTopicAssociation_Fields
	{
		HSTopicAssociation_ParentTopicID,
		HSTopicAssociation_ChildTopicID,
		HSTopicAssociation_SequenceNo
	};


	//! HSTopicAssociationBase is a base class that \b MUST be subclassed through a class called \a HSTopicAssociation.
	/*! The purpose of this class to provide more natural access to related objects as well as this' columns.
			This class is only usefull if it is subclassed by a class called HSTopicAssociation
			Equally important is that the meta dictionary knows of the existence of your subclass as well as
			specialised Relation classes implemented herein. Only once these details have been added to the
			dictionary will D3 instantiate objects of type \a HSTopicAssociation representing rows from the table \a D3HSDB.HSTopicAssociation.
	*/
	class D3_API HSTopicAssociationBase: public Entity
	{
		D3_CLASS_DECL(HSTopicAssociationBase);

		public:
			//! Enable iterating over all instances of this
			class D3_API iterator : public InstanceKeyPtrSetItr
			{
				public:
					iterator() {}
					iterator(const InstanceKeyPtrSetItr& itr) : InstanceKeyPtrSetItr(itr) {}

					//! De-reference operator*()
					virtual HSTopicAssociationPtr   operator*();
					//! Assignment operator=()
					virtual iterator&               operator=(const iterator& itr);
			};

			static unsigned int                 size(DatabasePtr pDB)         { return GetAll(pDB)->size(); }
			static bool                         empty(DatabasePtr pDB)        { return GetAll(pDB)->empty(); }
			static iterator                     begin(DatabasePtr pDB)        { return iterator(GetAll(pDB)->begin()); }
			static iterator                     end(DatabasePtr pDB)          { return iterator(GetAll(pDB)->end()); }



		protected:
			HSTopicAssociationBase() {}

		public:
			~HSTopicAssociationBase() {}

			//! Create a new HSTopicAssociation
			static HSTopicAssociationPtr        CreateHSTopicAssociation(DatabasePtr pDB)		{ return (HSTopicAssociationPtr) pDB->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopicAssociation)->CreateInstance(pDB); }

			//! Return a collection of all instances of this
			static InstanceKeyPtrSetPtr					GetAll(DatabasePtr pDB);

			//! Load all instances of this
			static void													LoadAll(DatabasePtr pDB, bool bRefresh = false, bool bLazyFetch = true);

			//! Load a particular instance of this
			static HSTopicAssociationPtr        Load(DatabasePtr pDB, long lParentTopicID, long lChildTopicID, float fSequenceNo, bool bRefresh = false, bool bLazyFetch = true);

			//! Get related parent HSTopic object
			virtual HSTopicPtr                  GetParentTopic();
			//! Set related parent HSTopic object
			virtual void												SetParentTopic(HSTopicPtr pHSTopic);
			//! Get related parent HSTopic object
			virtual HSTopicPtr                  GetChildTopic();
			//! Set related parent HSTopic object
			virtual void												SetChildTopic(HSTopicPtr pHSTopic);


			/*! @name Get Column Values
			    \note These accessors do not throw even if the column's value is NULL.
			           Therefore, you should use these methods only if you're sure the
			           column's value is NOT NULL before using these.
			*/
			//@{
			//! ParentTopicID
			virtual long                        GetParentTopicID()             { return Column(HSTopicAssociation_ParentTopicID)->GetLong(); }
			//! ChildTopicID
			virtual long                        GetChildTopicID()              { return Column(HSTopicAssociation_ChildTopicID)->GetLong(); }
			//! SequenceNo
			virtual float                       GetSequenceNo()                { return Column(HSTopicAssociation_SequenceNo)->GetFloat(); }
			//@}

			/*! @name Set Column Values
			*/
			//@{
			//! Set ParentTopicID
			virtual bool												SetParentTopicID(long val)     { return Column(HSTopicAssociation_ParentTopicID)->SetValue(val); }
			//! Set ChildTopicID
			virtual bool												SetChildTopicID(long val)      { return Column(HSTopicAssociation_ChildTopicID)->SetValue(val); }
			//! Set SequenceNo
			virtual bool												SetSequenceNo(float val)       { return Column(HSTopicAssociation_SequenceNo)->SetValue(val); }
			//@}

			//! A column accessor provided mainly for backwards compatibility
			virtual ColumnPtr										Column(HSTopicAssociation_Fields eCol){ return Entity::GetColumn((unsigned int) eCol); }
	};



} // end namespace D3


#endif /* INC_HSTopicAssociationBase_H */
