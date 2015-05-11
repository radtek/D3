// ===========================================================
// File Info:
//
// $Author: pete $
// $Date: 2011/02/04 06:37:05 $
// $Revision: 1.3 $
//
// ===========================================================
// Change History:
// ===========================================================
//
// 13/03/03 - R1 - Hugp
//
// Implementation of common stuff for D3 objects and classes
// -----------------------------------------------------------

// @@DatatypeInclude
#include "D3Types.h"
// @@End

// @@Includes
#include "D3.h"
#include "Exception.h"
// @@End

namespace D3
{
	//========================================================================
	// Settings::DBVersion Implementation

	std::string Settings::DBVersion::toString()
	{
		char			buff[30];

		snprintf(buff, 30, "%d_%02d_%04d", major, minor, revision);

		return buff;
	}




	//========================================================================
	// Object Implementation

	// Class support for Object class
	//
	const Class			Object::M_Class("Object", NULL, sizeof(Object));



	// Return a copy of this Object. Implemented by calling the Class
	// constructor and calling CopyTo with the new object.
	//
	ObjectPtr Object::CreateCopy()
	{
		ObjectPtr			pObj;

		pObj = IAm().CreateInstance();

		if (pObj)
			pObj->Copy(this);

		return pObj;
	}

	// Return a deep copy of this Object. Implemented by calling the Class
	// constructor and calling deepCopyTo with the new object.
	//
	ObjectPtr Object::CreateDeepCopy()
	{
		ObjectPtr	pObj;

		pObj = IAm().CreateInstance();

		if (pObj)
			pObj->DeepCopy(this);

		return pObj;
	}

	// Return true if this is an instance of aCls.
	//
	bool Object::IsA(const std::string & strCls) const
	{
		return IsA(Class::Of(strCls));
	}

	// Return true if this is an instance of aCls or an instance of a descendent
	// of aCls.
	//
	bool Object::IsKindOf(const Class& aCls) const
	{
		return IAm().IsKindOf(aCls);
	}

	// Return true if this is an instance of aCls or an instance of a descendent
	// of aCls.
	//
	bool Object::IsKindOf(const std::string & strCls) const
	{
		return IsKindOf(Class::Of(strCls));
	}

	// Assignment method. This method is called by the overloaded assign operator.
	// It should never be invoked directly. On this level, a shallow copy of pObj
	// is assigned to this. Sub-classes shoud reimplement this method if this
	// is not desirable.
	//
	bool Object::Assign(ObjectPtr pObj)
	{
		return Copy(pObj);
	}


	// Shallow copy method. This method makes this a shallow copy of pObj.
	//
	bool Object::Copy(ObjectPtr pObj)
	{
		if (!pObj)
			return false;

		if (IAm() != pObj->IAm())
			return false;

		memcpy(this, pObj, IAm().Size());

		return true;
	}

	// Deep copy method. This method makes this a deep copy of aObj. At this
	// level, a shallow copy is made, since this method should only be invoked
	// if the descending class does not have any object's (or object pointers)
	// as instance variables. Otherwise, descending classes must reimplement a
	// suitable DeepCopy method.
	//
	bool Object::DeepCopy(ObjectPtr pObj)
	{
		return Copy(pObj);
	}

	#ifdef D3_ARCHIVER
	// Load the contents of this Object from the D3Archiver a. For any class, this
	// method is only responsible for the member variables of that class. Sub-
	// classes should chain the ancestors GetFrom before storing their member vars
	//
	bool Object::GetFrom(D3Archiver &a)
	{
		return false;
	}

	// Store the contents of an object in the archive. Subclasses should chain
	// the ancestors PutTo before storing their member vars
	//
	bool Object::PutTo(D3Archiver &a)
	{
		return false;
	}
	#endif

	// Compare two objects. This method does a compare based on the memory location.
	// It returns 1 if this is greater than aObj, 0 if this equal to aObj and
	// -1 if this is less than aObj.
	//
	int Object::Compare(ObjectPtr pObj)
	{
		if(this > pObj)
			return 1;

		if(this < pObj)
			return -1;

		return 0;
	}



	//========================================================================
	// Class Implementation

	// Class support for Class class
	//
	const Class			Class::M_Class("Class", NULL, sizeof(Class));


	// Constructor for the creation of Class class instance for a class with
	// no ancestor.
	//
	Class::Class(const std::string & strName, D3_CNSTRFUNC pfnCtor, int szObject)
	 : m_strName(strName), m_pfnCtor(pfnCtor), m_szObject(szObject), m_pAncestor(NULL)
	{
		GetClassPtrList().push_back(this);
	}

	// Constructor for the creation of Class class instance for a class with
	// ancestor.
	//
	Class::Class(const std::string & strName, D3_CNSTRFUNC pfnCtor, int szObject, Class* pAncestor)
	 : m_strName(strName), m_pfnCtor(pfnCtor), m_szObject(szObject), m_pAncestor(pAncestor)
	{
		GetClassPtrList().push_back(this);
	}

	// Destructor
	//
	Class::~Class()
	{
		GetClassPtrList().remove(this);

		if (GetClassPtrList().empty())
			delete 	&(GetClassPtrList());
	}

	// Static function: Return the Class instance with the class name name.
	//
	ClassPtr Class::Find(const std::string & strClsName)
	{
		ClassPtr					pCls;
		ClassPtrListItr	itrCls;

		for (itrCls = GetClassPtrList().begin(); itrCls != GetClassPtrList().end(); itrCls++)
		{
			pCls = *itrCls;

			if (pCls->Name() == strClsName)
				return pCls;
		}

		return NULL;
	}

	// Static function: Return the Class instance with the class name name.
	//
	const Class& Class::Of(const std::string & strClsName)
	{
		ClassPtr pCls = Find(strClsName);

		if (!pCls)
			throw Exception(__FILE__, __LINE__, Exception_error, "Class::Of(): Unknown class '%s'!", strClsName.c_str());

		return *pCls;
	}

	// Static function: CreateInstance an instance of the class specified by strClsName and
	// return a pointer to it. (Returns NULL if creation fails).
	//
	ObjectPtr Class::CreateInstance(const std::string & strClsName)
	{
		ClassPtr	pCls;

		pCls = Class::Find(strClsName);

		if (!pCls)
			throw Exception(__FILE__, __LINE__, Exception_error, "Class::CreateInstance(): Unknown class '%s'!", strClsName.c_str());

		return pCls->CreateInstance();
	}

	// Static function: GetCtor returns the constructor function for an instance of the
	// class with the name specified
	//
	D3_CNSTRFUNC Class::Ctor(const std::string & strClsName)
	{
		ClassPtr	pCls;

		pCls = Class::Find(strClsName);

		if (pCls)
			return pCls->m_pfnCtor;

		return NULL;
	}



	// Return true if this is an instance of aCls or an instance of a descendent
	// of aCls.
	//
	bool Class::IsKindOf(const Class& aCls) const
	{
		ClassPtr pCls = (Class*) this;

		while (pCls && pCls != &aCls)
			pCls = pCls->m_pAncestor;

		return pCls == &aCls;
	}



	void Class::DumpHierarchy(ClassPtr pParent, const std::string & strPfx)
	{
		ClassPtrListItr				itrClass;
		std::string						strPrefix = strPfx;


		if (pParent)
		{
			std::cout << strPfx.c_str() << pParent->Name().c_str() << std::endl;
			strPrefix += "  ";
		}
		else
		{
			std::cout << "Registered D3 Classes" << std::endl;
			std::cout << "=====================" << std::endl;
		}

		for (	itrClass  = GetClassPtrList().begin();
					itrClass != GetClassPtrList().end();
					itrClass++)
		{
			if (pParent == (*itrClass)->m_pAncestor)
				DumpHierarchy(*itrClass, strPrefix);
		}

		if (!pParent)
			std::cout << std::endl;
	}



	ClassPtrList& Class::GetClassPtrList()
	{
		static	ClassPtrListPtr M_pClsTbl = new ClassPtrList();
		return *M_pClsTbl;
	}



} // end namespace D3
