#ifndef INC_D3MetaKey_H
#define INC_D3MetaKey_H

#include "D3MetaKeyBase.h"

namespace D3
{
	//! D3MetaKey objects represent instances of D3MetaKey records from the D3 Meta Dictionary database
	/*! This class is based on the generated #D3MetaKeyBase class.
	*/
	class D3_API D3MetaKey : public D3MetaKeyBase
	{
		friend class D3::MetaDatabase;

		D3_CLASS_DECL(D3MetaKey);

		protected:
			D3MetaKey() {}
			D3MetaKey(DatabasePtr pDB, D3MetaEntityPtr pD3ME, MetaKeyPtr pMK, float fSequenceNo);

		public:
			~D3MetaKey() {}

			//! This static method simplifies creating fully populated and linked D3MetaKey objects
			/*! This method allows you to create a new D3MetaKey object and in one call pass all it's
					attributes to the method.

					The newly create D3MetaKey object will be created in the same database workspace
					which owns the D3MetaEntity you pass in. The newly created D3MetaKey will become
					a key of the D3MetaEntity object you pass in.

					The purpose of D3MetaKey objects is to persist D3::MetaKey objects. In other words,
					at some stage these objects will be converted into D3::MetaKey objects. Not surprisingly,
					the attributes passed in with this message resemble attributes of D3::MetaKey.

					@param  [all]							See D3::MetaKey for details

					@return D3MetaKeyPtr			The method returns the newly created D3MetaKey which is a member
																		of the passed in D3MetaEntity
			*/
			static D3MetaKeyPtr Create(	D3MetaEntityPtr pME,
																	const std::string & strName,
																	const std::string & strInstanceClass,
																	const std::string & strInstanceInterface,
																	const MetaKey::Flags & flags = MetaKey::Flags());

			//! Returns this as a softlink like e.g.: (SELECT ID FROM D3MetaKey WHERE Name='PK_AP3Customer' AND MetaEntityID=(SELECT ID FROM D3MetaEntity WHERE Name='AP3Customer' AND MetaDatabaseID=(SELECT ID FROM D3MetaDatabase WHERE Alias=APAL3DB AND VersionMajor=8 AND VersionMinor=1 AND VersionRevision=1)))
			std::string								IDAsSoftlink();

			//! Helper called by D3MetaEntity::ExportAsSQLScript
			void											ExportAsSQLScript(std::ofstream& sqlout);
	};






	class D3_API CK_D3MetaKey : public InstanceKey
	{
		D3_CLASS_DECL(CK_D3MetaKey);

		public:
			CK_D3MetaKey() {};
			~CK_D3MetaKey() {};

			//! Returns a string reflecting the database, entities and the keys name like [APAL3DB.6.06.0003][AP3WarehouseArea][PK_AP3WarehouseArea]
			virtual std::string			AsString();
	};


} // end namespace D3

#endif /* INC_D3MetaKey_H */
