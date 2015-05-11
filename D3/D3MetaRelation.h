#ifndef INC_D3MetaRelation_H
#define INC_D3MetaRelation_H

#include "D3MetaRelationBase.h"

namespace D3
{
	//! D3MetaRelation objects represent instances of D3MetaRelation records from the D3 Meta Dictionary database
	/*! This class is based on the generated #D3MetaRelationBase class.
	*/
	class D3_API D3MetaRelation : public D3MetaRelationBase
	{
		friend class D3::MetaDatabase;

		D3_CLASS_DECL(D3MetaRelation);

		protected:
			D3MetaRelation() {}
			D3MetaRelation(DatabasePtr pDB, D3MetaKeyPtr pParentD3MK, D3MetaKeyPtr pChildD3MK, D3MetaColumnPtr pSwitchD3MC, MetaRelationPtr pMR, float fSequenceNo);

		public:
			~D3MetaRelation() {}

			//! This static method simplifies creating fully populated and linked D3MetaRelation objects
			/*! This method allows you to create a new D3MetaRelation object and in one call pass all it's
					attributes to the method.

					The newly create D3MetaRelation object will be created in the same database workspace
					which owns the D3MetaDatabase you pass in.

					The purpose of D3MetaRelation objects is to persist D3::MetaRelation objects. In other words,
					at some stage these objects will be converted into D3::MetaRelation objects. Not surprisingly,
					the attributes passed in with this message resemble attributes of D3::MetaRelation.

					@param  [all]								See D3::MetaRelation for details

					@return D3MetaRelationPtr		The method returns the newly created D3MetaRelation which is a member
																			of the passed in D3MetaDatabase
			*/
			static D3MetaRelationPtr		Create	(	const std::string & strName,
																						const std::string & strReverseName,
																						const std::string & strInstanceClass,
																						const std::string & strInstanceInterface,
																						D3MetaKeyPtr	pMKParent,
																						D3MetaKeyPtr	pMKChild,
																						const MetaRelation::Flags & flags = MetaRelation::Flags(),
																						D3MetaColumnPtr	pMCSwitch = NULL,
																						const std::string & strSwitchColumnValue = "");

			//! Helper called by D3MetaEntity::ExportAsSQLScript
			void												ExportAsSQLScript(std::ofstream& sqlout);
	};






	class D3_API CK_D3MetaRelation : public InstanceKey
	{
		D3_CLASS_DECL(CK_D3MetaRelation);

		public:
			CK_D3MetaRelation() {};
			~CK_D3MetaRelation() {};

			//! Returns a string reflecting the database, entity and the relations name like [APAL3DB.6.06.0003][AP3WarehouseArea][Shipments]
			virtual std::string			AsString();
	};


} // end namespace D3

#endif /* INC_D3MetaRelation_H */
