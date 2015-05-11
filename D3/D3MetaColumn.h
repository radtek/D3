#ifndef INC_D3MetaColumn_H
#define INC_D3MetaColumn_H

#include "D3MetaColumnBase.h"

namespace D3
{
	//! D3MetaColumn objects represent instances of D3MetaColumn records from the D3 Meta Dictionary database
	/*! This class is based on the generated #D3MetaColumnBase class.
	*/
	class D3_API D3MetaColumn : public D3MetaColumnBase
	{
		friend class D3::MetaDatabase;

		D3_CLASS_DECL(D3MetaColumn);

		protected:
			D3MetaColumn() {}
			D3MetaColumn(DatabasePtr pDB, D3MetaEntityPtr pD3ME, MetaColumnPtr pMC, float fSequenceNo);

		public:
			~D3MetaColumn() {}

			//! This static method simplifies creating fully populated and linked D3MetaColumn objects
			/*! This method allows you to create a new D3MetaColumn object and in one call pass all it's
					attributes to the method.

					The newly create D3MetaColumn object will be created in the same database workspace
					which owns the D3MetaEntity you pass in. The newly created D3MetaColumn will become
					a column of the D3MetaEntity object you pass in. The sequence in which D3MetaColumn
					objects are created is the sequence in which they appear in the database if a database
					is created based on meta dictionary information.

					The purpose of D3MetaColumn objects is to persist D3::MetaColumn objects. In other words,
					at some stage these objects will be converted into D3::MetaColumn objects. Not surprisingly,
					the attributes passed in with this message resemble attributes of D3::MetaColumn.

					@param  [all]							See D3::MetaColumn for details

					@return D3MetaColumnPtr		The method returns the newly created D3MetaColumn which is a member
																		of the passed in D3MetaEntity
			*/
			static D3MetaColumnPtr	Create(	D3MetaEntityPtr pME,
																			const std::string & strName,	
																			const std::string & strInstanceClass,	
																			const std::string & strInstanceInterface,	
																			const std::string & strProsaName,	
																			const std::string & strDescription,
																			MetaColumn::Type eType,
																			int  iMaxLength,
																			const MetaColumn::Flags & flags = MetaColumn::Flags(),
																			const char*	szDefaultValue = 0);

			//! Returns this as a softlink like e.g.: (SELECT ID FROM D3MetaColumn WHERE Name='Length' AND MetaEntityID=(SELECT ID FROM D3MetaEntity WHERE Name='AP3Customer' AND MetaDatabaseID=(SELECT ID FROM D3MetaDatabase WHERE Alias=APAL3DB AND VersionMajor=8 AND VersionMinor=1 AND VersionRevision=1)))
			std::string								IDAsSoftlink();

			//! Helper called by D3MetaEntity::ExportAsSQLScript
			void											ExportAsSQLScript(std::ofstream& sqlout);
	};






	class D3_API CK_D3MetaColumn : public InstanceKey
	{
		D3_CLASS_DECL(CK_D3MetaColumn);

		public:
			CK_D3MetaColumn() {};
			~CK_D3MetaColumn() {};

			//! Returns a string reflecting the database and the entities name like [APAL3DB.6.06.0003][AP3WarehouseArea][ID]
			virtual std::string			AsString();
	};


} // end namespace D3

#endif /* INC_D3MetaColumn_H */
