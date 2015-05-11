#ifndef INC_D3MetaKeyColumn_H
#define INC_D3MetaKeyColumn_H

#include "D3MetaKeyColumnBase.h"

namespace D3
{
	//! D3MetaKeyColumn objects represent instances of D3MetaKeyColumn records from the D3 Meta Dictionary database
	/*! This class is based on the generated #D3MetaKeyColumnBase class.
	*/
	class D3_API D3MetaKeyColumn : public D3MetaKeyColumnBase
	{
		friend class D3::MetaDatabase;

		D3_CLASS_DECL(D3MetaKeyColumn);

		protected:
			D3MetaKeyColumn() {}
			D3MetaKeyColumn(DatabasePtr pDB, D3MetaKeyPtr pD3MK, D3MetaColumnPtr pD3MC, float fSequenceNo);

		public:
			~D3MetaKeyColumn() {}

			//! This static method simplifies creating fully populated and linked D3MetaKeyColumn objects
			/*! This method allows you to create a new D3MetaKeyColumn object and in one call pass all it's
					attributes to the method.

					The newly create D3MetaKeyColumn object will be created in the same database workspace
					which owns the D3MetaKey you pass in. The newly created D3MetaKeyColumn will become
					a key of the D3MetaKey object you pass in. It is important that you create key columns in the
					correct order.

					The purpose of D3MetaKeyColumn objects is to persist D3::MetaKeyColumn objects. In other words,
					at some stage these objects will be converted into D3::MetaKeyColumn objects. Not surprisingly,
					the attributes passed in with this message resemble attributes of D3::MetaKeyColumn.

					@param  [all]								See D3::MetaKeyColumn for details

					@return D3MetaKeyColumnPtr	The method returns the newly created D3MetaKeyColumn which is a member
																			of the passed in D3MetaKey
			*/
			static D3MetaKeyColumnPtr			Create( D3MetaKeyPtr pMK,
																						const std::string & strColumnName);

			//! Helper called by D3MetaKey::ExportAsSQLScript
			void													ExportAsSQLScript(std::ofstream& sqlout);
	};
} // end namespace D3

#endif /* INC_D3MetaKeyColumn_H */
