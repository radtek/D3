#ifndef _D3_H_
#define _D3_H_

// ===========================================================
// File Info:
//
// $Author: daellenj $
// $Date: 2014/08/23 21:47:45 $
// $Revision: 1.3 $
//
// ===========================================================
// Change History:
// ===========================================================
//
// 13/03/03 - R1 - Hugp
//
// Created header with common stuff for D3 objects and classes
// -----------------------------------------------------------

// D3 classes must use these macros for D3 classes
//
#define D3_CLASS_DECL(d3cls) \
	protected: \
		static	const Class			M_Class; \
		static  Object*					ClassCtor()		{ return new d3cls(); } \
	public: \
		static  const Class&		ClassObject() { return M_Class; } \
		virtual const Class&		IAm() const		{ return M_Class; }

#define D3_CLASS_DECL_PV(d3cls) \
	protected: \
		static	const Class			M_Class; \
	public: \
		static  const Class&		ClassObject() { return M_Class; } \
		virtual const Class&		IAm() const		{ return M_Class; }

#define D3_CLASS_IMPL(d3cls, d3ancestor) \
const Class  d3cls::M_Class(#d3cls, &(d3cls::ClassCtor), sizeof(d3cls), (Class*) &(d3ancestor::ClassObject()))

#define D3_CLASS_IMPL_PV(d3cls, d3ancestor) \
const Class  d3cls::M_Class(#d3cls, NULL, sizeof(d3cls), (Class*) &(d3ancestor::ClassObject()))





//! D3 Namespace
/*! Just to avoid potential name clashes everything in D3 is in the D3 namespace
*/
namespace D3
{
	// Necessary forward declarations
	//
	class Settings;
	class Object;
	class Class;

#ifdef D3_ARCHIVER
	class Archiver;   // To be implemented sometimes later
#endif

	// The Settings class provides the means for clients to initialise
	// parameters used by this module.
	//
	class D3_API Settings
	{
		public:
			struct DBVersion
			{
				unsigned int	major;
				unsigned int	minor;
				unsigned int	revision;

				DBVersion() : major(0), minor(0), revision(0) {}
				DBVersion(unsigned int r1, unsigned int r2, unsigned int r3) : major(r1), minor(r2), revision(r3) {}

				std::string		toString();
				bool					valid()		{ return major > 0 || minor > 0 || revision > 0; }
			};

			typedef std::map<std::string, DBVersion>	DatabaseVersionMap;
			typedef DatabaseVersionMap::iterator			DatabaseVersionMapItr;

		protected:
			bool								m_bImperial;
			int									m_iPasswordExpiresInDays;
			bool								m_bAllowReusingPasswords;
			int									m_iRejectReusingMostRecentXPasswords;
			int									m_iRejectReusingPasswordsUsedInPastXDays;
			int									m_iMaxPasswordRetries;
			std::string					m_strSysadminPWD;
			std::string					m_strAdminPWD;
			DatabaseVersionMap	m_mapDBVersions;


		public:
			// Let's use some sensible defaults 
			Settings()
			:	m_bImperial(true),
				m_iPasswordExpiresInDays(0),
				m_bAllowReusingPasswords(true),
				m_iRejectReusingMostRecentXPasswords(0),
				m_iRejectReusingPasswordsUsedInPastXDays(0),
				m_iMaxPasswordRetries(0),
				m_strSysadminPWD("HVpiLQ4KEEsKNwckHHAiJRIgBmEJI3l/"),
				m_strAdminPWD("0Ud6Qsc0zK+oA/aRkMl3yw==")
			{}

			~Settings() {}

			static Settings&		Singleton()
			{
				static Settings		s_Settings;
				return s_Settings;
			}

		public:
			void				Imperial(bool bImperial)												{ m_bImperial = bImperial; }
			bool				Imperial()																			{	return m_bImperial;	}

			void				PasswordExpiresInDays(int days)									{ m_iPasswordExpiresInDays = std::max(days, 0); }
			int					PasswordExpiresInDays()													{ return m_iPasswordExpiresInDays; }

			void				AllowReusingPasswords(bool bAllow)							{ m_bAllowReusingPasswords = bAllow; }
			bool				AllowReusingPasswords()													{	return m_bAllowReusingPasswords;	}

			void				RejectReusingMostRecentXPasswords(int num)			{ m_iRejectReusingMostRecentXPasswords = num; }
			int					RejectReusingMostRecentXPasswords()							{	return m_iRejectReusingMostRecentXPasswords;	}

			void				RejectReusingPasswordsUsedInPastXDays(int days)	{ m_iRejectReusingPasswordsUsedInPastXDays = days; }
			int					RejectReusingPasswordsUsedInPastXDays()					{	return m_iRejectReusingPasswordsUsedInPastXDays;	}

			void				MaxPasswordRetries(int num)											{ m_iMaxPasswordRetries = num; }
			int					MaxPasswordRetries()														{	return m_iMaxPasswordRetries;	}

			void				SysadminPWD(std::string pwd)										{ m_strSysadminPWD = pwd; }
			std::string	SysadminPWD()																		{	return m_strSysadminPWD;	}

			void				AdminPWD(std::string pwd)												{ m_strAdminPWD = pwd; }
			std::string	AdminPWD()																			{	return m_strAdminPWD;	}

			void				RegisterDBVersion(const std::string & strAlias, DBVersion & dbVersion)		{ m_mapDBVersions[strAlias] = dbVersion; }
			DBVersion		GetDBVersion(const std::string & strAlias)
			{ 
				DatabaseVersionMapItr	itr = m_mapDBVersions.find(strAlias);
			
				if (itr == m_mapDBVersions.end())
					throw std::runtime_error(std::string("D3::Settings: Version for Database ") + strAlias + " not registered");
					
				return itr->second;  	
			}
	};


	//  The Object class is the root of all the D3 classes
	//  in hierarchy. Object is a abstract class supporting the
	//  notions of object copying, archival, comparison and
	//  equivalence.
	//
	class D3_API Object
	{
		protected:
			static	const Class			M_Class;

		public:
			static  const Class&		ClassObject() { return M_Class; }
			virtual const Class&		IAm() const		{ return M_Class; }

		public:
			// Dtor() is virtual
			//
			virtual	~Object()	{};

			// Optional standard public interface
			//
			virtual	bool				Assign(ObjectPtr aObj);			// optional
			virtual	bool				Copy(ObjectPtr aObj);				// optional
			virtual	bool				DeepCopy(ObjectPtr aObj);		// optional
			virtual	int					Compare(ObjectPtr aObj);		// optional

			ObjectPtr						CreateCopy();
			ObjectPtr						CreateDeepCopy();

			bool								IsA(const Class& aCls) const					{ return &(IAm()) == &aCls; };
			bool								IsA(const std::string & strCls) const;
			bool								IsKindOf(const Class& aCls) const;
			bool								IsKindOf(const std::string & strCls) const;

#ifdef D3_ARCHIVER
			virtual	bool				GetFrom(Archiver & aArch);
			virtual	bool				PutTo(Archiver & aArch);
#endif

			// Object comparing: Overload Compare(Object&) and IsEqual(Object&) in
			//									 your Object subclass to implement proper comparing
			//
			bool								operator<=(Object & aObj)	{ return  Compare(&aObj) <= 0; }
			bool								operator>=(Object & aObj)	{ return  Compare(&aObj) >= 0; }
			bool								operator <(Object & aObj)	{ return  Compare(&aObj) <  0; }
			bool								operator >(Object & aObj)	{ return  Compare(&aObj) >  0; }
			bool								operator==(Object & aObj)	{ return  Compare(&aObj) == 0; }
			bool								operator!=(Object & aObj)	{ return  Compare(&aObj) != 0; }
	};


	//  The Class class is used to implement class identity. An
	//	instance of it is constructed for each class in class
	//	hierarchy. It holds the name, a null-constructor function,
	//	the size and a referent to the ancestor class of each
	//	defined class. The connection between an instance of a
	//	class and its corresponding instance of Class is made
	//	through the iam() member function.
	//

	typedef	ObjectPtr (*D3_CNSTRFUNC)();

	class D3_API Class : public Object
	{
		// Standard D3 stuff
		//
		protected:
			static	const Class			M_Class;

		public:
			static  const Class&		ClassObject() { return M_Class; }
			virtual const Class&		IAm() const		{ return M_Class; }

		protected:
			// Instance Variables
			//
			std::string					m_strName;			// Class name
			D3_CNSTRFUNC				m_pfnCtor;			// Function creating a class instance
			int									m_szObject;			// Shallow-copy class instance size
			Class*							m_pAncestor;		// Ancestor Class class object

		public:
			// Constructors and Destructors
			//
			Class(const std::string & strName, D3_CNSTRFUNC pfnCtor, int szObject);
			Class(const std::string & strName, D3_CNSTRFUNC pfnCtor, int szObject, Class* pAncestor);
			~Class();

			bool									operator==(const Class & aCls) const { return this == &aCls; }
			bool									operator!=(const Class & aCls) const { return this != &aCls; }

			bool									IsKindOf(const Class& aCls) const;

			// Object interogation
			int										Size() const						{ return m_szObject; };
			const std::string &		Name() const						{ return m_strName; };
			const Class&					Ancestor() const				{ return *m_pAncestor; };

			ObjectPtr							CreateInstance() const
			{
				if (m_pfnCtor)
					return (m_pfnCtor)();

				return NULL;
			}

			// Other public methods
			//
			static	ObjectPtr			CreateInstance(const std::string & strClsName);
			static	const Class&	Of(const std::string & strClsName);
			static	D3_CNSTRFUNC	Ctor(const std::string & strClsName);
			static	Class*				Find(const std::string & strClsName);

			// Other public methods
			//
			static	void					DumpHierarchy(ClassPtr pParent = NULL, const std::string & strPfx = "");

		protected:
			//! Returns a static std::list containing all class instances
			/*! Using simply a static class member is not a good idea. The reason is that this
					would create dependencies between static members and since there is no portable
					way to ensure that the collection of all class objects is created *before* any
					class object is created, we simply use a method to access the collection which
					on first use will create the collection.
					\note This mechanism is also know as the "construct-on-first-use" idiom. There is
					a small price to pay though: the allocated collection will normaly leak at program
					exit. In this implementation the leak is avoided in that the collection is deleted
					together with last class object it contains. This is an acceptable approach because
					class objects are expected to be added to the container during static initialisation
					and remove during static deinitialisation. If this behaviour is changed in the future
					(i.e. class objects are added and removed during normal program execution), this
					approach is no longer suitable.
			*/
			static ClassPtrList&  GetClassPtrList();

	};

} // end namespace D3

#endif /* _D3_H_ */
