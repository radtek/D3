// MODULE: Column source
//;
//; IMPLEMENTATION CLASS: Column
//;
// ===========================================================
// File Info:
//
// $Author: lalit $
// $Date: 2014/12/29 08:32:40 $
// $Revision: 1.55 $
//
// MetaColumn: This class describes a column of any of
// the ap3/t3 classes.
//
// ===========================================================
// Change History:
// ===========================================================
//
// 09/10/02 - R1 - Hugp
//
// Changes required to accomodate extended attributes as
// implemented by APAL3ActivityType and APAL3Activity:
//
// - Allow base columns to live without m_pEntity
// - Store column names as strings
//
// -----------------------------------------------------------

// @@DatatypeInclude
#include "D3Types.h"
// @@End
// @@Includes
#include <time.h>
#include <stdio.h>
#include <limits.h>
#include <limits.h>

#include "Entity.h"
#include "Column.h"
#include "Key.h"
#include "Relation.h"
#include "Exception.h"
#include "D3Funcs.h"
#include "Session.h"
#include "Codec.h"

namespace D3
{
	// Support for IsApproximatelyEqual
	//
	namespace
	{
		template <typename T>
		inline T abs(T p) { return p > 0 ? p : p * -1; }

		template <typename T>
		inline bool approx_equal(T p1, T p2, float pct) { return (abs<T>(p1-p2) <= (p1*pct*.01F)); }
	}


	// ==========================================================================
	// MetaColumn::Flags Mask specification
	//

	// Implement Characteristics bitmask
	BITS_IMPL(MetaColumn, Flags);

	// Primitive mask values
	PRIMITIVEMASK_IMPL(MetaColumn, Flags, Mandatory,					0x00000004);
	PRIMITIVEMASK_IMPL(MetaColumn, Flags, AutoNum,						0x00000008);
	PRIMITIVEMASK_IMPL(MetaColumn, Flags, Derived,						0x00000010);

	PRIMITIVEMASK_IMPL(MetaColumn, Flags, SingleChoice,				0x00000040);
	PRIMITIVEMASK_IMPL(MetaColumn, Flags, MultiChoice,				0x00000080);
	PRIMITIVEMASK_IMPL(MetaColumn, Flags, LazyFetch,					0x00000100);
	PRIMITIVEMASK_IMPL(MetaColumn, Flags, Password,						0x00000020);
	PRIMITIVEMASK_IMPL(MetaColumn, Flags, EncodedValue,				0x00000400);

	PRIMITIVEMASK_IMPL(MetaColumn, Flags, HiddenOnDetailView,	0x00000002);
	PRIMITIVEMASK_IMPL(MetaColumn, Flags, HiddenOnListView,		0x00000200);

	// Combo mask values
	COMBOMASK_IMPL(MetaColumn, Flags, Default,	0x00000000);
	COMBOMASK_IMPL(MetaColumn, Flags, Hidden,		MetaColumn::Flags::HiddenOnDetailView |
																							MetaColumn::Flags::HiddenOnListView);


	// Implement Permissions bitmask
	BITS_IMPL(MetaColumn, Permissions);

	// Primitive Permissions bitmask values
	PRIMITIVEMASK_IMPL(MetaColumn, Permissions, Read,					0x00000001);
	PRIMITIVEMASK_IMPL(MetaColumn, Permissions, Write,				0x00000002);

	// Combo Permissions bitmask values
	COMBOMASK_IMPL(MetaColumn, Permissions, Default,	MetaColumn::Permissions::Read |
																										MetaColumn::Permissions::Write);


	// ==========================================================================
	// Data implementation
	//

	Data::Data(unsigned int l) : m_iLen(0), m_pBuf(NULL)
	{
		if (l > 0)
		{
			m_pBuf = new unsigned char [l];
			memset(m_pBuf, 0, l);
			m_iLen = l;
		}
		else
		{
			m_iLen = 0;
		}
	}



	//! compare the data of this with the data in external buffer
	int Data::compare(unsigned char* p, unsigned int l) const
	{
		if (m_iLen == 0 && l == 0)
			return 0;

		if (m_iLen == 0)
			return -1;

		if (l == 0)
			return 1;

		for (unsigned int i=0; i < l && i < m_iLen; i++)
		{
			if (m_pBuf[i] < p[i])
				return -1;

			if (m_pBuf[i] > p[i])
				return 1;
		}

		if (m_iLen < l)
			return -1;

		if (m_iLen > l)
			return 1;

		return 0;
	}



	//! assign data from a buffer
	void Data::assign(const unsigned char* p, unsigned int l)
	{
		if (m_iLen != l)
		{
			m_iLen = 0;
			delete [] m_pBuf;
			m_pBuf = NULL;

			if (l > 0)
			{
				m_pBuf = new unsigned char [l];
				m_iLen = l;
			}
		}

		if (m_pBuf)
			memcpy((void*) m_pBuf, (void*) p, m_iLen);
	}



	std::string Data::base64_encode() const
	{
		std::string strEncoded;

		if (m_iLen > 0)
			APALUtil::base64_encode(strEncoded, m_pBuf, m_iLen);

		return strEncoded;
	}



	void Data::base64_decode(const std::string& strEncodedData)
	{
		std::string strDecoded;
		strDecoded = APALUtil::base64_decode(strEncodedData);
		assign((const unsigned char*) strDecoded.c_str(), strDecoded.length());
	}



	// ==========================================================================
	// MetaColumn implementation
	//

	// Statics:
	//
	ColumnID																MetaColumn::M_uNextInternalID = COLUMN_ID_MAX;
	MetaColumnPtrMap												MetaColumn::M_mapMetaColumn;

	// Standard D3 stuff
	//
	D3_CLASS_IMPL_PV(MetaColumn, Object);


	MetaColumn::MetaColumn()
	{
		// this is a dummy constructor... if you want to use it, address m_uID/M_mapMetaColumn issues first
		assert(false);
	}



	MetaColumn::~MetaColumn()
	{
		if (m_pMetaEntity)
			m_pMetaEntity->On_MetaColumnDeleted(this);

		delete m_pMapColumnChoice;

		M_mapMetaColumn.erase(m_uID);
	}



	void MetaColumn::Init(const std::string& strInstanceClassName, const std::string& strDefaultClassName)
	{
		MetaColumnPtr		pMC = NULL;


		try
		{
			if (!strInstanceClassName.empty())
				m_pInstanceClass = &(Class::Of(strInstanceClassName));
			else
				m_pInstanceClass = &(Class::Of(strDefaultClassName));

			assert(m_pInstanceClass->IsKindOf(Class::Of(strDefaultClassName)));
		}
		catch(Exception & e)
		{
			e.AddMessage("MetaColumn::Init(): Error creating MetaColumn %s, the instance class name %s is not a valid, using default class %s", GetFullName().c_str(), strInstanceClassName.c_str(), strDefaultClassName.c_str());
			e.LogError();
			m_pInstanceClass = &(Class::Of(strDefaultClassName));
		}

		assert(!m_strName.empty());

		// Check Duplicate
		//
		if (m_pMetaEntity)
		{
			// We must do this before we add the new column as otherwise we'd always
			// find one. If we find one now, report it after adding this one
			//
			pMC = m_pMetaEntity->GetMetaColumn(m_strName);

			// Add this to the MetaEntity objects column collection
			//
			m_pMetaEntity->On_MetaColumnCreated(this);
		}

		// If this is a duplicate, throw an exception
		//
		if (pMC)
			throw Exception(__FILE__, __LINE__, Exception_error, "MetaColumn::Init(): Column %s already exists!", GetFullName().c_str());

		assert(M_mapMetaColumn.find(m_uID) == M_mapMetaColumn.end());
		M_mapMetaColumn[m_uID] = this;
	}



	std::string MetaColumn::ChoiceDescAsString(const std::string & strChoiceVal)
	{
		ColumnChoiceMapItr			itr;


		assert(IsSingleChoice());

		for (	itr =		m_pMapColumnChoice->begin();
					itr !=	m_pMapColumnChoice->end();
					itr++)
		{
			if (itr->second.Val == strChoiceVal)
				return itr->second.Desc;
		}

		return strChoiceVal;	// return the index if there is no value
	}



	StringList MetaColumn::ChoiceDescs(unsigned long lValue)
	{
		ColumnChoiceMapItr			itr;
		StringList							listString;
		unsigned long						lCurrentBit;


		assert(IsMultiChoice());

		for (	itr =  m_pMapColumnChoice->begin();
					itr != m_pMapColumnChoice->end();
					itr++)
		{
			Convert(lCurrentBit, itr->second.Val);

			if (lValue & lCurrentBit)
				listString.push_back(itr->second.Desc);
		}

		return listString;
	}



	std::string MetaColumn::ChoiceDescsAsString(unsigned long lBitMap)
	{
		StringListItr				itr;
		StringList					listSelection;
		std::string					strValue;


		assert(IsMultiChoice());

		listSelection = ChoiceDescs(lBitMap);

		for (	itr =  listSelection.begin();
					itr != listSelection.end();
					itr++)
		{
			strValue += '[';
			strValue += *itr;
			strValue += ']';
		}

		return strValue;
	}



	//! Debug aid: The method dumps this and all its objects to cout
	void MetaColumn::Dump(int nIndentSize, bool bDeep)
	{
		// Dump this
		//
		std::cout << std::setw(nIndentSize) << "";
		std::cout << "MetaColumn " << m_strName << std::endl;
		std::cout << std::setw(nIndentSize) << "";
		std::cout << "{" << std::endl;

		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "Name              " << m_strName << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "InstanceClass     " << m_pInstanceClass->Name() << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "InstanceInterface " << m_strInstanceInterface << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "ProsaName         " << m_strProsaName << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "Description       " << m_strDescription << std::endl;

		if (m_pMetaEntity)
		{
			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "Type              " << GetTypeAsSQLString(m_pMetaEntity->GetMetaDatabase()->GetTargetRDBMS()) << std::endl;
		}
		else
		{
			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "Type              " << GetTypeAsCString() << std::endl;
		}

		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "Flags             " << m_Flags.AsString() << std::endl;

		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "Permissions       " << m_Permissions.AsString() << std::endl;

		std::cout << std::setw(nIndentSize) << "";
		std::cout << "}" << std::endl;
	}



	/* static */
	std::ostream & MetaColumn::AllAsJSON(RoleUserPtr pRoleUser, std::ostream & ojson)
	{
		D3::RolePtr									pRole = pRoleUser ? pRoleUser->GetRole() : NULL;
		D3::MetaDatabasePtr					pMD;
		D3::MetaDatabasePtrMapItr		itrMD;
		D3::MetaEntityPtr						pME;
		D3::MetaEntityPtrVectItr		itrME;
		D3::MetaColumnPtr						pMC;
		D3::MetaColumnPtrVectItr		itrMC;
		bool												bFirst = true;

		ojson << '[';

		for ( itrMD =	 MetaDatabase::GetMetaDatabases().begin();
					itrMD != MetaDatabase::GetMetaDatabases().end();
					itrMD++)
		{
			pMD = itrMD->second;

			for ( itrME =  pMD->GetMetaEntities()->begin();
						itrME != pMD->GetMetaEntities()->end();
						itrME++)
			{
				pME = *itrME;

				for ( itrMC =  pME->GetMetaColumns()->begin();
							itrMC != pME->GetMetaColumns()->end();
							itrMC++)
				{
					pMC = *itrMC;

					if (!pMC->IsDerived() && pRole->GetPermissions(pMC) & D3::MetaColumn::Permissions::Read)
					{
						bFirst ? bFirst = false : ojson << ',';
						ojson << "\n\t";
						pMC->AsJSON(pRoleUser, ojson, NULL);
					}
				}
			}
		}

		ojson << "\n]";


		return ojson;
	}



	std::ostream & MetaColumn::AsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm, bool* pFirstSibling)
	{
		RolePtr										pRole = pRoleUser ? pRoleUser->GetRole() : NULL;
		MetaColumn::Permissions		permissions = (!IsDerived() && pRole) ? pRole->GetPermissions(this) : m_Permissions;

		if (permissions & MetaColumn::Permissions::Read)
		{
			if (pFirstSibling)
			{
				if (*pFirstSibling)
					*pFirstSibling = false;
				else
					ostrm << ',';
			}

			ostrm << "{";
			ostrm << "\"ID\":"									<< GetID() << ',';

			ostrm << "\"MetaEntityID\":";

			if (!m_pMetaEntity)
				ostrm << "null";
			else
				ostrm << m_pMetaEntity->GetID();

			ostrm << ',';

			ostrm << "\"Idx\":"									<< GetColumnIdx() << ',';
			ostrm << "\"Name\":\""							<< JSONEncode(GetName()) << "\",";
			ostrm << "\"ProsaName\":\""					<< JSONEncode(GetProsaName()) << "\",";
/*
			std::string		strBase64Desc;
			APALUtil::base64_encode(strBase64Desc, (const unsigned char *) GetDescription().c_str(), GetDescription().size());
			ostrm << "\"Description\":\""				<< strBase64Desc << "\",";
*/
			ostrm << "\"Type\":\""							<< JSONEncode(GetTypeAsString()) << "\",";
			ostrm << "\"MaxLength\":"						<< GetMaxLength() << ',';

			ostrm << "\"InstanceClass\":\""			<< JSONEncode(GetInstanceClassName()) << "\",";
			ostrm << "\"InstanceInterface\":\""	<< JSONEncode(GetInstanceInterface()) << "\",";
			ostrm << "\"JSMetaClass\":\""				<< JSONEncode(GetJSMetaClassName()) << "\",";
			ostrm << "\"JSInstanceClass\":\""		<< JSONEncode(GetJSInstanceClassName()) << "\",";
			ostrm << "\"JSViewClass\":\""				<< JSONEncode(GetJSViewClassName()) << "\",";

			ostrm << "\"Mandatory\":"						<< (IsMandatory()						? "true" : "false") << ',';
			ostrm << "\"AutoNum\":"							<< (IsAutoNum()							? "true" : "false") << ',';
			ostrm << "\"Derived\":"							<< (IsDerived()							? "true" : "false") << ',';
			ostrm << "\"PrimaryKeyMember\":"		<< (IsPrimaryKeyMember()		? "true" : "false") << ',';
			ostrm << "\"SingleChoice\":"				<< (IsSingleChoice()				? "true" : "false") << ',';
			ostrm << "\"MultiChoice\":"					<< (IsMultiChoice()					? "true" : "false") << ',';
			ostrm << "\"LazyFetch\":"						<< (IsLazyFetch()						? "true" : "false") << ',';
			ostrm << "\"Password\":"						<< (IsPassword()						? "true" : "false") << ',';
			ostrm << "\"EncodedValue\":"				<< (IsEncodedValue()				? "true" : "false") << ',';

			ostrm << "\"HiddenOnDetailView\":"	<< (IsHiddenOnDetailView()	? "true" : "false") << ',';
			ostrm << "\"HiddenOnListView\":"		<< (IsHiddenOnListView()		? "true" : "false") << ',';

			ostrm << "\"AllowRead\":"						<< (permissions & MetaColumn::Permissions::Read		? "true" : "false") << ',';
			ostrm << "\"AllowWrite\":"					<< (permissions & MetaColumn::Permissions::Write	? "true" : "false") << ',';

			ostrm << "\"HSTopics\":"						<< (m_strHSTopicsJSON.empty() ? "null" : m_strHSTopicsJSON) << ',';

			ostrm << "\"Unit\":"								<< m_uUnit << ',';
			ostrm << "\"FloatPrecision\":"			<< m_uFloatPrecision << ',';

			ostrm << "\"DefaultValue\":";
			if (IsDefaultValueNull())
				ostrm << "null,";
			else
				ostrm << "\"" << JSONEncode(GetDefaultValue()) << "\",";

			ostrm << "\"Choices\":";
			if (m_pMapColumnChoice && (IsSingleChoice() || IsMultiChoice()))
			{
				ColumnChoiceMapItr		itr;
				bool									bFirst;

				ostrm << "\n\t[";

				bFirst = true;

				for ( itr	 = m_pMapColumnChoice->begin();
							itr != m_pMapColumnChoice->end();
							itr++)
				{
					bFirst ? bFirst = false : ostrm << ',';

					ostrm << "\n\t\t{";
					ostrm << "\"SeqNo\":"		<< itr->second.SeqNo	<< ",";
					ostrm << "\"Val\":\""		<< JSONEncode(itr->second.Val)		<< "\",";
					ostrm << "\"Desc\":\""	<< JSONEncode(itr->second.Desc)		<< "\"";
					ostrm << "}";
				}

				ostrm << "\n\t]";
			}
			else
			{
				ostrm << "[]";
			}

			ostrm << "}";
		}

		return ostrm;
	}





	std::string MetaColumn::GetAbbreviationofUnit()
	{
		std::string		strAbbreviation("");
		if(GetUnit() == D3::MetaColumn::unitGeneric)
			strAbbreviation = "";
		else if(GetUnit() == D3::MetaColumn::unitInch)
			strAbbreviation = "IN";
		else if(GetUnit() == D3::MetaColumn::unitFoot)
			strAbbreviation = "FT";
		else if(GetUnit() == D3::MetaColumn::unitYard)
			strAbbreviation = "YD";
		else if(GetUnit() == D3::MetaColumn::unitCentimeter)
			strAbbreviation = "CM";
		else if(GetUnit() == D3::MetaColumn::unitMeter)
			strAbbreviation = "M";
		else if(GetUnit() == D3::MetaColumn::unitPound)
			strAbbreviation = "LB";
		else if(GetUnit() == D3::MetaColumn::unitGram)
			strAbbreviation = "G";
		else if(GetUnit() == D3::MetaColumn::unitKilogram)
			strAbbreviation = "KG";
		else if(GetUnit() == D3::MetaColumn::unitCelsius)
			strAbbreviation = "C";
		else if(GetUnit() == D3::MetaColumn::unitFarenheit)
			strAbbreviation = "F";
		else if(GetUnit() == D3::MetaColumn::unitPercent)
			strAbbreviation = "%";
		else if(GetUnit() == D3::MetaColumn::unitMillisecond)
			strAbbreviation = "MSEC";
		else if(GetUnit() == D3::MetaColumn::unitSecond)
			strAbbreviation = "SEC";
		else if(GetUnit() == D3::MetaColumn::unitMinute)
			strAbbreviation = "MIN";
		else if(GetUnit() == D3::MetaColumn::unitHour)
			strAbbreviation = "HR";
		else if(GetUnit() == D3::MetaColumn::unitDay)
			strAbbreviation = "DAY";
		else if(GetUnit() == D3::MetaColumn::unitWeek)
			strAbbreviation = "WK";
		else if(GetUnit() == D3::MetaColumn::unitMonth)
			strAbbreviation = "MO";
		else if(GetUnit() == D3::MetaColumn::unitYear)
			strAbbreviation = "YR";

		return strAbbreviation;
	}





	// Construct a MetaColumn object from a D3MetaColumn object (a D3MetaColumn object
	// represents a record from the D3MetaColumn table in the D3 MetaDictionary database)
	//
	/* static */
	void MetaColumn::LoadMetaObject(MetaEntityPtr pNewME, D3MetaColumnPtr pD3MC)
	{
		static int				s_imperial = Settings::Singleton().Imperial();

		MetaColumnPtr			pNewMC;
		std::string				strName;
		std::string				strClassName;
		Flags::Mask				flags;
		Permissions::Mask	permissions;
		Type							eType;
		int								iMaxLength;
		ColumnID					uID;


		assert(pD3MC);
		assert(pD3MC->GetMetaEntity()->GetName() == "D3MetaColumn");

		// Get the details needed by the MetaColumn ctor()
		//
		strName			= pD3MC->GetName();
		iMaxLength	= pD3MC->GetMaxLength();
		eType				= (Type) pD3MC->GetType();

		if (eType > dbfMaxValid || eType < dbfMinValid)
		{
			ReportError("MetaColumn::LoadMetaObject(): Column %s.%s has incorrect type.", pNewME->GetFullName().c_str(), strName.c_str());
			eType = dbfUndefined;
		}

		// Get the instance class name for the MetaEntity
		//
		if (!pD3MC->GetColumn(D3MDDB_D3MetaColumn_InstanceClass)->IsNull())
			strClassName = pD3MC->GetInstanceClass();

		// Set the construction flags
		//
		if (!pD3MC->GetColumn(D3MDDB_D3MetaColumn_Flags)->IsNull())
			flags = Flags::Mask((unsigned long) pD3MC->GetFlags());

		// Set the construction permissions and adjust if necessary
		//
		if (!pD3MC->GetColumn(D3MDDB_D3MetaColumn_AccessRights)->IsNull())
			permissions = Permissions::Mask((unsigned long) pD3MC->GetAccessRights());

		if (!pNewME->AllowSelect())
			permissions &= ~Permissions::Read;

		if (!pNewME->AllowUpdate())
			permissions &= ~Permissions::Write;

		// Get the ColumnID
		uID = pD3MC->GetID();

		// Create matching MetaEntity object
		//
		switch (eType)
		{
			case dbfString:
				pNewMC = new MetaColumnString(pNewME, uID, strName, iMaxLength, flags, permissions, strClassName);
				break;

			case dbfChar:
				pNewMC = new MetaColumnChar(pNewME, uID, strName, flags, permissions, strClassName);
				break;

			case dbfShort:
				pNewMC = new MetaColumnShort(pNewME, uID, strName, flags, permissions, strClassName);
				break;

			case dbfBool:
				pNewMC = new MetaColumnBool(pNewME, uID, strName, flags, permissions, strClassName);
				break;

			case dbfInt:
				pNewMC = new MetaColumnInt(pNewME, uID, strName, flags, permissions, strClassName);
				break;

			case dbfLong:
				pNewMC = new MetaColumnLong(pNewME, uID, strName, flags, permissions, strClassName);
				break;

			case dbfFloat:
				pNewMC = new MetaColumnFloat(pNewME, uID, strName, flags, permissions, strClassName);
				break;

			case dbfDate:
				pNewMC = new MetaColumnDate(pNewME, uID, strName, flags, permissions, strClassName);
				break;

			case dbfBlob:
				pNewMC = new MetaColumnBlob(pNewME, uID, strName, iMaxLength, flags, permissions, strClassName);
				break;

			default:
				throw Exception(__FILE__, __LINE__, Exception_error, "MetaColumn::LoadMetaObject(): Meta column type %i for column %s in table %s is not supported!", eType, strName.c_str(), pD3MC->GetParentRelation(D3MDDB_D3MetaEntity_PR_D3MetaDatabase)->GetParent()->GetColumn(D3MDDB_D3MetaEntity_Name)->AsString().c_str());
		}

		// Set all the other attributes
		//
		pNewMC->m_strProsaName					= pD3MC->GetProsaName();
		pNewMC->m_strDescription				= pD3MC->GetDescription();
		pNewMC->m_strInstanceInterface	= pD3MC->GetInstanceInterface();
		pNewMC->m_strJSMetaClass				= pD3MC->GetJSMetaClass();
		pNewMC->m_strJSInstanceClass		= pD3MC->GetJSInstanceClass();
		pNewMC->m_strJSViewClass				= pD3MC->GetJSViewClass();
		pNewMC->m_uFloatPrecision				= pD3MC->GetFloatPrecision();

		if (!pD3MC->GetColumn(D3MDDB_D3MetaColumn_Unit)->IsNull())
			pNewMC->m_uUnit	= (Unit) pD3MC->GetUnit();
		else
			pNewMC->m_uUnit = unitGeneric;

		// Translate units
		if (s_imperial)
			pNewMC->MakeImperial();
		else
			pNewMC->MakeMetric();

		if (!pD3MC->GetColumn(D3MDDB_D3MetaColumn_DefaultValue)->IsNull())
			pNewMC->SetDefaultValue(pD3MC->GetDefaultValue());

		// Create MetaColumnChoices
		//
		if (!pD3MC->GetD3MetaColumnChoices()->empty())
		{
			D3MetaColumn::D3MetaColumnChoices::iterator		itrD3MCC;
			D3MetaColumnChoicePtr													pD3MCC;
			float																					fSeqNo;


			pNewMC->m_pMapColumnChoice = new ColumnChoiceMap;

			for ( itrD3MCC =  pD3MC->GetD3MetaColumnChoices()->begin();
						itrD3MCC != pD3MC->GetD3MetaColumnChoices()->end();
						itrD3MCC++)
			{
				pD3MCC = *itrD3MCC;
				fSeqNo = pD3MCC->GetSequenceNo();
				pNewMC->m_pMapColumnChoice->insert(ColumnChoiceMap::value_type(fSeqNo, ColumnChoice(fSeqNo, pD3MCC->GetChoiceVal(), pD3MCC->GetChoiceDesc())));
			}
		}
	}



	unsigned int MetaColumn::GetMetaKeyIndex(MetaKeyPtr pMK)
	{
		unsigned int		idx;


		assert(pMK);

		for (idx = 0; idx < m_vectMetaKey.size(); idx++)
		{
			if (m_vectMetaKey[idx] == pMK)
				break;
		}

		return idx;
	}



	void MetaColumn::SetDefaultDescription()
	{
		if (m_strDescription.empty())
		{
			if (IsSingleChoice() || IsMultiChoice())
			{
				if (!m_pMapColumnChoice || m_pMapColumnChoice->empty())
				{
					m_strDescription = "Undefined";
				}
				else
				{
					ColumnChoiceMapItr								itr;

					for ( itr =  m_pMapColumnChoice->begin();
								itr != m_pMapColumnChoice->end();
								itr++)
					{
						if (m_strDescription.empty())
						{
							if (IsSingleChoice())
								m_strDescription = "One of<br/><ul>";
							else
								m_strDescription = "Any combination of<br/><ul>";
						}

						m_strDescription += "<li>";
						m_strDescription += itr->second.Desc;
						m_strDescription += "</li>";
					}

					m_strDescription += "</ul>";
				}
			}
		}
	}



	std::string MetaColumn::GetFullName() const
	{
		std::string			strFullName;


		if (m_pMetaEntity)
		{
			strFullName  = m_pMetaEntity->GetFullName();
			strFullName += ".";
		}

		strFullName += m_strName;

		return strFullName;
	}



	/* static */ const char* MetaColumn::UnitToString(Unit eUnit)
	{
		// we only bother with types that make sense to convert
		switch (eUnit)
		{
			case unitInch:
				return "IN";
			case unitFoot:
				return "FT";
			case unitYard:
				return "YRD";
			case unitCentimeter:
				return "CM";
			case unitMeter:
				return "M";
			case unitPound:
				return "LBS";
			case unitGram:
				return "GR";
			case unitKilogram:
				return "KG";
			case unitCelsius:
				return "C";
			case unitFarenheit:
				return "F";
		}

		return NULL;
	}



	/* static */ MetaColumn::Unit MetaColumn::StringToUnit(const std::string& strUOM)
	{
		if (strUOM == "IN")
			return unitInch;
		if (strUOM == "FT")
			return unitFoot;
		if (strUOM == "YRD")
			return unitYard;
		if (strUOM == "CM")
			return unitCentimeter;
		if (strUOM == "M")
			return unitMeter;
		if (strUOM == "LBS")
			return unitPound;
		if (strUOM == "GR")
			return unitGram;
		if (strUOM == "KG")
			return unitKilogram;
		if (strUOM == "C")
			return unitCelsius;
		if (strUOM == "F")
			return unitFarenheit;

		return unitGeneric;
	}



	void MetaColumn::MakeMetric()
	{
		switch (m_uUnit)
		{
			case unitInch:
				m_uUnit = unitCentimeter;
				break;
			case unitFoot:
			case unitYard:
				m_uUnit = unitMeter;
				break;
			case unitPound:
				m_uUnit = unitKilogram;
				break;
			case unitFarenheit:
				m_uUnit = unitCelsius;
				break;
		}
	}



	void MetaColumn::MakeImperial()
	{
		switch (m_uUnit)
		{
			case unitCentimeter:
				m_uUnit = unitInch;
				break;
			case unitMeter:
				m_uUnit = unitFoot;
				break;
			case unitGram:
			case unitKilogram:
				m_uUnit = unitPound;
				break;
			case unitCelsius:
				m_uUnit = unitFarenheit;
				break;
		}
	}



	std::string MetaColumn::AsCreateColumnSQL(TargetRDBMS eTarget)
	{
		std::string			strSQL;


		switch (eTarget)

		{
			case SQLServer:
			{
				strSQL  = "[";
				strSQL += m_strName;
				strSQL += "] ";
				strSQL += GetTypeAsSQLString(eTarget);
				strSQL += " ";

				if (IsAutoNum())
				{
					strSQL += "IDENTITY (1, 1) NOT NULL";
				}
				else
				{
					if (IsMandatory())
						strSQL += "NOT NULL";
					else
						strSQL += "NULL";
				}

				break;
			}

			case Oracle:
			{
				strSQL  = "";
				strSQL += m_strName;
				strSQL += " ";
				strSQL += GetTypeAsSQLString(eTarget);

				strSQL += " ";

				if (IsAutoNum())
				{
					strSQL += " NOT NULL";
				}
				else
				{
					if (IsMandatory())
						strSQL += "NOT NULL";
					else
						strSQL += "NULL";
				}
				break;
			}
		}
		return strSQL;
	}



	void MetaColumn::On_AddedToMetaKey(MetaKeyPtr pMetaKey)
	{
		assert(pMetaKey);
		assert(!HasMetaKey(pMetaKey));

		m_vectMetaKey.push_back(pMetaKey);

		if (pMetaKey->IsPrimary())
			m_bPKMember = true;
	}



	void MetaColumn::On_RemovedFromMetaKey(MetaKeyPtr pMetaKey)
	{
		unsigned int idx;


		assert(pMetaKey);

		// find it
		//
		for (idx = 0; idx < m_vectMetaKey.size(); idx++)
		{
			if (m_vectMetaKey[idx] == pMetaKey)
				break;
		}

		assert(idx < m_vectMetaKey.size());
		m_vectMetaKey[idx] = NULL;
	}



	void MetaColumn::On_MetaRelationCreated(MetaRelationPtr pMetaRelation)
	{
#ifdef _DEBUG
		assert(pMetaRelation);
		assert(pMetaRelation->GetSwitchColumn()->GetMetaColumn() == this);

		if (pMetaRelation->IsChildSwitch())
		{
			assert(pMetaRelation->GetChildMetaKey()->GetMetaEntity() == m_pMetaEntity);
		}
		else if (pMetaRelation->IsParentSwitch())
		{
			assert(pMetaRelation->GetParentMetaKey()->GetMetaEntity() == m_pMetaEntity);
		}
		else
		{
			assert(false);
		}

		// Assert if the relation is already a member
		for (unsigned int idx = 0; idx < m_vectMetaRelation.size(); idx++)
			assert(m_vectMetaRelation[idx] != pMetaRelation);
#endif

		m_vectMetaRelation.push_back(pMetaRelation);
	}



	void MetaColumn::On_MetaRelationDeleted(MetaRelationPtr pMetaRelation)
	{
		unsigned int idx;


		assert(pMetaRelation);
		assert(pMetaRelation->GetSwitchColumn()->GetMetaColumn() == this);

		// find it
		//
		for (idx = 0; idx < m_vectMetaRelation.size(); idx++)
		{
			if (m_vectMetaRelation[idx] == pMetaRelation)
				break;
		}

		assert(idx < m_vectMetaRelation.size());
		m_vectMetaRelation[idx] = NULL;
	}



	int MetaColumn::Compare(ObjectPtr pObj)
	{
		if (!pObj->IsA(IAm()))
		{
			ReportWarning("MetaColumn::Compare(): Can't compare a MetaColumn with a %s.", pObj->IAm().Name().c_str());
			return -1;
		}

		MetaColumnPtr pMC = (MetaColumnPtr) pObj;

		// Compare the types
		//
		if (GetType() < pMC->GetType())
			return -1;

		if (GetType() > pMC->GetType())
			return 1;

		// Types are identical, but if this is a string, compare the length also
		//
		if (GetType() == dbfString)
		{
			if (GetMaxLength() < pMC->GetMaxLength())
				return -1;

			if (GetMaxLength() > pMC->GetMaxLength())
				return 1;
		}


		return 0;
	}





	// ==========================================================================
	// MetaColumnString implementation
	//
	D3_CLASS_IMPL(MetaColumnString, MetaColumn);


	MetaColumnString::MetaColumnString(	MetaEntityPtr								pMetaEntity,
																			const std::string &					strName,
																			unsigned int								iMaxLength,
																			const Flags::Mask &					flags,
																			const Permissions::Mask &		permissions,
																			const std::string &					strInstanceClassName)
		: MetaColumn(pMetaEntity, M_uNextInternalID--, strName, flags, permissions, strInstanceClassName),
			m_iMaxLen(iMaxLength)
	{
		Init(strInstanceClassName, "ColumnString");
	}



	MetaColumnString::MetaColumnString(	MetaEntityPtr								pMetaEntity,
																			ColumnID										uID,
																			const std::string &					strName,
																			unsigned int								iMaxLength,
																			const Flags::Mask &					flags,
																			const Permissions::Mask &		permissions,
																			const std::string &					strInstanceClassName)
		: MetaColumn(pMetaEntity, uID, strName, flags, permissions, strInstanceClassName),
			m_iMaxLen(iMaxLength)
	{
		Init(strInstanceClassName, "ColumnString");
	}



	ColumnPtr MetaColumnString::CreateInstance(EntityPtr pEntity)
	{
		ColumnStringPtr	pCol = NULL;

		pCol = (ColumnStringPtr) m_pInstanceClass->CreateInstance();

		if (!pCol)
			throw Exception(__FILE__, __LINE__, Exception_error, "MetaColumnString::CreateInstance(): Class factory %s failed to create instance of column %s!", m_pInstanceClass->Name().c_str(), GetFullName().c_str());

		pCol->m_pMetaColumn = this;
		pCol->m_pEntity = pEntity;

		if (!m_bNullDefault)
			pCol->SetValue(m_strDefaultValue);

		pCol->On_PostCreate();

		return pCol;
	}




	std::string MetaColumnString::GetTypeAsSQLString(TargetRDBMS eTarget)
	{
		std::string			strType = "[UNDEFINED]";
		std::string			strLength;


		if (m_iMaxLen > D3_MAX_CONVENTIONAL_STRING_LENGTH)
		{
			switch (eTarget)
			{
				case SQLServer:
					strType  = "text";
					break;

				case Oracle:
					strType  = "CLOB";
					break;
			}
		}
		else
		{
			Convert(strLength, m_iMaxLen);

			switch (eTarget)
			{
				case SQLServer:
					strType  = "varchar(";
					strType += strLength;
					strType += ")";
					break;

				case Oracle:
					strType  = "varchar2(";
					strType += strLength;
					strType += ")";
					break;
			}
		}

		return strType;
	}



	int MetaColumnString::Compare(ObjectPtr pObj)
	{
		if (!pObj->IsKindOf(MetaColumnString::ClassObject()))
		{
			ReportWarning("MetaColumnString::Compare(): Can't compare ako MetaColumnString with a %s.", pObj->IAm().Name().c_str());
			return -1;
		}

		MetaColumnStringPtr pMC = (MetaColumnStringPtr) pObj;

		// Now compare the length
		//
		if (m_iMaxLen < pMC->m_iMaxLen)
			return -1;

		if (m_iMaxLen > pMC->m_iMaxLen)
			return 1;


		return 0;
	}






	// ==========================================================================
	// MetaColumnChar implementation
	//
	D3_CLASS_IMPL(MetaColumnChar, MetaColumn);


	MetaColumnChar::MetaColumnChar(			MetaEntityPtr								pMetaEntity,
																			const std::string &					strName,
																			const Flags::Mask & 				flags,
																			const Permissions::Mask &		permissions,
																			const std::string & 				strInstanceClassName)
		: MetaColumn(pMetaEntity, M_uNextInternalID--, strName, flags, permissions, strInstanceClassName)
	{
		Init(strInstanceClassName, "ColumnChar");
	}



	MetaColumnChar::MetaColumnChar(			MetaEntityPtr								pMetaEntity,
																			ColumnID										uID,
																			const std::string &					strName,
																			const Flags::Mask & 				flags,
																			const Permissions::Mask &		permissions,
																			const std::string & 				strInstanceClassName)
		: MetaColumn(pMetaEntity, uID, strName, flags, permissions, strInstanceClassName)
	{
		Init(strInstanceClassName, "ColumnChar");
	}



	ColumnPtr MetaColumnChar::CreateInstance(EntityPtr pEntity)
	{
		ColumnCharPtr	pCol = NULL;

		pCol = (ColumnCharPtr) m_pInstanceClass->CreateInstance();

		if (!pCol)
			throw Exception(__FILE__, __LINE__, Exception_error, "MetaColumnChar::CreateInstance(): Class factory %s failed to create instance of column %s!", m_pInstanceClass->Name().c_str(), GetFullName().c_str());

		pCol->m_pMetaColumn = this;
		pCol->m_pEntity = pEntity;

		if (!m_bNullDefault)
			pCol->SetValue(m_cDefaultValue);

		pCol->On_PostCreate();

		return pCol;
	}



	std::string MetaColumnChar::GetTypeAsSQLString(TargetRDBMS eTarget)
	{
		std::string			strType = "[UNDEFINED]";


		switch (eTarget)
		{
			case SQLServer:
				strType = "tinyint";
				break;

			case Oracle:
				strType = "NUMBER(3,0)";
				break;
		}

		return strType;
	}



	int MetaColumnChar::Compare(ObjectPtr pObj)
	{
		if (!pObj->IsKindOf(MetaColumnChar::ClassObject()))
		{
			ReportWarning("MetaColumnChar::Compare(): Can't compare ako MetaColumnChar with a %s.", pObj->IAm().Name().c_str());
			return -1;
		}

		return 0;
	}





	// ==========================================================================
	// MetaColumnShort implementation
	//
	D3_CLASS_IMPL(MetaColumnShort, MetaColumn);


	MetaColumnShort::MetaColumnShort(		MetaEntityPtr								pMetaEntity,
																			const std::string & 				strName,
																			const Flags::Mask &					flags,
																			const Permissions::Mask &		permissions,
																			const std::string & 				strInstanceClassName)
		: MetaColumn(pMetaEntity, M_uNextInternalID--, strName, flags, permissions, strInstanceClassName)
	{
		Init(strInstanceClassName, "ColumnShort");
	}



	MetaColumnShort::MetaColumnShort(		MetaEntityPtr								pMetaEntity,
																			ColumnID										uID,
																			const std::string & 				strName,
																			const Flags::Mask &					flags,
																			const Permissions::Mask &		permissions,
																			const std::string & 				strInstanceClassName)
		: MetaColumn(pMetaEntity, uID, strName, flags, permissions, strInstanceClassName)
	{
		Init(strInstanceClassName, "ColumnShort");
	}



	ColumnPtr MetaColumnShort::CreateInstance(EntityPtr pEntity)
	{
		ColumnShortPtr	pCol = NULL;

		pCol = (ColumnShortPtr) m_pInstanceClass->CreateInstance();

		if (!pCol)
			throw Exception(__FILE__, __LINE__, Exception_error, "MetaColumnShort::CreateInstance(): Class factory %s failed to create instance of column %s!", m_pInstanceClass->Name().c_str(), GetFullName().c_str());

		pCol->m_pMetaColumn = this;
		pCol->m_pEntity = pEntity;

		if (!m_bNullDefault)
			pCol->SetValue(m_sDefaultValue);

		pCol->On_PostCreate();

		return pCol;
	}



	std::string MetaColumnShort::GetTypeAsSQLString(TargetRDBMS eTarget)
	{
		std::string			strType = "[UNDEFINED]";


		switch (eTarget)
		{
			case SQLServer:
				strType = "smallint";
				break;

			case Oracle:
				strType = "NUMBER(5,0)";
				break;
		}

		return strType;
	}



	int MetaColumnShort::Compare(ObjectPtr pObj)
	{
		if (!pObj->IsKindOf(MetaColumnShort::ClassObject()))
		{
			ReportWarning("MetaColumnShort::Compare(): Can't compare ako MetaColumnShort with a %s.", pObj->IAm().Name().c_str());
			return -1;
		}

		return 0;
	}






	// ==========================================================================
	// MetaColumnBool implementation
	//
	D3_CLASS_IMPL(MetaColumnBool, MetaColumn);


	MetaColumnBool::MetaColumnBool(			MetaEntityPtr 							pMetaEntity,
																			const std::string & 				strName,
																			const Flags::Mask & 				flags,
																			const Permissions::Mask &		permissions,
																			const std::string & 				strInstanceClassName)
		: MetaColumn(pMetaEntity, M_uNextInternalID--, strName, flags, permissions, strInstanceClassName)
	{
		Init(strInstanceClassName, "ColumnBool");
	}



	MetaColumnBool::MetaColumnBool(			MetaEntityPtr 							pMetaEntity,
																			ColumnID										uID,
																			const std::string & 				strName,
																			const Flags::Mask & 				flags,
																			const Permissions::Mask &		permissions,
																			const std::string & 				strInstanceClassName)
		: MetaColumn(pMetaEntity, uID, strName, flags, permissions, strInstanceClassName)
	{
		Init(strInstanceClassName, "ColumnBool");
	}



	ColumnPtr MetaColumnBool::CreateInstance(EntityPtr pEntity)
	{
		ColumnBoolPtr	pCol = NULL;

		pCol = (ColumnBoolPtr) m_pInstanceClass->CreateInstance();

		if (!pCol)
			throw Exception(__FILE__, __LINE__, Exception_error, "MetaColumnBool::CreateInstance(): Class factory %s failed to create instance of column %s!", m_pInstanceClass->Name().c_str(), GetFullName().c_str());

		pCol->m_pMetaColumn = this;
		pCol->m_pEntity = pEntity;

		if (!m_bNullDefault)
			pCol->SetValue(m_bDefaultValue);

		pCol->On_PostCreate();

		return pCol;
	}



	std::string MetaColumnBool::GetTypeAsSQLString(TargetRDBMS eTarget)
	{
		std::string			strType = "[UNDEFINED]";


		switch (eTarget)
		{
			case SQLServer:
				strType = "bit";
				break;

			case Oracle:
				strType = "NUMBER(1)";
				break;
		}

		return strType;
	}



	int MetaColumnBool::Compare(ObjectPtr pObj)
	{
		if (!pObj->IsKindOf(MetaColumnBool::ClassObject()))
		{
			ReportWarning("MetaColumnBool::Compare(): Can't compare ako MetaColumnBool with a %s.", pObj->IAm().Name().c_str());
			return -1;
		}

		return 0;
	}





	// ==========================================================================
	// MetaColumnInt implementation
	//
	D3_CLASS_IMPL(MetaColumnInt, MetaColumn);


	MetaColumnInt::MetaColumnInt(				MetaEntityPtr 							pMetaEntity,
																			const std::string & 				strName,
																			const Flags::Mask & 				flags,
																			const Permissions::Mask &		permissions,
																			const std::string & 				strInstanceClassName)
		: MetaColumn(pMetaEntity, M_uNextInternalID--, strName, flags, permissions, strInstanceClassName)
	{
		Init(strInstanceClassName, "ColumnInt");
	}



	MetaColumnInt::MetaColumnInt(				MetaEntityPtr 							pMetaEntity,
																			ColumnID										uID,
																			const std::string & 				strName,
																			const Flags::Mask & 				flags,
																			const Permissions::Mask &		permissions,
																			const std::string & 				strInstanceClassName)
		: MetaColumn(pMetaEntity, uID, strName, flags, permissions, strInstanceClassName)
	{
		Init(strInstanceClassName, "ColumnInt");
	}



	ColumnPtr MetaColumnInt::CreateInstance(EntityPtr pEntity)
	{
		ColumnIntPtr	pCol = NULL;

		pCol = (ColumnIntPtr) m_pInstanceClass->CreateInstance();

		if (!pCol)
			throw Exception(__FILE__, __LINE__, Exception_error, "MetaColumnInt::CreateInstance(): Class factory %s failed to create instance of column %s!", m_pInstanceClass->Name().c_str(), GetFullName().c_str());

		pCol->m_pMetaColumn = this;
		pCol->m_pEntity = pEntity;

		if (!m_bNullDefault)
			pCol->SetValue(m_iDefaultValue);

		pCol->On_PostCreate();

		return pCol;
	}



	std::string MetaColumnInt::GetTypeAsSQLString(TargetRDBMS eTarget)
	{
		std::string			strType = "[UNDEFINED]";


		switch (eTarget)
		{
			case SQLServer:
				strType = "int";
				break;

			case Oracle:
				strType = "NUMBER(10,0)";
				break;
		}

		return strType;
	}



	int MetaColumnInt::Compare(ObjectPtr pObj)
	{
		if (!pObj->IsKindOf(MetaColumnInt::ClassObject()))
		{
			ReportWarning("MetaColumnInt::Compare(): Can't compare ako MetaColumnInt with a %s.", pObj->IAm().Name().c_str());
			return -1;
		}

		return 0;
	}







	// ==========================================================================
	// MetaColumnLong implementation
	//
	D3_CLASS_IMPL(MetaColumnLong, MetaColumn);


	MetaColumnLong::MetaColumnLong(			MetaEntityPtr								pMetaEntity,
																			const std::string & 				strName,
																			const Flags::Mask & 				flags,
																			const Permissions::Mask &		permissions,
																			const std::string & 				strInstanceClassName)
		: MetaColumn(pMetaEntity, M_uNextInternalID--, strName, flags, permissions, strInstanceClassName)
	{
		Init(strInstanceClassName, "ColumnLong");
	}



	MetaColumnLong::MetaColumnLong(			MetaEntityPtr								pMetaEntity,
																			ColumnID										uID,
																			const std::string & 				strName,
																			const Flags::Mask & 				flags,
																			const Permissions::Mask &		permissions,
																			const std::string & 				strInstanceClassName)
		: MetaColumn(pMetaEntity, uID, strName, flags, permissions, strInstanceClassName)
	{
		Init(strInstanceClassName, "ColumnLong");
	}



	ColumnPtr MetaColumnLong::CreateInstance(EntityPtr pEntity)
	{
		ColumnLongPtr	pCol = NULL;

		pCol = (ColumnLongPtr) m_pInstanceClass->CreateInstance();

		if (!pCol)
			throw Exception(__FILE__, __LINE__, Exception_error, "MetaColumnLong::CreateInstance(): Class factory %s failed to create instance of column %s!", m_pInstanceClass->Name().c_str(), GetFullName().c_str());

		pCol->m_pMetaColumn = this;
		pCol->m_pEntity = pEntity;

		if (!m_bNullDefault)
			pCol->SetValue(m_lDefaultValue);

		pCol->On_PostCreate();

		return pCol;
	}



	std::string MetaColumnLong::GetTypeAsSQLString(TargetRDBMS eTarget)
	{
		std::string			strType = "[UNDEFINED]";


		switch (eTarget)
		{
			case SQLServer:
				strType = "int";
				break;

			case Oracle:
				strType = "NUMBER(10,0)";
				break;
		}

		return strType;
	}



	int MetaColumnLong::Compare(ObjectPtr pObj)
	{
		if (!pObj->IsKindOf(MetaColumnLong::ClassObject()))
		{
			ReportWarning("MetaColumnLong::Compare(): Can't compare ako MetaColumnLong with a %s.", pObj->IAm().Name().c_str());
			return -1;
		}

		return 0;
	}







	// ==========================================================================
	// MetaColumnFloat implementation
	//
	D3_CLASS_IMPL(MetaColumnFloat, MetaColumn);


	MetaColumnFloat::MetaColumnFloat(		MetaEntityPtr								pMetaEntity,
																			const std::string &					strName,
																			const Flags::Mask &					flags,
																			const Permissions::Mask &		permissions,
																			const std::string &					strInstanceClassName)
		: MetaColumn(pMetaEntity, M_uNextInternalID--, strName, flags, permissions, strInstanceClassName)
	{
		Init(strInstanceClassName, "ColumnFloat");
	}



	MetaColumnFloat::MetaColumnFloat(		MetaEntityPtr								pMetaEntity,
																			ColumnID										uID,
																			const std::string &					strName,
																			const Flags::Mask &					flags,
																			const Permissions::Mask &		permissions,
																			const std::string &					strInstanceClassName)
		: MetaColumn(pMetaEntity, uID, strName, flags, permissions, strInstanceClassName)
	{
		Init(strInstanceClassName, "ColumnFloat");
	}



	ColumnPtr MetaColumnFloat::CreateInstance(EntityPtr pEntity)
	{
		ColumnFloatPtr	pCol = NULL;

		pCol = (ColumnFloatPtr) m_pInstanceClass->CreateInstance();

		if (!pCol)
			throw Exception(__FILE__, __LINE__, Exception_error, "MetaColumnFloat::CreateInstance(): Class factory %s failed to create instance of column %s!", m_pInstanceClass->Name().c_str(), GetFullName().c_str());

		pCol->m_pMetaColumn = this;
		pCol->m_pEntity = pEntity;

		if (!m_bNullDefault)
			pCol->SetValue(m_fDefaultValue);

		pCol->On_PostCreate();

		return pCol;
	}



	std::string MetaColumnFloat::GetTypeAsSQLString(TargetRDBMS eTarget)
	{
		std::string			strType = "[UNDEFINED]";


		switch (eTarget)
		{
			case SQLServer:
				strType = "real";
				break;

			case Oracle:
				strType = "FLOAT(24)";
				break;
		}

		return strType;
	}



	int MetaColumnFloat::Compare(ObjectPtr pObj)
	{
		if (!pObj->IsKindOf(MetaColumnFloat::ClassObject()))
		{
			ReportWarning("MetaColumnFloat::Compare(): Can't compare ako MetaColumnFloat with a %s.", pObj->IAm().Name().c_str());
			return -1;
		}

		return 0;
	}







	// ==========================================================================
	// MetaColumnDate implementation
	//
	D3_CLASS_IMPL(MetaColumnDate, MetaColumn);


	MetaColumnDate::MetaColumnDate(			MetaEntityPtr								pMetaEntity,
																			const std::string &					strName,
																			const Flags::Mask &					flags,
																			const Permissions::Mask &		permissions,
																			const std::string &					strInstanceClassName)
		: MetaColumn(pMetaEntity, M_uNextInternalID--, strName, flags, permissions, strInstanceClassName)
	{
		Init(strInstanceClassName, "ColumnDate");
	}



	MetaColumnDate::MetaColumnDate(			MetaEntityPtr								pMetaEntity,
																			ColumnID										uID,
																			const std::string &					strName,
																			const Flags::Mask &					flags,
																			const Permissions::Mask &		permissions,
																			const std::string &					strInstanceClassName)
		: MetaColumn(pMetaEntity, uID, strName, flags, permissions, strInstanceClassName)
	{
		Init(strInstanceClassName, "ColumnDate");
	}



	ColumnPtr MetaColumnDate::CreateInstance(EntityPtr pEntity)
	{
		ColumnDatePtr	pCol = NULL;

		pCol = (ColumnDatePtr) m_pInstanceClass->CreateInstance();

		if (!pCol)
			throw Exception(__FILE__, __LINE__, Exception_error, "MetaColumnDate::CreateInstance(): Class factory %s failed to create instance of column %s!", m_pInstanceClass->Name().c_str(), GetFullName().c_str());

		pCol->m_pMetaColumn = this;
		pCol->m_pEntity = pEntity;

		if (!m_bNullDefault)
			pCol->SetValue(m_dtDefaultValue);

		pCol->On_PostCreate();

		return pCol;
	}



	std::string MetaColumnDate::GetTypeAsSQLString(TargetRDBMS eTarget)
	{
		std::string			strType = "[UNDEFINED]";


		switch (eTarget)
		{
			case SQLServer:
				strType = "datetime";
				break;

			case Oracle:
				strType = "DATE";
				break;
		}

		return strType;
	}



	int MetaColumnDate::Compare(ObjectPtr pObj)
	{
		if (!pObj->IsKindOf(MetaColumnDate::ClassObject()))
		{
			ReportWarning("MetaColumnDate::Compare(): Can't compare ako MetaColumnDate with a %s.", pObj->IAm().Name().c_str());
			return -1;
		}

		return 0;
	}






	// ==========================================================================
	// MetaColumnBlob implementation
	//
	D3_CLASS_IMPL(MetaColumnBlob, MetaColumn);


	MetaColumnBlob::MetaColumnBlob(	MetaEntityPtr									pMetaEntity,
																	const std::string &						strName,
																	unsigned int									iMaxLength,
																	const Flags::Mask &						flags,
																	const Permissions::Mask &			permissions,
																	const std::string &						strInstanceClassName)
		: MetaColumn(pMetaEntity, M_uNextInternalID--, strName, flags, permissions, strInstanceClassName)
	{
		// Blob columns are always encoded
		m_Flags |= MetaColumn::Flags::EncodedValue;

		Init(strInstanceClassName, "ColumnBlob");
	}



	MetaColumnBlob::MetaColumnBlob(	MetaEntityPtr									pMetaEntity,
																	ColumnID											uID,
																	const std::string &						strName,
																	unsigned int									iMaxLength,
																	const Flags::Mask &						flags,
																	const Permissions::Mask &			permissions,
																	const std::string &						strInstanceClassName)
		: MetaColumn(pMetaEntity, uID, strName, flags, permissions, strInstanceClassName)
	{
		// Blob columns are always encoded
		m_Flags |= MetaColumn::Flags::EncodedValue;

		Init(strInstanceClassName, "ColumnBlob");
	}



	ColumnPtr MetaColumnBlob::CreateInstance(EntityPtr pEntity)
	{
		ColumnBlobPtr	pCol = NULL;

		pCol = (ColumnBlobPtr) m_pInstanceClass->CreateInstance();

		if (!pCol)
			throw Exception(__FILE__, __LINE__, Exception_error, "MetaColumnBlob::CreateInstance(): Class factory %s failed to create instance of column %s!", m_pInstanceClass->Name().c_str(), GetFullName().c_str());

		pCol->m_pMetaColumn = this;
		pCol->m_pEntity = pEntity;

		if (!m_bNullDefault)
			pCol->SetValue(m_strDefaultValue);

		pCol->On_PostCreate();

		return pCol;
	}



	std::string MetaColumnBlob::GetTypeAsSQLString(TargetRDBMS eTarget)
	{
		std::string			strType = "[UNDEFINED]";


		switch (eTarget)
		{
			case SQLServer:
				strType = "image";
				break;

			case Oracle:
				strType = "BLOB";
				break;
		}

		return strType;
	}



	int MetaColumnBlob::Compare(ObjectPtr pObj)
	{
		if (!pObj->IsKindOf(MetaColumnBlob::ClassObject()))
		{
			ReportWarning("MetaColumnBlob::Compare(): Can't compare ako MetaColumnBlob with a %s.", pObj->IAm().Name().c_str());
			return -1;
		}

		return 0;
	}






	// ==========================================================================
	// MetaColumnBinary implementation
	//
	D3_CLASS_IMPL(MetaColumnBinary, MetaColumn);


	MetaColumnBinary::MetaColumnBinary(	MetaEntityPtr							pMetaEntity,
																	const std::string &						strName,
																	unsigned int									iMaxLength,
																	const Flags::Mask &						flags,
																	const Permissions::Mask &			permissions,
																	const std::string &						strInstanceClassName)
		: MetaColumn(pMetaEntity, M_uNextInternalID--, strName, flags, permissions, strInstanceClassName),
			m_iMaxLen(iMaxLength)
	{
		// Binary Columns marked as Passwords contain 128bit MD5 CRC values
		if (m_Flags & Flags::Password)
		{
			m_iMaxLen = D3_PASSWORD_SIZE;
			m_Flags &= ~Flags::LazyFetch;
		}


		// Binary columns are always encoded
		m_Flags |= MetaColumn::Flags::EncodedValue;

		Init(strInstanceClassName, "ColumnBinary");
	}



	MetaColumnBinary::MetaColumnBinary(	MetaEntityPtr									pMetaEntity,
																	ColumnID											uID,
																	const std::string &						strName,
																	unsigned int									iMaxLength,
																	const Flags::Mask &						flags,
																	const Permissions::Mask &			permissions,
																	const std::string &						strInstanceClassName)
		: MetaColumn(pMetaEntity, uID, strName, flags, permissions, strInstanceClassName),
			m_iMaxLen(iMaxLength)
	{
		// Binary Columns marked as Passwords contain 128bit MD5 CRC values
		if (m_Flags & Flags::Password)
		{
			m_iMaxLen = D3_PASSWORD_SIZE;
			m_Flags &= ~Flags::LazyFetch;
		}

		// Binary columns are always encoded
		m_Flags |= MetaColumn::Flags::EncodedValue;

		Init(strInstanceClassName, "ColumnBinary");
	}



	ColumnPtr MetaColumnBinary::CreateInstance(EntityPtr pEntity)
	{
		ColumnBinaryPtr	pCol = NULL;

		pCol = (ColumnBinaryPtr) m_pInstanceClass->CreateInstance();

		if (!pCol)
			throw Exception(__FILE__, __LINE__, Exception_error, "MetaColumnBinary::CreateInstance(): Class factory %s failed to create instance of column %s!", m_pInstanceClass->Name().c_str(), GetFullName().c_str());

		pCol->m_pMetaColumn = this;
		pCol->m_pEntity = pEntity;

		if (!m_bNullDefault)
			pCol->SetValue((const unsigned char*) m_strDefaultValue.c_str(), m_strDefaultValue.length());

		pCol->On_PostCreate();

		return pCol;
	}



	std::string MetaColumnBinary::GetTypeAsSQLString(TargetRDBMS eTarget)
	{
		std::ostringstream					ostrm;

		switch (eTarget)
		{
			case SQLServer:
				ostrm << "varbinary(" << this->m_iMaxLen << ')';
				break;

			case Oracle:
				ostrm << "raw(" << this->m_iMaxLen << ')';
				break;
		}

		return ostrm.str();
	}



	int MetaColumnBinary::Compare(ObjectPtr pObj)
	{
		if (!pObj->IsKindOf(MetaColumnBinary::ClassObject()))
		{
			ReportWarning("MetaColumnBinary::Compare(): Can't compare ako MetaColumnBinary with a %s.", pObj->IAm().Name().c_str());
			return -1;
		}

		return 0;
	}






	// ==========================================================================
	// Column implementation
	//
	D3_CLASS_IMPL_PV(Column, Object);


	// Static member initialisation
	std::string		Column::M_strNull;
	char					Column::M_cNull		= 0;
	short					Column::M_sNull		= 0;
	bool					Column::M_bNull		= false;
	int						Column::M_iNull		= 0;
	long					Column::M_lNull		= 0;
	float					Column::M_fNull		= 0.0f;
	D3Date				Column::M_dtNull  = D3Date(boost::posix_time::ptime(boost::date_time::not_a_date_time));
	std::stringstream Column::M_blobNull;
	Data					Column::M_dataNull = Data();


	Column::~Column()
	{
		if (m_pEntity)
			m_pEntity->On_ColumnDeleted(this);

		delete m_pVectKey;
	}


	// Set the Column to NULL
	//
	bool Column::SetNull()
	{
		if (IsNull())
		{
			MarkDefined();
			return true;
		}

		// Check if the the update is allowed
		//
		if (NotifyBeforeUpdate())
		{
			bool	prvDirty(IsDirty());

			MarkNull();
			MarkDirty();

			if (NotifyAfterUpdate())
				return true;

			// Reset values because the update was declined
			//
			MarkNotNull();

			if (!prvDirty)
				MarkClean();
		}

		return false;
	}




	bool Column::SetValue(const Json::Value & jsonValue)
	{
		float			f = 0;


		if (jsonValue == Json::nullValue)
		{
			return SetNull();
		}
		else
		{
			switch (m_pMetaColumn->GetType())
			{
				case D3::MetaColumn::dbfString:
				case D3::MetaColumn::dbfDate:
					if (!jsonValue.isString())
					{
						ReportError("Column::SetValue(): Type of json data for column %s is not a string", m_pMetaColumn->GetFullName().c_str());
						return false;
					}
					else
					{
						if (m_pMetaColumn->IsEncodedValue())
						{
							int							length;
							bool						bResult;
							unsigned char*	pszDecoded = APALUtil::base64_decode(jsonValue.asString(), length);
							bResult = SetValue(std::string((char*) pszDecoded));
							delete [] pszDecoded;
							return bResult;
						}

						return SetValue(jsonValue.asString());
					}

				case D3::MetaColumn::dbfBinary:
					if (!jsonValue.isString())
					{
						ReportError("Column::SetValue(): Type of json data for column %s is not convertible to a string", m_pMetaColumn->GetFullName().c_str());
						return false;
					}
					else
					{
						int							length;
						bool						bResult;
						unsigned char*	pszDecoded = APALUtil::base64_decode(jsonValue.asString(), length);
						bResult = SetValue(pszDecoded, length);
						delete [] pszDecoded;
						return bResult;
					}

				case D3::MetaColumn::dbfBlob:
					if (!jsonValue.isString())
					{
						ReportError("Column::SetValue(): Type of json data for column %s is not convertible to a string", m_pMetaColumn->GetFullName().c_str());
						return false;
					}
					else
					{
						return SetValue(jsonValue.asString());
					}

				case D3::MetaColumn::dbfChar:
				case D3::MetaColumn::dbfShort:
				case D3::MetaColumn::dbfInt:
				case D3::MetaColumn::dbfLong:
					if (!jsonValue.isConvertibleTo(Json::intValue))
					{
						ReportError("Column::SetValue(): Type of json data for column %s is not convertible to an int", m_pMetaColumn->GetFullName().c_str());
						return false;
					}
					else
					{
						return SetValue(jsonValue.asInt());
					}

				case D3::MetaColumn::dbfFloat:
					if (!jsonValue.isConvertibleTo(Json::realValue))
					{
						ReportError("Column::SetValue(): Type of json data for column %s is not convertible to a real", m_pMetaColumn->GetFullName().c_str());
						return false;
					}
					else
					{
						f = (float) jsonValue.asDouble();
						return SetValue(f);
					}

				case D3::MetaColumn::dbfBool:
					if (!jsonValue.isConvertibleTo(Json::booleanValue))
					{
						ReportError("Column::SetValue(): Type of json data for column %s is not convertible to a boolean", m_pMetaColumn->GetFullName().c_str());
						return false;
					}
					else
					{
						return SetValue(jsonValue.asBool());
					}
			}
		}

		return false;
	}



	bool Column::SetValue(const double dNewValue, const std::string& strInUOM)
	{
		// \todo Currently, this code will convert unrelated meassurements (like say pound to centimeter)

		// First convert from InUOM to inches or pounds. Then convert inches or pounds into OutUOM
		double		dValue, dFactor = 1.0;

		if (strInUOM == "CM" || strInUOM == "CMT")
			dFactor = 0.3937;
		else if (strInUOM == "DMT")
			dFactor = 3.937;
		else if (strInUOM == "FT" || strInUOM == "FOT")
			dFactor = 12.0;
		else if (strInUOM == "GR" || strInUOM == "GRM" || strInUOM == "G")
			dFactor = 0.002205;
		else if (strInUOM == "IN3")
			;
		else if (strInUOM == "IN" || strInUOM == "INH")
			;
		else if (strInUOM == "KG" || strInUOM == "KGM")
			dFactor = 2.205;
		else if (strInUOM == "LB" || strInUOM == "LBS" || strInUOM == "LBR")
			;
		else if (strInUOM == "MG" || strInUOM == "MGM")
			dFactor = 0.000002205;
		else if (strInUOM == "MM" || strInUOM == "MMT")
			dFactor = 0.03937;
		else if (strInUOM == "M3")
			dFactor = 61023.377;
		else if (strInUOM == "M" || strInUOM == "MTR")
			dFactor = 39.37;
		else if (strInUOM == "OZ" || strInUOM == "ONZ")
			dFactor = 0.0625;
		else
		{
			std::ostringstream	msg;
			msg << "Column::SetValue: " << GetMetaColumn()->GetFullName() << " cannot convert value " << dNewValue << " from units " << strInUOM << ". The units are not known.";
			ReportWarning(msg.str().c_str());
			return false;
		}

		switch (GetMetaColumn()->GetUnit())
		{
			case MetaColumn::unitInch:
				break;
			case MetaColumn::unitFoot:
				dFactor /= 12.0;
				break;
			case MetaColumn::unitYard:
				dFactor /= 36.0;
				break;
			case MetaColumn::unitCentimeter:
				dFactor /= 0.3937;
				break;
			case MetaColumn::unitMeter:
				dFactor /= 39.37;
				break;
			case MetaColumn::unitPound:
				break;
			case MetaColumn::unitGram:
				dFactor /= 0.002205;
				break;
			case MetaColumn::unitKilogram:
				dFactor /= 2.205;
				break;
			default:
			{
				std::ostringstream	msg;
				msg << "Column::SetValue: " << GetMetaColumn()->GetFullName() << " cannot convert value " << dNewValue << " from units " << strInUOM << " to units " << GetMetaColumn()->UnitToString() << ". Check meta dictionary settings.";
				ReportWarning(msg.str().c_str());
				return false;
			}
		}

		// Success : input and output UOM are both recognized
		dValue = dNewValue * dFactor;
		return SetValue((float) dValue);
	}



	void Column::Dump(int nIndentSize, int iLW)
	{
		int iLabelWidth = iLW - m_pMetaColumn->GetName().size();


		if (iLabelWidth < 0)
			iLabelWidth = 0;

		if (IsNull())
		{
			std::cout << std::setw(nIndentSize) << "";
			std::cout << m_pMetaColumn->GetName();
			std::cout << std::setw(iLabelWidth) << "";
			std::cout << " [type/value]: [" << IAm().Name() << "/" << "NULL" << "]" << std::endl;
		}
		else
		{
			if (IsKindOf(ColumnChar::ClassObject()) && !m_pMetaColumn->IsSingleChoice())
			{
				int i = AsInt();
				std::cout << std::setw(nIndentSize) << "";
				std::cout << m_pMetaColumn->GetName();
				std::cout << std::setw(iLabelWidth) << "";
				std::cout << " [type/value]: [" << IAm().Name() << "/" << i << "]" << std::endl;
			}
			else
			{
				std::cout << std::setw(nIndentSize) << "";
				std::cout << m_pMetaColumn->GetName();
				std::cout << std::setw(iLabelWidth) << "";
				std::cout << " [type/value]: [" << IAm().Name() << "/" << AsString() << "]" << std::endl;
			}
		}
	}



	// Functionality to deal with keys
	//
	void Column::On_AddedToKey(KeyPtr pKey)
	{
		unsigned int		idx;

		assert(pKey);
		assert(IsKeyMember());
		assert(m_pVectKey);
		idx = m_pMetaColumn->GetMetaKeyIndex(pKey->GetMetaKey());
		assert(idx < m_pVectKey->size());
		(*m_pVectKey)[idx] = pKey;
	}



	// Functionality to deal with keys
	//
	void Column::On_RemovedFromKey(KeyPtr pKey)
	{
		unsigned int		idx;

		assert(pKey);
		assert(IsKeyMember());
		assert(m_pVectKey);
		idx = m_pMetaColumn->GetMetaKeyIndex(pKey->GetMetaKey());
		assert(idx < m_pVectKey->size());
		assert((*m_pVectKey)[idx] == pKey);
		(*m_pVectKey)[idx] = NULL;
	}



	// Returns true if the key is one that this knows
	//
	bool Column::HasKey(KeyPtr pKey)
	{
		unsigned int		idx;

		assert(pKey);

		if (m_pVectKey)
		{
			for (idx = 0; idx < m_pVectKey->size(); idx++)
			{
				if ((*m_pVectKey)[idx] == pKey)
					return true;
			}
		}

		return false;
	}



	bool Column::NotifyBeforeUpdate()
	{
		if (!m_pEntity || m_pEntity->IsConstructing() || m_pEntity->IsPopulating())
			return true;

		// Let the entity give approval first
		//
		if (!m_pEntity->On_BeforeUpdateColumn(this))
			return false;

		// Make sure we send notifications to the relevant keys if this
		// is a relation switch column
		//
		if (!m_pMetaColumn->GetSwitchedMetaRelations().empty())
		{
			unsigned int					idx;
			MetaRelationPtr				pMetaRelation;
			RelationPtr						pRelation;
			InstanceKeyPtr				pKey;
			InstanceKeyPtrSet			setKey;					// list of processed keys


			for (idx = 0; idx < m_pMetaColumn->GetSwitchedMetaRelations().size(); idx++)
			{
				pMetaRelation = m_pMetaColumn->GetSwitchedMetaRelations()[idx];

				if (!pMetaRelation)
					continue;

				pKey = NULL;

				if (pMetaRelation->IsChildSwitch())
				{
					pRelation	= m_pEntity->GetParentRelation(pMetaRelation);

					if (pRelation)
						pKey = m_pEntity->GetInstanceKey(pMetaRelation->GetChildMetaKey());
				}

				if (pMetaRelation->IsParentSwitch())
				{
					pRelation	= m_pEntity->GetChildRelation(pMetaRelation);

					if (pRelation)
						pKey = pRelation->GetParentKey();
				}

				if (pKey)
				{
					// Only send a notification if this is not a key column
					//
					if (!HasKey(pKey))
					{
						// Never send a notification to the same key more than once
						//
						if (setKey.find(pKey) == setKey.end())
						{
							if (!pKey->On_BeforeUpdateColumn(this))
								return false;

							setKey.insert(pKey);
						}
					}
				}
			}
		}

		// Deal with key columns
		//
		if (IsKeyMember())
		{
			unsigned int		idx;
			KeyPtr					pKey;

			for (idx = 0; idx < m_pVectKey->size(); idx++)
			{
				pKey = (*m_pVectKey)[idx];

				if (pKey && !pKey->On_BeforeUpdateColumn(this))
					return false;
			}
		}

		return true;
	}



	bool Column::NotifyAfterUpdate()
	{
		if (!m_pEntity || m_pEntity->IsConstructing() || m_pEntity->IsPopulating())
		{
			MarkDefined();
			return true;
		}

		// Let the entity give approval first
		//
		if (!m_pEntity->On_AfterUpdateColumn(this))
			return false;

		// Make sure we send notifications to the relevant keys if this
		// is a relation switch column
		//
		if (!m_pMetaColumn->GetSwitchedMetaRelations().empty())
		{
			unsigned int					idx;
			MetaRelationPtr				pMetaRelation;
			RelationPtr						pRelation;
			InstanceKeyPtr				pKey;
			InstanceKeyPtrSet			setKey;					// list of processed keys


			for (idx = 0; idx < m_pMetaColumn->GetSwitchedMetaRelations().size(); idx++)
			{
				pMetaRelation = m_pMetaColumn->GetSwitchedMetaRelations()[idx];

				if (!pMetaRelation)
					continue;

				pKey = NULL;

				if (pMetaRelation->IsChildSwitch())
				{
					pRelation	= m_pEntity->GetParentRelation(pMetaRelation);

					if (pRelation)
						pKey = m_pEntity->GetInstanceKey(pMetaRelation->GetChildMetaKey());
				}

				if (pMetaRelation->IsParentSwitch())
				{
					pRelation	= m_pEntity->GetChildRelation(pMetaRelation);

					if (pRelation)
						pKey = pRelation->GetParentKey();
				}

				if (pKey)
				{
					// Only send a notification if this is not a key column
					//
					if (!HasKey(pKey))
					{
						// Never send a notification to the same key more than once
						//
						if (setKey.find(pKey) == setKey.end())
						{
							if (!pKey->On_AfterUpdateColumn(this))
								return false;

							setKey.insert(pKey);
						}
					}
				}
			}
		}

		// Deal with key columns
		//
		if (IsKeyMember())
		{
			unsigned int		idx;
			KeyPtr					pKey;

			for (idx = 0; idx < m_pVectKey->size(); idx++)
			{
				pKey = (*m_pVectKey)[idx];

				if (pKey && !pKey->On_AfterUpdateColumn(this))
					return false;
			}
		}

		MarkDefined();

		return true;
	}



	// This notification is sent by the associated MetaColumn's CreateInstance() method.
	//
	void Column::On_PostCreate()
	{
		// Sanity checks
		//
		assert(m_pMetaColumn);

		// Deal with key columns
		//
		if (IsKeyMember())
		{
			unsigned int		idx;

			m_pVectKey = new KeyPtrVect;

			m_pVectKey->reserve(m_pMetaColumn->GetMetaKeys().size());

			for (idx = 0; idx < m_pMetaColumn->GetMetaKeys().size(); idx++)
				m_pVectKey->push_back(NULL);
		}

		m_pMetaColumn->On_InstanceCreated(this);

		if (m_pEntity)
			m_pEntity->On_ColumnCreated(this);
	}



	std::ostream & Column::AsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm, bool * pFirstSibling)
	{
		RolePtr										pRole = pRoleUser ? pRoleUser->GetRole() : NULL;
		MetaColumn::Permissions		permissions = (!m_pMetaColumn->IsDerived() && pRole) ? pRole->GetPermissions(m_pMetaColumn) : m_pMetaColumn->GetPermissions();

		if (permissions & MetaColumn::Permissions::Read)
		{
			if (pFirstSibling)
			{
				if (*pFirstSibling)
					*pFirstSibling = false;
				else
					ostrm << ',';
			}

			ostrm << AsJSON(pRoleUser);
		}

		return ostrm;
	}




	//! Fetches this value if it is LazyFetch and not yet marked as fetched
	void Column::Fetch()
	{
		if (m_pMetaColumn->IsLazyFetch() && !IsFetched())
		{
			if (m_pEntity)
			{
				if (m_pEntity->GetDatabase()->LoadColumn(this))
					MarkFetched();
			}
		}
	}









	// ==========================================================================
	// ColumnString implementation
	//
	D3_CLASS_IMPL(ColumnString, Column);

	// Convert this' value so that it can be use in SQL commands such as WHERE predicates and
	// SET clauses.
	// If this is NULL, returns the string "NULL"..
	// The function parses this' value and builds a new std::string that contains two single
	// quotes for each single quote that is encountered in the original std::string.
	// Furthermore, the returned string is embedded in single quotes.
	//
	// Example:
	//
	// original: "This' size attribute's value"
	// returns:  "'This'' size attribute''s value'"
	//
	std::string ColumnString::AsSQLString()
	{
		std::string		strTemp = "NULL";
		const char		cQuote = '\'';
		const char*		pszChar;


		if (IsNull())
			return strTemp;

		// Truncate the string if it is too long but issue a warning
		//
		if (m_pMetaColumn->GetMaxLength() < m_strValue.size())
		{
			m_strValue.resize(m_pMetaColumn->GetMaxLength());
			ReportWarning("ColumnString::AsSQLString(): String column %s.%s.%s has a maximum length of %u, value '%s' has been truncated.", m_pMetaColumn->GetMetaEntity()->GetMetaDatabase()->GetName().c_str(), m_pMetaColumn->GetMetaEntity()->GetName().c_str(), m_pMetaColumn->GetName().c_str(), m_pMetaColumn->GetMaxLength(), m_strValue.substr(0,512).c_str());
			MarkDirty();
		}

		strTemp = cQuote;

		// Translate single quotes to two single quotes
		//
		for (pszChar = m_strValue.c_str(); *pszChar; pszChar++)
		{
			if (*pszChar == cQuote)
			{
				strTemp += cQuote;
				strTemp += cQuote;
			}
			else
			{
				strTemp += *pszChar;
			}
		}

		strTemp += cQuote;

		return strTemp;
	}



	// Type translators
	//
	std::string ColumnString::AsString(bool bHumanReadable)
	{
		if (IsNull())
			return M_strNull;

		if (bHumanReadable)
		{
			if (m_pMetaColumn->IsSingleChoice())
				return m_pMetaColumn->ChoiceDescAsString(m_strValue);
		}

		return m_strValue;
	}



	char ColumnString::AsChar()
	{
		char		valOut = M_cNull;

		if (!IsNull())
			Convert(valOut, m_strValue);

		return valOut;
	}



	bool ColumnString::AsBool()
	{
		bool			valOut = M_bNull;

		if (!IsNull())
			Convert(valOut, m_strValue);

		return valOut;
	}



	short ColumnString::AsShort()
	{
		short			valOut = M_sNull;

		if (!IsNull())
			Convert(valOut, m_strValue);

		return valOut;
	}



	int ColumnString::AsInt()
	{
		int			valOut = M_iNull;

		if (!IsNull())
			Convert(valOut, m_strValue);

		return valOut;
	}



	long ColumnString::AsLong()
	{
		long			valOut = M_lNull;

		if (!IsNull())
			Convert(valOut, m_strValue);

		return valOut;
	}



	float ColumnString::AsFloat()
	{
		float			valOut = M_fNull;

		if (!IsNull())
			Convert(valOut, m_strValue);

		return valOut;
	}


	D3Date ColumnString::AsDate()
	{
		D3Date			valOut = M_dtNull;

		if (!IsNull())
			Convert(valOut, m_strValue);

		return valOut;
	}



	bool ColumnString::SetValue(const std::string & strVal)
	{
		std::string							strPrevValue(m_strValue);
		std::string							strValue(strVal);
		bool										bPrevNull(IsNull());
		bool										bPrevDirty(IsDirty());
		std::string::size_type	posLastNonBlank = strValue.find_last_not_of(' ');


		// Remove trailing spaces (SQL Server can't handle this)
		if (posLastNonBlank != std::string::npos && posLastNonBlank < strValue.size() - 1)
		{
#ifdef _DEBUG
			ReportWarning("ColumnString::SetValue(): Removing trailing spaces from value '%s' for column %s!", strValue.c_str(), m_pMetaColumn->GetFullName().c_str());
#endif
			strValue.erase(posLastNonBlank + 1);
		}

		// If the string is empty, treat this like NULL (ORACLE can't handle empty strings)
		if (strValue.empty())
			return SetNull();

		if (strValue.size() > m_pMetaColumn->GetMaxLength())
		{
#ifdef _DEBUG
			ReportWarning("ColumnString::SetValue(): Value '%s' exceeds the limit for column %s, value will be truncated.", strValue.c_str(), m_pMetaColumn->GetFullName().c_str());
#endif
			strValue.resize(m_pMetaColumn->GetMaxLength());
		}

		if (!bPrevNull && m_strValue == strValue)
		{
			MarkDefined();
			return true;
		}

		// Check if the the update is allowed
		//
		if (NotifyBeforeUpdate())
		{
			m_strValue = strValue;
			MarkNotNull();
			MarkDirty();

			if (NotifyAfterUpdate())
				return true;

			// Reset values because the update was declined
			//
			m_strValue = strPrevValue;

			if (bPrevNull)
				MarkNull();

			if (!bPrevDirty)
				MarkClean();
		}

		return false;
	}



	bool ColumnString::SetValue(const char & valIn)
	{
		std::string			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnString::SetValue(const short & valIn)
	{
		std::string			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnString::SetValue(const bool & valIn)
	{
		std::string			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnString::SetValue(const int & valIn)
	{
		std::string			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnString::SetValue(const long & valIn)
	{
		std::string			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnString::SetValue(const float & valIn)
	{
		std::string			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnString::SetValue(const D3Date & valIn)
	{
		std::string			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	// Unsupported typecast operators assert
	ColumnString::operator const std::string &() const
	{
		if (IsNull())
			return M_strNull;

		return m_strValue;
	}



	bool ColumnString::Assign(Column & aFld)
	{
		bool	bSuccess;


		if (aFld.IsNull())
		{
			bSuccess = SetNull();
		}
		else
		{
			if (aFld.IsKindOf(ColumnString::ClassObject()))
				bSuccess = SetValue(((ColumnString&) aFld).m_strValue);
			else
				bSuccess = SetValue(aFld.AsString());
		}

		return bSuccess;
	}



	// This method returns true if the Column's value is within the range of this'
	// value plus minus fVariation (which is a percentage figure, e.g. 5.5F is 5.5%)
	//
	bool ColumnString::IsApproximatelyEqual(Column & aCol, float fVariation)
	{
		if (!aCol.IsKindOf(ColumnString::ClassObject()))
			return false;		// different types

		// Currently strings are either equal or not, but maybe one day we'll implement some similarity test instead
		return Compare(&aCol) == 0;
	}



	int ColumnString::Compare(ObjectPtr pObj)
	{
		if (!pObj || !pObj->IsKindOf(ColumnString::ClassObject()))
			throw Exception(__FILE__, __LINE__,  Exception_error, "ColumnString::Compare(): Can't compare column %s with object which is not ako Column!", m_pMetaColumn->GetFullName().c_str());

		ColumnStringPtr pCol = (ColumnStringPtr) pObj;

		if (IsNull() && pCol->IsNull())
			return 0;

		if (IsNull() != pCol->IsNull())
			return IsNull() ? -1 : 1;

		return Compare(pCol->m_strValue);
	}



	int ColumnString::Compare(const std::string & val)
	{
		if (IsNull())
			return -1;

		if (m_strValue < val)
			return -1;

		if (m_strValue > val)
			return 1;

		return 0;
	}



	std::string ColumnString::AsJSON(RoleUserPtr pRoleUser)
	{
		std::string		strOut;

		if (IsNull())
			strOut = "null";
		else
		{
			if (m_pMetaColumn->IsEncodedValue())
			{
				std::string		strBase64;

				APALUtil::base64_encode(strBase64, (const unsigned char *) m_strValue.c_str(), m_strValue.size());

				strOut = '"';
				strOut += strBase64;
				strOut += '"';
			}
			else
			{
				strOut = '"';
				strOut += JSONEncode(m_strValue);
				strOut += '"';
			}
		}

		return strOut;
	}








	// ==========================================================================
	// ColumnChar implementation
	//
	D3_CLASS_IMPL(ColumnChar, Column);

	// Convert this' value so that it can be use in SQL commands such as WHERE predicates and
	// SET clauses.
	// If this is NULL, returns the string "NULL", otherwise it returns AsString()
	//
	std::string ColumnChar::AsSQLString()
	{
		if (IsNull())
			return "NULL";

		return AsString(false);
	}



	// Type translators
	//
	std::string ColumnChar::AsString(bool bHumanReadable)
	{
		if (IsNull())
			return M_strNull;

		std::string		valOut;

		if (bHumanReadable)
		{
			if (m_pMetaColumn->IsSingleChoice())
			{
				Convert(valOut, m_cValue);
				return m_pMetaColumn->ChoiceDescAsString(valOut);
			}
			else
			{
				if (m_pMetaColumn->IsMultiChoice())
					return m_pMetaColumn->ChoiceDescsAsString((unsigned long) m_cValue);
			}
		}

		Convert(valOut, m_cValue);

		return valOut;
	}



	char ColumnChar::AsChar()
	{
		if (!IsNull())
			return m_cValue;

		return M_cNull;
	}



	bool ColumnChar::AsBool()
	{
		bool			valOut = M_bNull;

		if (!IsNull())
			Convert(valOut, m_cValue);

		return valOut;
	}



	short ColumnChar::AsShort()
	{
		short			valOut = M_sNull;

		if (!IsNull())
			Convert(valOut, m_cValue);

		return valOut;
	}



	int ColumnChar::AsInt()
	{
		int			valOut = M_iNull;

		if (!IsNull())
			Convert(valOut, m_cValue);

		return valOut;
	}



	long ColumnChar::AsLong()
	{
		long			valOut = M_lNull;

		if (!IsNull())
			Convert(valOut, m_cValue);

		return valOut;
	}



	float ColumnChar::AsFloat()
	{
		float			valOut = M_fNull;

		if (!IsNull())
			Convert(valOut, m_cValue);

		return valOut;
	}


	D3Date ColumnChar::AsDate()
	{
		assert(false);
		return M_dtNull;
	}



	bool ColumnChar::SetValue(const std::string & valIn)
	{
		char			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnChar::SetValue(const char & cValue)
	{
		char						cPrevValue(m_cValue);
		bool						bPrevNull(IsNull());
		bool						bPrevDirty(IsDirty());


		if (!bPrevNull && m_cValue == cValue)
		{
			MarkDefined();
			return true;
		}

		// Check if the the update is allowed
		//
		if (NotifyBeforeUpdate())
		{
			m_cValue = cValue;
			MarkNotNull();
			MarkDirty();

			if (NotifyAfterUpdate())
				return true;

			// Reset values because the update was declined
			//
			m_cValue = cPrevValue;

			if (bPrevNull)
				MarkNull();

			if (!bPrevDirty)
				MarkClean();
		}

		return false;
	}



	bool ColumnChar::SetValue(const short & valIn)
	{
		char			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnChar::SetValue(const bool & valIn)
	{
		char			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnChar::SetValue(const int & valIn)
	{
		char			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnChar::SetValue(const long & valIn)
	{
		char			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnChar::SetValue(const float & valIn)
	{
		char			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnChar::SetValue(const D3Date & valIn)
	{
		char			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	// Unsupported typecast operators assert
	ColumnChar::operator const char &() const
	{
		if (IsNull())
			return M_cNull;

		return m_cValue;
	}



	bool ColumnChar::Assign(Column & aFld)
	{
		bool	bSuccess;


		if (aFld.IsNull())
		{
			bSuccess = SetNull();
		}
		else
		{
			if (aFld.IsKindOf(ColumnChar::ClassObject()))
				bSuccess = SetValue(((ColumnChar&) aFld).m_cValue);
			else
				bSuccess = SetValue(aFld.AsChar());
		}

		return bSuccess;
	}



	// This method returns true if the Column's value is within the range of this'
	// value plus minus fVariation (which is a percentage figure, e.g. 5.5F is 5.5%)
	//
	bool ColumnChar::IsApproximatelyEqual(Column & aCol, float fVariation)
	{
		if (!aCol.IsKindOf(ColumnChar::ClassObject()))
			return false; // different types

		if (IsNull() != aCol.IsNull())
			return false; // One NULL and one not NULL

		if (IsNull())
			return true; // both NULL

		// Currently strings are either equal or not, but maybe one day we'll implement some similarity test instead
		return approx_equal(m_cValue, ((ColumnChar) aCol).m_cValue, fVariation);
	}



	int ColumnChar::Compare(ObjectPtr pObj)
	{
		if (!pObj || !pObj->IsKindOf(ColumnChar::ClassObject()))
			throw Exception(__FILE__, __LINE__,  Exception_error, "ColumnChar::Compare(): Can't compare ColumnChar %s with object which is not ako ColumnChar!", m_pMetaColumn->GetFullName().c_str());

		ColumnCharPtr pCol = (ColumnCharPtr) pObj;

		if (IsNull() && pCol->IsNull())
			return 0;

		if (pCol->IsNull())
			return 1;

		return Compare(pCol->m_cValue);
	}



	int ColumnChar::Compare(const char & val)
	{
		if (IsNull())
			return -1;

		if (m_cValue < val)
			return -1;

		if (m_cValue > val)
			return 1;

		return 0;
	}



	std::string ColumnChar::AsJSON(RoleUserPtr pRoleUser)
	{
		std::string		strOut;

		if (IsNull())
			strOut = "null";
		else
			Convert(strOut, m_cValue);

		return strOut;
	}








	// ==========================================================================
	// ColumnShort implementation
	//
	D3_CLASS_IMPL(ColumnShort, Column);

	// Convert this' value so that it can be use in SQL commands such as WHERE predicates and
	// SET clauses.
	// If this is NULL, returns the string "NULL", otherwise it returns AsString()
	//
	std::string ColumnShort::AsSQLString()
	{
		if (IsNull())
			return "NULL";

		return AsString(false);
	}



	// Type translators
	//
	std::string ColumnShort::AsString(bool bHumanReadable)
	{
		if (IsNull())
			return M_strNull;

		std::string		valOut;

		if (bHumanReadable)
		{
			if (m_pMetaColumn->IsSingleChoice())
			{
				Convert(valOut, m_sValue);
				return m_pMetaColumn->ChoiceDescAsString(valOut);
			}
			else
			{
				if (m_pMetaColumn->IsMultiChoice())
					return m_pMetaColumn->ChoiceDescsAsString((unsigned long) m_sValue);
			}
		}

		Convert(valOut, m_sValue);

		return valOut;
	}



	char ColumnShort::AsChar()
	{
		char			valOut = M_cNull;

		if (!IsNull())
			Convert(valOut, m_sValue);

		return valOut;
	}



	short ColumnShort::AsShort()
	{
		if (!IsNull())
			return m_sValue;

		return M_sNull;
	}



	bool ColumnShort::AsBool()
	{
		bool			valOut = M_bNull;

		if (!IsNull())
			Convert(valOut, m_sValue);

		return valOut;
	}



	int ColumnShort::AsInt()
	{
		int			valOut = M_iNull;

		if (!IsNull())
			Convert(valOut, m_sValue);

		return valOut;
	}



	long ColumnShort::AsLong()
	{
		long			valOut = M_lNull;

		if (!IsNull())
			Convert(valOut, m_sValue);

		return valOut;
	}



	float ColumnShort::AsFloat()
	{
		float			valOut = M_fNull;

		if (!IsNull())
			Convert(valOut, m_sValue);

		return valOut;
	}


	D3Date ColumnShort::AsDate()
	{
		assert(false);
		return M_dtNull;
	}



	bool ColumnShort::SetValue(const std::string & valIn)
	{
		short			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnShort::SetValue(const char & valIn)
	{
		short			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnShort::SetValue(const short & sValue)
	{
		short						sPrevValue(m_sValue);
		bool						bPrevNull(IsNull());
		bool						bPrevDirty(IsDirty());


		if (!bPrevNull && m_sValue == sValue)
		{
			MarkDefined();
			return true;
		}

		// Check if the the update is allowed
		//
		if (NotifyBeforeUpdate())
		{
			m_sValue = sValue;
			MarkNotNull();
			MarkDirty();

			if (NotifyAfterUpdate())
				return true;

			// Reset values because the update was declined
			//
			m_sValue = sPrevValue;

			if (bPrevNull)
				MarkNull();

			if (!bPrevDirty)
				MarkClean();
		}

		return false;
	}



	bool ColumnShort::SetValue(const bool & valIn)
	{
		short			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnShort::SetValue(const int & valIn)
	{
		short			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnShort::SetValue(const long & valIn)
	{
		short			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnShort::SetValue(const float & valIn)
	{
		short			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnShort::SetValue(const D3Date & valIn)
	{
		short			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	// Unsupported typecast operators assert
	ColumnShort::operator const short &() const
	{
		if (IsNull())
			return M_sNull;

		return m_sValue;
	}



	bool ColumnShort::Assign(Column & aFld)
	{
		bool	bSuccess;


		if (aFld.IsNull())
		{
			bSuccess = SetNull();
		}
		else
		{
			if (aFld.IsKindOf(ColumnShort::ClassObject()))
				bSuccess = SetValue(((ColumnShort&) aFld).m_sValue);
			else
				bSuccess = SetValue(aFld.AsShort());
		}

		return bSuccess;
	}



	// This method returns true if the Column's value is within the range of this'
	// value plus minus fVariation (which is a percentage figure, e.g. 5.5F is 5.5%)
	//
	bool ColumnShort::IsApproximatelyEqual(Column & aCol, float fVariation)
	{
		if (!aCol.IsKindOf(ColumnShort::ClassObject()))
			return false; // different types

		if (IsNull() != aCol.IsNull())
			return false; // One NULL and one not NULL

		if (IsNull())
			return true; // both NULL

		// Currently strings are either equal or not, but maybe one day we'll implement some similarity test instead
		return approx_equal(m_sValue, ((ColumnShort) aCol).m_sValue, fVariation);
	}



	int ColumnShort::Compare(ObjectPtr pObj)
	{
		if (!pObj || !pObj->IsKindOf(ColumnShort::ClassObject()))
			throw Exception(__FILE__, __LINE__,  Exception_error, "ColumnShort::Compare(): Can't compare ColumnShort %s with object which is not ako ColumnShort!", m_pMetaColumn->GetFullName().c_str());

		ColumnShortPtr pCol = (ColumnShortPtr) pObj;

		if (IsNull() && pCol->IsNull())
			return 0;

		if (pCol->IsNull())
			return 1;

		return Compare(pCol->m_sValue);
	}



	int ColumnShort::Compare(const short & val)
	{
		if (IsNull())
			return -1;

		if (m_sValue < val)
			return -1;

		if (m_sValue > val)
			return 1;

		return 0;
	}



	std::string ColumnShort::AsJSON(RoleUserPtr pRoleUser)
	{
		std::string		strOut;

		if (IsNull())
			strOut = "null";
		else
			Convert(strOut, m_sValue);

		return strOut;
	}








	// ==========================================================================
	// ColumnBool implementation
	//
	D3_CLASS_IMPL(ColumnBool, Column);

	// Convert this' value so that it can be use in SQL commands such as WHERE predicates and
	// SET clauses.
	// If this is NULL, returns the string "NULL", otherwise it returns AsString()
	//
	std::string ColumnBool::AsSQLString()
	{
		if (IsNull())
			return "NULL";

		return AsString(false);
	}



	// Type translators
	//
	std::string ColumnBool::AsString(bool bHumanReadable)
	{
		if (IsNull())
			return M_strNull;

		std::string		valOut;

		if (bHumanReadable)
		{
			if (m_pMetaColumn->IsSingleChoice())
			{
				Convert(valOut, m_bValue);
				return m_pMetaColumn->ChoiceDescAsString(valOut);
			}
		}

		Convert(valOut, m_bValue);

		return valOut;
	}



	char ColumnBool::AsChar()
	{
		char			valOut = M_cNull;

		if (!IsNull())
			Convert(valOut, m_bValue);

		return valOut;
	}



	short ColumnBool::AsShort()
	{
		short			valOut = M_sNull;

		if (!IsNull())
			Convert(valOut, m_bValue);

		return valOut;
	}



	bool ColumnBool::AsBool()
	{
		if (!IsNull())
			return m_bValue;

		return M_bNull;
	}



	int ColumnBool::AsInt()
	{
		int			valOut = M_iNull;

		if (!IsNull())
			Convert(valOut, m_bValue);

		return valOut;
	}



	long ColumnBool::AsLong()
	{
		long			valOut = M_lNull;

		if (!IsNull())
			Convert(valOut, m_bValue);

		return valOut;
	}



	float ColumnBool::AsFloat()
	{
		float			valOut = M_fNull;

		if (!IsNull())
			Convert(valOut, m_bValue);

		return valOut;
	}


	D3Date ColumnBool::AsDate()
	{
		assert(false);
		return M_dtNull;
	}



	bool ColumnBool::SetValue(const std::string & valIn)
	{
		bool			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnBool::SetValue(const char & valIn)
	{
		bool			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnBool::SetValue(const short & valIn)
	{
		bool			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnBool::SetValue(const bool & bValue)
	{
		bool						bPrevValue(m_bValue);
		bool						bPrevNull(IsNull());
		bool						bPrevDirty(IsDirty());


		if (!bPrevNull && m_bValue == bValue)
		{
			MarkDefined();
			return true;
		}

		// Check if the the update is allowed
		//
		if (NotifyBeforeUpdate())
		{
			m_bValue = bValue;
			MarkNotNull();
			MarkDirty();

			if (NotifyAfterUpdate())
				return true;

			// Reset values because the update was declined
			//
			m_bValue = bPrevValue;

			if (bPrevNull)
				MarkNull();

			if (!bPrevDirty)
				MarkClean();
		}

		return false;
	}



	bool ColumnBool::SetValue(const int & valIn)
	{
		bool			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnBool::SetValue(const long & valIn)
	{
		bool			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnBool::SetValue(const float & valIn)
	{
		bool			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnBool::SetValue(const D3Date & valIn)
	{
		bool			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	// Unsupported typecast operators assert
	ColumnBool::operator const bool &() const
	{
		if (IsNull())
			return M_bNull;

		return m_bValue;
	}



	bool ColumnBool::Assign(Column & aFld)
	{
		bool	bSuccess;


		if (aFld.IsNull())
		{
			bSuccess = SetNull();
		}
		else
		{
			if (aFld.IsKindOf(ColumnBool::ClassObject()))
				bSuccess = SetValue(((ColumnBool&) aFld).m_bValue);
			else
				bSuccess = SetValue(aFld.AsBool());
		}

		return bSuccess;
	}



	// This method returns true if the Column's value is within the range of this'
	// value plus minus fVariation (which is a percentage figure, e.g. 5.5F is 5.5%)
	//
	bool ColumnBool::IsApproximatelyEqual(Column & aCol, float fVariation)
	{
		if (!aCol.IsKindOf(ColumnBool::ClassObject()))
			return false; // different types

		if (IsNull() != aCol.IsNull())
			return false; // One NULL and one not NULL

		if (IsNull())
			return true; // both NULL

		// Bools are either equal or not
		return m_bValue == ((ColumnBool) aCol).m_bValue;
	}



	int ColumnBool::Compare(ObjectPtr pObj)
	{
		if (!pObj || !pObj->IsKindOf(ColumnBool::ClassObject()))
			throw Exception(__FILE__, __LINE__,  Exception_error, "ColumnBool::Compare(): Can't compare ColumnBool %s with object which is not ako ColumnBool!", m_pMetaColumn->GetFullName().c_str());

		ColumnBoolPtr pCol = (ColumnBoolPtr) pObj;

		if (IsNull() && pCol->IsNull())
			return 0;

		if (pCol->IsNull())
			return 1;

		return Compare(pCol->m_bValue);
	}



	int ColumnBool::Compare(const bool & val)
	{
		if (IsNull())
			return -1;

		if (m_bValue < val)
			return -1;

		if (m_bValue > val)
			return 1;

		return 0;
	}



	std::string ColumnBool::AsJSON(RoleUserPtr pRoleUser)
	{
		std::string		strOut;

		if (IsNull())
			strOut = "null";
		else
			strOut = (m_bValue ? "true" : "false");

		return strOut;
	}








	// ==========================================================================
	// ColumnInt implementation
	//
	D3_CLASS_IMPL(ColumnInt, Column);

	// Convert this' value so that it can be use in SQL commands such as WHERE predicates and
	// SET clauses.
	// If this is NULL, returns the string "NULL", otherwise it returns AsString()
	//
	std::string ColumnInt::AsSQLString()
	{
		if (IsNull())
			return "NULL";

		return AsString(false);
	}



	// Type translators
	//
	std::string ColumnInt::AsString(bool bHumanReadable)
	{
		if (IsNull())
			return M_strNull;

		std::string		valOut;

		if (bHumanReadable)
		{
			if (m_pMetaColumn->IsSingleChoice())
			{
				Convert(valOut, m_iValue);
				return m_pMetaColumn->ChoiceDescAsString(valOut);
			}
			else
			{
				if (m_pMetaColumn->IsMultiChoice())
					return m_pMetaColumn->ChoiceDescsAsString((unsigned long) m_iValue);
			}
		}

		Convert(valOut, m_iValue);

		return valOut;
	}



	char ColumnInt::AsChar()
	{
		char			valOut = M_cNull;

		if (!IsNull())
			Convert(valOut, m_iValue);

		return valOut;
	}



	short ColumnInt::AsShort()
	{
		short			valOut = M_sNull;

		if (!IsNull())
			Convert(valOut, m_iValue);

		return valOut;
	}



	bool ColumnInt::AsBool()
	{
		bool			valOut = M_bNull;

		if (!IsNull())
			Convert(valOut, m_iValue);

		return valOut;
	}



	int ColumnInt::AsInt()
	{
		if (!IsNull())
			return m_iValue;

		return M_iNull;
	}



	long ColumnInt::AsLong()
	{
		long			valOut = M_lNull;

		if (!IsNull())
			Convert(valOut, m_iValue);

		return valOut;
	}



	float ColumnInt::AsFloat()
	{
		float			valOut = M_fNull;

		if (!IsNull())
			Convert(valOut, m_iValue);

		return valOut;
	}



	D3Date ColumnInt::AsDate()
	{
		D3Date		valOut = M_dtNull;

		if (!IsNull())
			Convert(valOut, m_iValue);

		return valOut;
	}



	bool ColumnInt::SetValue(const std::string & valIn)
	{
		int			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnInt::SetValue(const char & valIn)
	{
		int			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnInt::SetValue(const short & valIn)
	{
		int			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnInt::SetValue(const bool & valIn)
	{
		int			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnInt::SetValue(const int & iValue)
	{
		int							iPrevValue(m_iValue);
		bool						bPrevNull(IsNull());
		bool						bPrevDirty(IsDirty());


		if (!bPrevNull && m_iValue == iValue)
		{
			MarkDefined();
			return true;
		}

		// Check if the the update is allowed
		//
		if (NotifyBeforeUpdate())
		{
			m_iValue = iValue;
			MarkNotNull();
			MarkDirty();

			if (NotifyAfterUpdate())
				return true;

			// Reset values because the update was declined
			//
			m_iValue = iPrevValue;

			if (bPrevNull)
				MarkNull();

			if (!bPrevDirty)
				MarkClean();
		}

		return false;
	}



	bool ColumnInt::SetValue(const long & valIn)
	{
		int			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnInt::SetValue(const float & valIn)
	{
		int			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnInt::SetValue(const D3Date & valIn)
	{
		int			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	// Unsupported typecast operators assert
	ColumnInt::operator const int &() const
	{
		if (IsNull())
			return M_iNull;

		return m_iValue;
	}



	bool ColumnInt::Assign(Column & aFld)
	{
		bool	bSuccess;


		if (aFld.IsNull())
		{
			bSuccess = SetNull();
		}
		else
		{
			if (aFld.IsKindOf(ColumnInt::ClassObject()))
				bSuccess = SetValue(((ColumnInt&) aFld).m_iValue);
			else
				bSuccess = SetValue(aFld.AsInt());
		}

		return bSuccess;
	}



	// This method returns true if the Column's value is within the range of this'
	// value plus minus fVariation (which is a percentage figure, e.g. 5.5F is 5.5%)
	//
	bool ColumnInt::IsApproximatelyEqual(Column & aCol, float fVariation)
	{
		if (!aCol.IsKindOf(ColumnInt::ClassObject()))
			return false; // different types

		if (IsNull() != aCol.IsNull())
			return false; // One NULL and one not NULL

		if (IsNull())
			return true; // both NULL

		return approx_equal(m_iValue, ((ColumnInt) aCol).m_iValue, fVariation);
	}



	int ColumnInt::Compare(ObjectPtr pObj)
	{
		if (!pObj || !pObj->IsKindOf(ColumnInt::ClassObject()))
			throw Exception(__FILE__, __LINE__,  Exception_error, "ColumnInt::Compare(): Can't compare ColumnInt %s with object which is not ako ColumnInt!", m_pMetaColumn->GetFullName().c_str());

		ColumnIntPtr pCol = (ColumnIntPtr) pObj;

		if (IsNull() && pCol->IsNull())
			return 0;

		if (pCol->IsNull())
			return 1;

		return Compare(pCol->m_iValue);
	}



	int ColumnInt::Compare(const int & val)
	{
		if (IsNull())
			return -1;

		if (m_iValue < val)
			return -1;

		if (m_iValue > val)
			return 1;

		return 0;
	}



	std::string ColumnInt::AsJSON(RoleUserPtr pRoleUser)
	{
		std::string		strOut;

		if (IsNull())
			strOut = "null";
		else
			Convert(strOut, m_iValue);

		return strOut;
	}








	// ==========================================================================
	// ColumnLong implementation
	//
	D3_CLASS_IMPL(ColumnLong, Column);

	// Convert this' value so that it can be use in SQL commands such as WHERE predicates and
	// SET clauses.
	// If this is NULL, returns the string "NULL", otherwise it returns AsString()
	//
	std::string ColumnLong::AsSQLString()
	{
		if (IsNull())
			return "NULL";

		return AsString(false);
	}



	// Type translators
	//
	std::string ColumnLong::AsString(bool bHumanReadable)
	{
		if (IsNull())
			return M_strNull;

		std::string		valOut;

		if (bHumanReadable)
		{
			if (m_pMetaColumn->IsSingleChoice())
			{
				Convert(valOut, m_lValue);
				return m_pMetaColumn->ChoiceDescAsString(valOut);
			}
			else
			{
				if (m_pMetaColumn->IsMultiChoice())
					return m_pMetaColumn->ChoiceDescsAsString((unsigned long) m_lValue);
			}
		}

		Convert(valOut, m_lValue);

		return valOut;
	}



	char ColumnLong::AsChar()
	{
		char			valOut = M_cNull;

		if (!IsNull())
			Convert(valOut, m_lValue);

		return valOut;
	}



	short ColumnLong::AsShort()
	{
		short			valOut = M_sNull;

		if (!IsNull())
			Convert(valOut, m_lValue);

		return valOut;
	}



	bool ColumnLong::AsBool()
	{
		bool			valOut = M_bNull;

		if (!IsNull())
			Convert(valOut, m_lValue);

		return valOut;
	}



	int ColumnLong::AsInt()
	{
		int			valOut = M_iNull;

		if (!IsNull())
			Convert(valOut, m_lValue);

		return valOut;
	}



	long ColumnLong::AsLong()
	{
		if (!IsNull())
			return m_lValue;

		return M_lNull;
	}



	float ColumnLong::AsFloat()
	{
		float			valOut = M_fNull;

		if (!IsNull())
			Convert(valOut, m_lValue);

		return valOut;
	}



	D3Date ColumnLong::AsDate()
	{
		D3Date		valOut = M_dtNull;

		if (!IsNull())
			Convert(valOut, m_lValue);

		return valOut;
	}



	bool ColumnLong::SetValue(const std::string & valIn)
	{
		long			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnLong::SetValue(const char & valIn)
	{
		long			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnLong::SetValue(const short & valIn)
	{
		long			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnLong::SetValue(const bool & valIn)
	{
		long			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnLong::SetValue(const int & valIn)
	{
		long			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnLong::SetValue(const long & lValue)
	{
		long						lPrevValue(m_lValue);
		bool						bPrevNull(IsNull());
		bool						bPrevDirty(IsDirty());


		if (!bPrevNull && m_lValue == lValue)
		{
			MarkDefined();
			return true;
		}

		// Check if the the update is allowed
		//
		if (NotifyBeforeUpdate())
		{
			m_lValue = lValue;
			MarkNotNull();
			MarkDirty();

			if (NotifyAfterUpdate())
				return true;

			// Reset values because the update was declined
			//
			m_lValue = lPrevValue;

			if (bPrevNull)
				MarkNull();

			if (!bPrevDirty)
				MarkClean();
		}

		return false;
	}



	bool ColumnLong::SetValue(const float & valIn)
	{
		long			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnLong::SetValue(const D3Date & valIn)
	{
		long			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	// Unsupported typecast operators assert
	ColumnLong::operator const long &() const
	{
		if (IsNull())
			return M_lNull;

		return m_lValue;
	}



	bool ColumnLong::Assign(Column & aFld)
	{
		bool	bSuccess;


		if (aFld.IsNull())
		{
			bSuccess = SetNull();
		}
		else
		{
			if (aFld.IsKindOf(ColumnLong::ClassObject()))
				bSuccess = SetValue(((ColumnLong&) aFld).m_lValue);
			else
				bSuccess = SetValue(aFld.AsLong());
		}

		return bSuccess;
	}



	// This method returns true if the Column's value is within the range of this'
	// value plus minus fVariation (which is a percentage figure, e.g. 5.5F is 5.5%)
	//
	bool ColumnLong::IsApproximatelyEqual(Column & aCol, float fVariation)
	{
		if (!aCol.IsKindOf(ColumnLong::ClassObject()))
			return false; // different types

		if (IsNull() != aCol.IsNull())
			return false; // One NULL and one not NULL

		if (IsNull())
			return true; // both NULL

		return approx_equal(m_lValue, ((ColumnLong) aCol).m_lValue, fVariation);
	}



	int ColumnLong::Compare(ObjectPtr pObj)
	{
		if (!pObj || !pObj->IsKindOf(ColumnLong::ClassObject()))
			throw Exception(__FILE__, __LINE__,  Exception_error, "ColumnLong::Compare(): Can't compare ColumnLong %s with object which is not ako ColumnLong!", m_pMetaColumn->GetFullName().c_str());

		ColumnLongPtr pCol = (ColumnLongPtr) pObj;

		if (IsNull() && pCol->IsNull())
			return 0;

		if (pCol->IsNull())
			return 1;

		return Compare(pCol->m_lValue);
	}



	int ColumnLong::Compare(const long & val)
	{
		if (IsNull())
			return -1;

		if (m_lValue < val)
			return -1;

		if (m_lValue > val)
			return 1;

		return 0;
	}



	std::string ColumnLong::AsJSON(RoleUserPtr pRoleUser)
	{
		std::string		strOut;

		if (IsNull())
			strOut = "null";
		else
			Convert(strOut, m_lValue);

		return strOut;
	}








	// ==========================================================================
	// ColumnFloat implementation
	//
	D3_CLASS_IMPL(ColumnFloat, Column);

	// Convert this' value so that it can be use in SQL commands such as WHERE predicates and
	// SET clauses.
	// If this is NULL, returns the string "NULL", otherwise it returns AsString()
	//
	std::string ColumnFloat::AsSQLString()
	{
		if (IsNull())
			return "NULL";

		return AsString(false);
	}



	// Type translators
	//
	std::string ColumnFloat::AsString(bool bHumanReadable)
	{
		if (IsNull())
			return M_strNull;

		std::string		valOut;

		Convert(valOut, m_fValue);

		return valOut;
	}



	char ColumnFloat::AsChar()
	{
		char			valOut = M_cNull;

		if (!IsNull())
			Convert(valOut, m_fValue);

		return valOut;
	}



	short ColumnFloat::AsShort()
	{
		short			valOut = M_sNull;

		if (!IsNull())
			Convert(valOut, m_fValue);

		return valOut;
	}



	bool ColumnFloat::AsBool()
	{
		bool			valOut = M_bNull;

		if (!IsNull())
			Convert(valOut, m_fValue);

		return valOut;
	}



	int ColumnFloat::AsInt()
	{
		int			valOut = M_iNull;

		if (!IsNull())
			Convert(valOut, m_fValue);

		return valOut;
	}



	long ColumnFloat::AsLong()
	{
		long			valOut = M_lNull;

		if (!IsNull())
			Convert(valOut, m_fValue);

		return valOut;
	}



	float ColumnFloat::AsFloat()
	{
		if (!IsNull())
			return m_fValue;

		return M_fNull;
	}



	D3Date ColumnFloat::AsDate()
	{
		D3Date		valOut = M_dtNull;

		if (!IsNull())
			Convert(valOut, m_fValue);

		return valOut;
	}



	bool ColumnFloat::SetValue(const std::string & valIn)
	{
		float			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnFloat::SetValue(const char & valIn)
	{
		float			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnFloat::SetValue(const short & valIn)
	{
		float			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnFloat::SetValue(const bool & valIn)
	{
		float			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnFloat::SetValue(const int & valIn)
	{
		float			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnFloat::SetValue(const long & valIn)
	{
		float			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnFloat::SetValue(const float & fValue)
	{
		float						fPrevValue(m_fValue);
		bool						bPrevNull(IsNull());
		bool						bPrevDirty(IsDirty());


		if (!bPrevNull && m_fValue == fValue)
		{
			MarkDefined();
			return true;
		}

		// Check if the the update is allowed
		//
		if (NotifyBeforeUpdate())
		{
			m_fValue = fValue;
			MarkNotNull();
			MarkDirty();

			if (NotifyAfterUpdate())
				return true;

			// Reset values because the update was declined
			//
			m_fValue = fPrevValue;

			if (bPrevNull)
				MarkNull();

			if (!bPrevDirty)
				MarkClean();
		}

		return false;
	}



	bool ColumnFloat::SetValue(const D3Date & valIn)
	{
		float			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	// Unsupported typecast operators assert
	ColumnFloat::operator const float &() const
	{
		if (IsNull())
			return M_fNull;

		return m_fValue;
	}



	bool ColumnFloat::Assign(Column & aFld)
	{
		bool	bSuccess;


		if (aFld.IsNull())
		{
			bSuccess = SetNull();
		}
		else
		{
			if (aFld.IsKindOf(ColumnFloat::ClassObject()))
				bSuccess = SetValue(((ColumnFloat&) aFld).m_fValue);
			else
				bSuccess = SetValue(aFld.AsFloat());
		}

		return bSuccess;
	}



	// This method returns true if the Column's value is within the range of this'
	// value plus minus fVariation (which is a percentage figure, e.g. 5.5F is 5.5%)
	//
	bool ColumnFloat::IsApproximatelyEqual(Column & aCol, float fVariation)
	{
		if (!aCol.IsKindOf(ColumnFloat::ClassObject()))
			return false; // different types

		if (IsNull() != aCol.IsNull())
			return false; // One NULL and one not NULL

		if (IsNull())
			return true; // both NULL

		return approx_equal(m_fValue, ((ColumnFloat) aCol).m_fValue, fVariation);
	}



	int ColumnFloat::Compare(ObjectPtr pObj)
	{
		if (!pObj || !pObj->IsKindOf(ColumnFloat::ClassObject()))
			throw Exception(__FILE__, __LINE__,  Exception_error, "ColumnFloat::Compare(): Can't compare ColumnFloat %s with object which is not ako ColumnFloat!", m_pMetaColumn->GetFullName().c_str());

		ColumnFloatPtr pCol = (ColumnFloatPtr) pObj;

		if (IsNull() && pCol->IsNull())
			return 0;

		if (pCol->IsNull())
			return 1;

		return Compare(pCol->m_fValue);
	}



	int ColumnFloat::Compare(const float & val)
	{
		if (IsNull())
			return -1;

		if (m_fValue < val)
			return -1;

		if (m_fValue > val)
			return 1;

		return 0;
	}



	std::string ColumnFloat::AsJSON(RoleUserPtr pRoleUser)
	{
		std::string		strOut;

		if (IsNull())
			strOut = "null";
		else
			Convert(strOut, m_fValue);

		return strOut;
	}








	// ==========================================================================
	// ColumnDate implementation
	//
	D3_CLASS_IMPL(ColumnDate, Column);

	// Convert this' value so that it can be use in SQL commands such as WHERE predicates and
	// SET clauses.
	// If this is NULL, returns the string "NULL", otherwise behaviour depends on RDBMS
	std::string ColumnDate::AsSQLString()
	{
		std::string		strTemp = "NULL";

		if (!IsNull())
		{
			const char		cQuote = '\'';
			D3Date				dt(m_dtValue);


			dt.AdjustToTimeZone(m_pMetaColumn->GetMetaEntity()->GetMetaDatabase()->GetTimeZone());

			// Date literals differ from RDBMS to RDBMS
			switch (m_pMetaColumn->GetMetaEntity()->GetMetaDatabase()->GetTargetRDBMS())
			{
				case SQLServer:
					strTemp	 = cQuote;
					strTemp += dt.AsString(3);
					strTemp += cQuote;
					break;

				case Oracle:
					strTemp  = "TO_DATE('";
					strTemp += dt.AsString();
					strTemp += "', 'YYYY-MM-DD HH24:MI:SS')";
					break;

				default:
					strTemp  = cQuote;
					strTemp += dt.AsString();
					strTemp += cQuote;
					break;
			}
		}

		return strTemp;
	}



	// Type translators
	//
	std::string ColumnDate::AsString(bool bHumanReadable)
	{
		std::string		valOut = M_strNull;

		if (!IsNull())
			Convert(valOut, m_dtValue);

		return valOut;
	}



	char ColumnDate::AsChar()
	{
		char			valOut = M_cNull;

		if (!IsNull())
			Convert(valOut, m_dtValue);

		return valOut;
	}



	short ColumnDate::AsShort()
	{
		short			valOut = M_sNull;

		if (!IsNull())
			Convert(valOut, m_dtValue);

		return valOut;
	}



	bool ColumnDate::AsBool()
	{
		bool			valOut = M_bNull;

		if (!IsNull())
			Convert(valOut, m_dtValue);

		return valOut;
	}



	int ColumnDate::AsInt()
	{
		int			valOut = M_iNull;

		if (!IsNull())
			Convert(valOut, m_dtValue);

		return valOut;
	}



	long ColumnDate::AsLong()
	{
		long			valOut = M_lNull;

		if (!IsNull())
			Convert(valOut, m_dtValue);

		return valOut;
	}



	float ColumnDate::AsFloat()
	{
		float		valOut = M_fNull;

		if (!IsNull())
			Convert(valOut, m_dtValue);

		return valOut;
	}



	D3Date ColumnDate::AsDate()
	{
		if (!IsNull())
			return m_dtValue;

		return M_dtNull;
	}



	bool ColumnDate::SetValue(const std::string & valIn)
	{
		D3Date			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnDate::SetValue(const char & valIn)
	{
		D3Date			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnDate::SetValue(const short & valIn)
	{
		D3Date			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnDate::SetValue(const bool & valIn)
	{
		D3Date			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnDate::SetValue(const int & valIn)
	{
		D3Date			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnDate::SetValue(const long & valIn)
	{
		D3Date			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnDate::SetValue(const float & valIn)
	{
		D3Date			valOut;
		Convert(valOut, valIn);
		return SetValue(valOut);
	}



	bool ColumnDate::SetValue(const D3Date & dtValue)
	{
		D3Date					dtPrevValue(m_dtValue);
		bool						bPrevNull(IsNull());
		bool						bPrevDirty(IsDirty());


		if (bPrevNull)
		{
			if (dtValue == Column::M_dtNull)
				return true;
		}
		else
		{
			if (dtValue == Column::M_dtNull)
				return SetNull();

			if (m_dtValue == dtValue)
				return true;
		}

		// Check if the the update is allowed
		//
		if (NotifyBeforeUpdate())
		{
			m_dtValue = dtValue;
			MarkNotNull();
			MarkDirty();

			if (NotifyAfterUpdate())
				return true;

			// Reset values because the update was declined
			//
			m_dtValue = dtPrevValue;

			if (bPrevNull)
				MarkNull();

			if (!bPrevDirty)
				MarkClean();
		}

		return false;
	}



	// Unsupported typecast operators assert
	ColumnDate::operator const D3Date &() const
	{
		if (IsNull())
			return M_dtNull;

		return m_dtValue;
	}



	bool ColumnDate::Assign(Column & aFld)
	{
		bool	bSuccess;


		if (aFld.IsNull())
		{
			bSuccess = SetNull();
		}
		else
		{
			if (aFld.IsKindOf(ColumnDate::ClassObject()))
				bSuccess = SetValue(((ColumnDate&) aFld).m_dtValue);
			else
				bSuccess = SetValue(aFld.AsDate());
		}

		return bSuccess;
	}



	// This method returns true if the Column's value is within the range of this'
	// value plus minus fVariation (which is a percentage figure, e.g. 5.5F is 5.5%)
	//
	bool ColumnDate::IsApproximatelyEqual(Column & aCol, float fVariation)
	{
		if (!aCol.IsKindOf(ColumnDate::ClassObject()))
			return false; // different types

		if (IsNull() != aCol.IsNull())
			return false; // One NULL and one not NULL

		if (IsNull())
			return true; // both NULL

		return m_dtValue == ((ColumnDate) aCol).m_dtValue;
	}



	int ColumnDate::Compare(ObjectPtr pObj)
	{
		if (!pObj || !pObj->IsKindOf(ColumnDate::ClassObject()))
			throw Exception(__FILE__, __LINE__,  Exception_error, "ColumnDate::Compare(): Can't compare ColumnDate %s with object which is not ako ColumnDate!", m_pMetaColumn->GetFullName().c_str());

		ColumnDatePtr pCol = (ColumnDatePtr) pObj;

		if (IsNull() && pCol->IsNull())
			return 0;

		if (pCol->IsNull())
			return 1;

		return Compare(pCol->m_dtValue);
	}



	int ColumnDate::Compare(const D3Date & val)
	{
		if (IsNull())
			return -1;

		if (m_dtValue < val)
			return -1;

		if (m_dtValue > val)
			return 1;

		return 0;
	}



	std::string ColumnDate::AsJSON(RoleUserPtr pRoleUser)
	{
		std::ostringstream	ostrm;

		if (IsNull())
			ostrm << "null";
		else
			ostrm << '"' << m_dtValue.AsISOString(3) << '"';

		return ostrm.str();
	}



	bool ColumnDate::SetNull()
	{
		if (!Column::SetNull())
			return false;

		m_dtValue = Column::M_dtNull;
		return true;
	}






	// ==========================================================================
	// ColumnBlob implementation
	//
	D3_CLASS_IMPL(ColumnBlob, Column);


	//! Default ctor might be used by D3 only
	ColumnBlob::ColumnBlob()
		: Column(),
			m_Stream(ios::in | ios::out | ios::binary)
	{
	}





	//! This ctor is the most typical way instances of this class are constructed
	ColumnBlob::ColumnBlob(MetaColumnPtr pMetaColumn, EntityPtr pEntity)
		: Column(pMetaColumn, pEntity),
			m_Stream(ios::in | ios::out | ios::binary)
	{
	}





	//! Copy ctor ensures that the new column is identical to this
	ColumnBlob::ColumnBlob(Column & aColumn)
		: Column(aColumn),
			m_Stream(ios::in | ios::out | ios::binary)
	{
		assert(aColumn.GetMetaColumn()->GetType() == MetaColumn::dbfBlob);

		ColumnBlobPtr		pBlob = (ColumnBlobPtr) &aColumn;

		pBlob->Fetch();

		if (pBlob->IsNull())
		{
			MarkNull();
		}
		else
		{
			MarkNotNull();
			SetValue(pBlob->m_Stream);
		}
	}





	//! Compares this with another column, see also Object::Compare (the other column must also be a ColumnBob)
	int ColumnBlob::Compare(ObjectPtr	pAnotherBlob)
	{
		assert(pAnotherBlob->IAm().IsKindOf(M_Class));

		ColumnBlobPtr				pBlob = (ColumnBlobPtr) pAnotherBlob;
		int									iResult = 0;

		pBlob->Fetch();

		if (IsNull())
		{
			if (pBlob->IsNull())
				iResult = 0;
			else
				iResult = -1;
		}
		else
		{
			if (pBlob->IsNull())
				iResult = 1;
			else
			{
				unsigned char *pSrc, *pTrg;
				unsigned int	nSrc, nTrg;

				pSrc = ReadRaw(nSrc);
				pTrg = pBlob->ReadRaw(nTrg);

				for (unsigned int i=0; i < std::min(nSrc, nTrg); i++)
				{
					if (*(pSrc + i) == *(pTrg + i))
						continue;

					if (*(pSrc + i) > *(pTrg + i))
						iResult = 1;
					else
						iResult = -1;

					break;
				}

				if (iResult == 0)
				{
					if (nSrc != nTrg)
					{
						if (nSrc > nTrg)
							iResult = 1;
						else
							iResult = -1;
					}
				}

				delete [] pSrc;
				delete [] pTrg;
			}
		}

		return iResult;
	}





	//! Assigns thhe content of this to another column (the other column must also be a ColumnBob)
	bool ColumnBlob::Assign(Column & aColumn)
	{
		assert(aColumn.GetMetaColumn()->GetType() == MetaColumn::dbfBlob);

		ColumnBlobPtr		pBlob = (ColumnBlobPtr) &aColumn;

		pBlob->Fetch();

		if (pBlob->IsNull())
			SetNull();
		else
			SetValue(pBlob->m_Stream);

		return true;
	}





	//! Returns this' value JSON encoded
	/*! If IsNull() is true, the string will be "null". Otherwise, the returned string is
			the same as AsBase64String() but enclosed in single quotes.
	*/
	std::string ColumnBlob::AsJSON(RoleUserPtr pRoleUser)
	{
		std::string			strJSON;

		if (IsNull())
		{
			strJSON = "null";
		}
		else
		{
			strJSON  = '"';
			strJSON += AsBase64String();
			strJSON += '"';
		}

		return strJSON;
	}





	//! Assigns the value passed in to this
	/*! The value that is passed in is expected to be Base64 encoded.
			The method decodes the data and writes it to it's internal stream.
			If the input string is empty, it will set this to NULL and release
			the content of the stream.

			If you have binary data to write, use method WriteRaw() instead.
	*/
	bool ColumnBlob::SetValue(const std::string & strBase64String)
	{
		if (NotifyBeforeUpdate())
		{
			// Decode the string
			char *							pDecodedBuffer = NULL;
			int									nDecodedLength;

			try
			{
				if (!strBase64String.length())
				{
					Clear();
					MarkNull();
					MarkDirty();
				}
				else
				{
					// Clear stream, decode the input, write the decoded buffer and delete the decoded buffer
					Clear();
					pDecodedBuffer = (char*) APALUtil::base64_decode(strBase64String, nDecodedLength);
					m_Stream.write(pDecodedBuffer, nDecodedLength);
					delete[] pDecodedBuffer;
					MarkNotNull();
					MarkDirty();
				}
			}
			catch(...)
			{
				delete[] pDecodedBuffer;
				return false;
			}

			MarkFetched();
			NotifyAfterUpdate();
		}

		return true;
	}





	//! Returns the this' data as a Base64 encoded string
	std::string ColumnBlob::AsBase64String()
	{
		std::string			strBlob;
		unsigned int		nRaw;
		unsigned char *	pRaw;

		// Fetch the raw buffer and encode it
		pRaw = ReadRaw(nRaw);

		if (pRaw)
		{
			APALUtil::base64_encode(strBlob, pRaw, nRaw);
			delete [] pRaw;
		}

		return strBlob;
	}





	//! Allows you to write the BLOB to an ostream
	std::ostream & ColumnBlob::ToStream(std::ostream & strmOut)
	{
		if (!IsNull())
		{
			std::streamsize								nRead = 0;
			char													buffer[D3_STREAMBUFFER_SIZE];

			// Make sure stream points to the start
			m_Stream.seekg(std::ios_base::beg);

			while (true)
			{
				m_Stream.read(buffer, D3_STREAMBUFFER_SIZE);
				nRead = m_Stream.gcount();

				if (nRead > 0)
					strmOut.write(buffer, nRead);

				if (nRead < D3_STREAMBUFFER_SIZE)
					break;
			}
		}

		return strmOut;
	}





	//! Assigns the content of the stream passed in to this
	/*! If the passed in stream is empty, this is set to NULL,
			otherwise, the data contained in strmIn will replace
			the current content in this' stream column
	*/
	std::istream & ColumnBlob::FromStream(std::istream & strmIn)
	{
		if (NotifyBeforeUpdate())
		{
			MarkNotNull();
			MarkDirty();

			Clear();

			std::streamsize	nStrmSize = 0, nRead = 0;
			char						buffer[D3_STREAMBUFFER_SIZE];

			while (true)
			{
				strmIn.read(buffer, D3_STREAMBUFFER_SIZE);
				nRead = strmIn.gcount();

				if (nRead > 0)
				{
					m_Stream.write(buffer, nRead);
					nStrmSize += nRead;
				}

				if (nRead < D3_STREAMBUFFER_SIZE)
					break;
			}

			if (!nStrmSize)
				MarkNull();

			MarkFetched();
			NotifyAfterUpdate();
		}

		return strmIn;
	}





	//! Assigns thhe content of this to another column (the other column must also be a ColumnBob)
	Column& ColumnBlob::operator=(std::stringstream & stream)
	{
		SetValue(stream);
		return *this;
	}





	//! This method returns a heap allocated buffer that contains this' raw data
	unsigned char* ColumnBlob::ReadRaw(unsigned int& nSize)
	{
		unsigned char *								pRaw = NULL;


		// Allocate the buffer
		nSize = m_Stream.str().length();

		if (nSize)
		{
			// Make sure stream is positioned to the start
			m_Stream.seekg(std::ios_base::beg);
			// Allocate buffer for output
			pRaw = new unsigned char[nSize+1];
			// Read data into buffer
			m_Stream.read((char*) pRaw, nSize);
		}

		return pRaw;
	}





	//! Replaces the content of the internal stream with the data passed in
	/*! If nRaw is 0, the internal stream will be cleared and will set this to NULL.
			This method does not check for null terminators. It will write exactly
			nRaw bytes to the internal stream.
	*/
	void ColumnBlob::WriteRaw(const char* pszRaw, unsigned int nRaw)
	{
		if (NotifyBeforeUpdate())
		{
			MarkNotNull();
			MarkDirty();

			Clear();

			if (nRaw > 0)
			{
				m_Stream.write(pszRaw, nRaw);
				MarkNotNull();
				MarkDirty();
			}
			else
			{
				MarkNull();
			}

			MarkFetched();
			NotifyAfterUpdate();
		}
	}





	//! Clear this' content
	void ColumnBlob::Clear()
	{
		m_Stream.str("");
	}





#ifdef APAL_SUPPORT_OTL
	//! Writes' this content to an OTLStream
	void ColumnBlob::WriteOTLStream(otl_nocommit_stream & otl_stream)
	{
		unsigned int			nRaw;
		unsigned char *		pRaw;

		pRaw = ReadRaw(nRaw);

		if (pRaw)
		{
			otl_long_string		oBlob(pRaw, nRaw, nRaw);

			otl_stream << oBlob;
			delete [] pRaw;
		}
	}
#endif






	// ==========================================================================
	// ColumnBinary implementation
	//
	D3_CLASS_IMPL(ColumnBinary, Column);


	//! Copy ctor ensures that the new column is identical to this
	ColumnBinary::ColumnBinary(Column & aColumn)
		: Column(aColumn)
	{
		if (aColumn.GetMetaColumn()->GetType() != MetaColumn::dbfBinary)
			throw std::runtime_error("ColumnBinary copy ctor() must be passed a Column of type dbfBinary");

		ColumnBinaryPtr		pBinary = (ColumnBinaryPtr) &aColumn;

		SetValue(pBinary->GetData());
	}





	//! Compares this with another column, see also Object::Compare (the other column must also be a ColumnBob)
	int ColumnBinary::Compare(ObjectPtr	pAnotherBinary)
	{
		assert(pAnotherBinary->IAm().IsKindOf(M_Class));
		return m_data.compare(((ColumnBinaryPtr) pAnotherBinary)->m_data);
	}





	//! Assigns thhe content of this to another column (the other column must also be a ColumnBob)
	bool ColumnBinary::Assign(Column & aColumn)
	{
		assert(aColumn.GetMetaColumn()->GetType() == MetaColumn::dbfBinary);
		SetValue(((ColumnBinary&) aColumn).m_data);
		return true;
	}





	//! Returns a string that can be used in SQL queries to compare or assign binary data
	std::string	ColumnBinary::AsSQLString()
	{
		std::string		strSQL = "NULL";

		if (!this->IsNull())
		{
			// Binary data literals differ from RDBMS to RDBMS
			switch (m_pMetaColumn->GetMetaEntity()->GetMetaDatabase()->GetTargetRDBMS())
			{
				case SQLServer:
					strSQL = std::string("0x") + charToHex(m_data, std::min(m_data.length(), m_pMetaColumn->GetMaxLength()));
					break;

				case Oracle:
					strSQL = std::string("hextoraw('") + charToHex(m_data, std::min(m_data.length(), m_pMetaColumn->GetMaxLength())) + "')";
					break;
			}
		}

		return strSQL;
	}





	//! Returns this' value JSON encoded
	/*! If IsNull() is true, the string will be "null". Otherwise, the returned string is
			the same as AsBase64String() but enclosed in single quotes.
	*/
	std::string ColumnBinary::AsJSON(RoleUserPtr pRoleUser)
	{
		std::string			strJSON;

		if (IsNull())
		{
			strJSON = "null";
		}
		else
		{
			strJSON  = '"';
			strJSON += AsBase64String();
			strJSON += '"';
		}

		return strJSON;
	}





	bool ColumnBinary::SetValue(const Data & data)
	{
		Data						prevValue(m_data);
		bool						bPrevNull(IsNull());
		bool						bPrevDirty(IsDirty());


		if (bPrevNull)
		{
			if (data == Column::M_dataNull)
				return true;
		}
		else
		{
			if (data == Column::M_dataNull)
				return SetNull();

			if (m_data == data)
				return true;
		}

		// Check if the the update is allowed
		//
		if (NotifyBeforeUpdate())
		{
			m_data = data;
			MarkNotNull();
			MarkDirty();

			if (NotifyAfterUpdate())
				return true;

			// Reset values because the update was declined
			//
			m_data = prevValue;

			if (bPrevNull)
				MarkNull();

			if (!bPrevDirty)
				MarkClean();
		}

		return false;
	}



	//! Returns the this' data as a Base64 encoded string
	std::string ColumnBinary::AsBase64String()
	{
		std::string			strBinary;

		if (m_data.length())
			strBinary = m_data.base64_encode();

		return strBinary;
	}






} // end namesapce D3
