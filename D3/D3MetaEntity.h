#ifndef INC_D3MetaEntity_H
#define INC_D3MetaEntity_H

#include "D3MetaEntityBase.h"

namespace D3
{
	//! D3MetaEntity objects represent instances of D3MetaEntity records from the D3 Meta Dictionary database
	/*! This class is based on the generated #D3MetaEntityBase class.
	*/
	class D3_API D3MetaEntity : public D3MetaEntityBase
	{
		friend class D3::MetaDatabase;

		D3_CLASS_DECL(D3MetaEntity);

		protected:
			D3MetaEntity() {}
			D3MetaEntity(DatabasePtr pDB, D3MetaDatabasePtr pD3MD, MetaEntityPtr pME, float fSequenceNo);

		public:
			~D3MetaEntity() {}

			//! Return the D3MetaColumn object with the specified name (throws a D3::Exception if the column does not exist)
			D3MetaColumnPtr						GetMetaColumn	(const std::string & strColumnName);

			//! Return the D3MetaKey object with the specified name (throws a D3::Exception if the key does not exist)
			D3MetaKeyPtr							GetMetaKey		(const std::string & strKeyName);

			//! This static method simplifies creating fully populated and linked D3MetaEntity objects
			/*! This method allows you to create a new D3MetaEntity object and in one call pass all it's
					attributes to the method.

					The newly create D3MetaEntity object will be created in the same database workspace
					which owns the D3MetaDatabase you pass in.

					The purpose of D3MetaEntity objects is to persist D3::MetaEntity objects. In other words,
					at some stage these objects will be converted into D3::MetaEntity objects. Not surprisingly,
					the attributes passed in with this message resemble attributes of D3::MetaEntity.

					@param  [all]							See D3::MetaEntity for details

					@return D3MetaEntityPtr		The method returns the newly created D3MetaEntity which is a member
																		of the passed in D3MetaDatabase
			*/
			static	D3MetaEntityPtr		Create(	D3MetaDatabasePtr pMD,
																				const std::string & strName,
																				const std::string & strAlias,
																				const std::string & strInstanceClass,
																				const std::string & strInstanceInterface,
																				const std::string & strProsaName,
																				const std::string & strDescription,
																				const MetaEntity::Flags & flags = MetaEntity::Flags());

			//! Find resident D3MetaColumn
			/*! This method looks up all related D3MetaColumn objects and returns the instance
					matching the passed in name.
			*/
			D3MetaColumnPtr						FindD3MetaColumn(const std::string & strName);

			//! Returns this as a softlink like e.g.: (SELECT ID FROM D3MetaEntity WHERE Name='AP3Customer' AND MetaDatabaseID=(SELECT ID FROM D3MetaDatabase WHERE Alias=APAL3DB AND VersionMajor=8 AND VersionMinor=1 AND VersionRevision=1))
			std::string								IDAsSoftlink();

			//! Helper called by D3MetaDatabase::ExportAsSQLScript
			void											ExportAsSQLScript(std::ofstream& sqlout);

			//! Helper called by D3MetaDatabase::ExportAsSQLScript
			void											ExportRelationsAsSQLScript(std::ofstream& sqlout);
	};






	class D3_API CK_D3MetaEntity : public InstanceKey
	{
		D3_CLASS_DECL(CK_D3MetaEntity);

		public:
			CK_D3MetaEntity() {};
			~CK_D3MetaEntity() {};

			//! Returns a string reflecting the database and the entities name like [APAL3DB.6.06.0003][AP3WarehouseArea]
			virtual std::string			AsString();
	};


} // end namespace D3

#endif /* INC_D3MetaEntity_H */
