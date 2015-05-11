#include "types.h"
#include "Menu.h"
#include <boost/filesystem.hpp>

using namespace std;
using namespace D3;



string getStringInput()
{
	char		szBuffer[4096];
	string	strDummy;

	while (true)
	{
		cin.getline(szBuffer, 4096);

		if (cin.fail())
		{
			cin.clear();
			cout << "Invalid input, please try again.\n";
			break;
		}

		strDummy = szBuffer;
		break;
	}

	return strDummy;
}


int getIntegerInput()
{
	int						iInput = -1;
	string				strInput;
	char					buffer[20];

	strInput = getStringInput();

	if (!strInput.empty())
	{
		iInput = atoi(strInput.c_str());
		::itoa(iInput, buffer, 10);

		if (strInput != buffer)
			iInput = -1;
	}

	return iInput;
}


void Menu::run()
{
	unsigned int iChoice;

	while (!m_vectMenuOptions.empty() && !m_bShouldBacktrack && !m_bShouldAbort && !m_bShouldExit)
	{
		unsigned int	numWidth = 1, num = m_vectMenuOptions.size(), maxChoice = 0;
		int						choiceSelectAll=-1, choiceSelectNone=-1, choiceToggle=-1, choiceAbort=-1, choiceBack=-1, choiceExit=-1;

		while (num > 0)
		{
			numWidth++; 
			num /= 10;
		}
		
		cout << endl;
		cout << endl;

		cout << title() << endl;
		cout << string(title().size(), '-') << endl;

		for (unsigned int i = 0; i < m_vectMenuOptions.size(); i++)
		{
			MenuOptionPtr			mo = m_vectMenuOptions[i];

			if (mo->separators() & MenuOption::blankbefore)
				cout << endl;

			maxChoice = i + 1;

			if (mo->enabled())
			{
				if (m_autoOptions & (SelectAll | SelectNone))
					cout << setfill(' ') << setw(numWidth) << maxChoice << "  " << (mo->selected() ? "Selected => " : "....................................Excluded => ") << mo->name() << endl;
				else
					cout << setfill(' ') << setw(numWidth) << maxChoice << "  " << mo->name() << endl;
			}

			if (mo->separators() & MenuOption::blankafter)
				cout << endl;
		}

		// Options if this is a selection menu
		if (m_autoOptions & (SelectAll | SelectNone))
		{
			cout << endl;
			
			maxChoice++;
			choiceSelectAll = maxChoice;
			cout << setfill(' ') << setw(numWidth) << maxChoice << "  " << selectAllText() << endl;
			
			maxChoice++;
			choiceSelectNone = maxChoice;
			cout << setfill(' ') << setw(numWidth) << maxChoice << "  " << selectNoneText()  << endl;
			
			maxChoice++;
			choiceToggle = maxChoice;
			cout << setfill(' ') << setw(numWidth) << maxChoice << "  " << toggleText()  << endl;
		}

		// Option group Abort/Back
		if (m_autoOptions & (Abort | Back))
		{
			cout << endl;
			
			if (m_autoOptions & Abort)
			{
				maxChoice++;
				choiceAbort = maxChoice;
				cout << setfill(' ') << setw(numWidth) << maxChoice << "  " << abortText() << endl;
			}
			
			if (m_autoOptions & Back)
			{
				maxChoice++;
				choiceBack = maxChoice;
				cout << setfill(' ') << setw(numWidth) << maxChoice << "  " << backText() << endl;
			}
		}

		if (!maxChoice)
			break;

		if (m_autoOptions & Exit)
		{
			choiceExit = 0;
			cout << endl;
			cout << setfill(' ') << setw(numWidth) << 0 << "  " << exitText() << endl;
		}

		cout << endl;
		cout << endl;

		cout << "Select option: ";

		try
		{
			iChoice = (unsigned int) getIntegerInput();

			if (iChoice < 0 || iChoice > maxChoice)
				cout << "Invalid choice" << endl;
			else if (iChoice == choiceSelectAll)
				on_SelectAll();
			else if (iChoice == choiceSelectNone)
				on_SelectNone();
			else if (iChoice == choiceToggle)
				on_Toggle();
			else if (iChoice == choiceAbort) 
				on_Abort();
			else if (iChoice == choiceBack) 
				on_Back();
			else if (iChoice == choiceExit)
				on_Exit();
			else if (iChoice < 0 || iChoice > maxChoice) 
				throw runtime_error("Invalid choice!");
			else if (iChoice > 0 && iChoice <= m_vectMenuOptions.size())
			{
				for (unsigned int i = 0, iCrnt = 0; i < m_vectMenuOptions.size(); i++)
				{
					MenuOptionPtr		pMO = m_vectMenuOptions[i];

					if (pMO->enabled())
						iCrnt++;

					if (iChoice == iCrnt)
					{
						on_Selection(pMO);
						break;
					}
				}
			}
		}
		catch(...)
		{
			string		strError = D3::ShortGenericExceptionHandler();

			D3::ReportError(strError.c_str());
			cout << strError << endl;
		}
	}
}



void Menu::reset()
{
	m_bShouldExit = m_bShouldAbort = m_bShouldBacktrack = false;

	// clear the selection if this is a 
	if (m_autoOptions & (SelectAll | SelectNone))
	{
		for (unsigned int i=0; i < m_vectMenuOptions.size(); i++)
		{
			MenuOptionPtr		pMO = m_vectMenuOptions[i];
			pMO->m_selected = !pMO->m_enabled || (m_autoOptions & SelectNone) ? false : true;
		}
	}
}



void Menu::on_Selection(MenuOptionPtr pMO)
{
	if (m_autoOptions & (SelectAll | SelectNone))
	{
		pMO->select(!pMO->selected());
	}
	else
	{
		if (m_pChoice)
			m_pChoice->select(false);

		m_pChoice = pMO;
		m_pChoice->select(true);
	}

	pMO->invoke();

	if (m_autoOptions & SelectOne)
		m_bShouldExit = true;
}



void Menu::on_SelectAll()
{
	for (unsigned int i = 0; i < m_vectMenuOptions.size(); i++)
	{
		MenuOptionPtr		pMO = m_vectMenuOptions[i];

		if (pMO->enabled())
			pMO->select(true);
	}
}



void Menu::on_SelectNone()
{
	for (unsigned int i = 0; i < m_vectMenuOptions.size(); i++)
	{
		MenuOptionPtr		pMO = m_vectMenuOptions[i];
		pMO->select(false);
	}
}



void Menu::on_Toggle()
{
	for (unsigned int i = 0; i < m_vectMenuOptions.size(); i++)
	{
		MenuOptionPtr		pMO = m_vectMenuOptions[i];

		if (pMO->enabled())
			pMO->select(!pMO->selected());
	}
}



void Menu::addOption(MenuOptionPtr pMO)
{ 
	m_vectMenuOptions.push_back(pMO);

	if (m_autoOptions & (SelectAll | SelectNone))
		pMO->select(m_autoOptions & SelectAll ? true : false);
}



// This menu allows you to select a meta database from a list
class MOSelectMetaDatabase : public MenuOption
{
	public:
		MetaDatabasePtr				m_pMD;
		
		MOSelectMetaDatabase(	Menu*					pMenu,
													MetaDatabase* pMD)
		: MenuOption(pMD->GetAlias() + " (" + pMD->GetName() + ')', pMenu), 
			m_pMD(pMD)
		{}
};



MenuSelectMetaDatabases::MenuSelectMetaDatabases(const string& strTitle, int autoOptions, Mode eMode)
: Menu(strTitle, autoOptions), 
	m_eMode(eMode)
{
	MetaDatabasePtrMapItr		itrMD;
	MetaDatabasePtr					pMDDict = MetaDatabase::GetMetaDictionary();
	MetaDatabasePtr					pMDHelp = MetaDatabase::GetMetaDatabase("D3HSDB");

	// Create the physical database for each we found
	//
	for (	itrMD =  MetaDatabase::GetMetaDatabases().begin();
				itrMD != MetaDatabase::GetMetaDatabases().end();
				itrMD++)
	{
		MetaDatabasePtr	pMD = itrMD->second;
		MenuOptionPtr		mo(new MOSelectMetaDatabase(this, pMD));

		if (((m_eMode & ExcludeMetaDictionary) && pMD == pMDDict) ||
				((m_eMode & ExcludeHelpDatabase)   && pMD == pMDHelp))
				mo->enable(false);

		addOption(mo);
	}
}



vector<MetaDatabasePtr> MenuSelectMetaDatabases::getSelection()
{
	vector<MetaDatabasePtr>			vectMD;
	MetaDatabasePtr							pMDDict = MetaDatabase::GetMetaDictionary();
	MetaDatabasePtr							pMDHelp = MetaDatabase::GetMetaDatabase("D3HSDB");

	vectMD.reserve(m_vectMenuOptions.size());

	for (unsigned int i=0; i < m_vectMenuOptions.size(); i++)
	{
		MOSelectMetaDatabase* pMOSelMD = (MOSelectMetaDatabase*) (m_vectMenuOptions[i].get());

		if (((m_eMode & ExcludeMetaDictionary) && pMOSelMD->m_pMD == pMDDict) ||
				((m_eMode & ExcludeHelpDatabase)   && pMOSelMD->m_pMD == pMDHelp))
			continue;

		if (pMOSelMD->selected())
			vectMD.push_back(pMOSelMD->m_pMD);
	}

	return vectMD;		
}



MetaDatabasePtr MenuSelectMetaDatabases::getSelected()
{
	MOSelectMetaDatabase* pMOSelMD = (MOSelectMetaDatabase*) (m_pChoice.get());

		if (pMOSelMD)
			return pMOSelMD->m_pMD;

	return NULL;		
}





// This menu allows you to select a meta entity from a list
class MOSelectMetaEntity : public MenuOption
{
	public:
		MetaEntityPtr				m_pME;

		MOSelectMetaEntity(	Menu*				pMenu,
												MetaEntity* pME) 
		: MenuOption(pME->GetMetaDatabase()->GetAlias() + " (" + pME->GetName() + ')', pMenu),
			m_pME(pME)
		{}
};



MenuSelectMetaEntities::MenuSelectMetaEntities(const string& strTitle, int autoOptions, Mode eMode)
: Menu(strTitle, autoOptions)
{
	for (MetaDatabasePtrMapItr itr = MetaDatabase::GetMetaDatabases().begin();
			itr != MetaDatabase::GetMetaDatabases().end();
			itr++)
	{
		MetaEntityPtrVectPtr			pVectME = itr->second->GetMetaEntities();

		for (unsigned int j=0; j < pVectME->size(); j++)
		{
			MetaEntityPtr	pME = pVectME->operator[](j);

			if (eMode != ExcludeVersionInfo || pME != pME->GetMetaDatabase()->GetVersionInfoMetaEntity())
				addOption(MenuOptionPtr(new MOSelectMetaEntity(this, pME)));
		}
	}
}



MenuSelectMetaEntities::MenuSelectMetaEntities(const string& strTitle, MetaDatabasePtr pMD, int autoOptions, Mode eMode)
: Menu(strTitle, autoOptions)
{
	MetaEntityPtrVectPtr			pVectME = pMD->GetMetaEntities();

	for (unsigned int i=0; i < pVectME->size(); i++)
	{
		MetaEntityPtr	pME = pVectME->operator[](i);

		if (eMode != ExcludeVersionInfo || pME != pME->GetMetaDatabase()->GetVersionInfoMetaEntity())
			addOption(MenuOptionPtr(new MOSelectMetaEntity(this, pME)));
	}
}



MenuSelectMetaEntities::MenuSelectMetaEntities(const string& strTitle, vector<MetaEntityPtr>& vectME, int autoOptions, Mode eMode)
: Menu(strTitle, autoOptions)
{
	for (unsigned int i=0; i < vectME.size(); i++)
	{
		MetaEntityPtr	pME = vectME[i];

		if (eMode != ExcludeVersionInfo || pME != pME->GetMetaDatabase()->GetVersionInfoMetaEntity())
			addOption(MenuOptionPtr(new MOSelectMetaEntity(this, pME)));
	}
}



vector<MetaEntityPtr> MenuSelectMetaEntities::getSelection()
{
	vector<MetaEntityPtr>			vectME;

	vectME.reserve(m_vectMenuOptions.size());

	for (unsigned int i=0; i < m_vectMenuOptions.size(); i++)
	{
		MOSelectMetaEntity* pMOSelME = (MOSelectMetaEntity*) (m_vectMenuOptions[i].get());

		if (pMOSelME->selected())
			vectME.push_back(pMOSelME->m_pME);
	}

	return vectME;		
}



MetaEntityPtr MenuSelectMetaEntities::getSelected()
{
	MOSelectMetaEntity* pMOSelME = (MOSelectMetaEntity*) (m_pChoice.get());

		if (pMOSelME)
			return pMOSelME->m_pME;

	return NULL;		
}





class MOEditFileName : public MenuOption
{
	protected:
		MenuGetFileName*	m_pMenuGetFileName;
		string&						m_strFileName;

	public:
		MOEditFileName(const string& strTitle, MenuGetFileName* pMenu)
			: MenuOption(strTitle, pMenu), m_pMenuGetFileName(pMenu), m_strFileName(pMenu->fileName())
		{}

		virtual string		name()
		{
			ostringstream		strm;

			strm << m_name;

			if (!m_strFileName.empty())
				strm << " (" << m_strFileName << ") ";

			return strm.str();
		}

		virtual void			invoke()
		{
			string		strError, strFN = m_strFileName;

			cout << endl;
			cout << "File name";

			if (!m_strFileName.empty())
				cout << " (" << m_strFileName << ")";

			cout << ": ";

			string strFileName = getStringInput();

			if (!strFileName.empty())
			{
				m_strFileName = strFileName;
				strError = m_pMenuGetFileName->validateFileName();

				if (!strError.empty())
				{
					cout << "ERROR: " << strError << endl;
					m_strFileName = strFN;
				}
			}
		}
};




MenuGetFileName::MenuGetFileName(const string& strTitle, int autoOptions, Mode eMode)
: Menu(strTitle, autoOptions), m_eMode(eMode)
{
	addOption(MenuOptionPtr(new MOEditFileName("Enter file name", this)));
}



string MenuGetFileName::validateFileName()
{
	string										strErrorText;

	if (m_strFileName.empty())
	{
		strErrorText = "no file name specified";
	}
	else
	{
		try
		{
			boost::filesystem::path		path(m_strFileName);
			string										fn = path.filename().string();

			path = path.remove_filename();

			if (path.string().empty())
				path = ".";

			path = boost::filesystem::canonical(path);

			if (!boost::filesystem::native(fn))
			{
				strErrorText = string("file name ") + fn + " is not valid";
			} 
			else if (!boost::filesystem::is_directory(path.string()))
			{
				strErrorText = string("folder ") + path.string() + " does not exist";
			}
			else
			{
				switch (m_eMode)
				{
					case MustExist:
					{
						if (!boost::filesystem::exists(m_strFileName))
							strErrorText = m_strFileName + " not found";

						break;
					}

					case PromptBeforeOverwrite:
					{
						if (boost::filesystem::exists(m_strFileName))
						{
							cout << "WARNING: File " << m_strFileName << " exists, ok to overwrite [YES=overwrite]? ";

							if (boost::to_upper_copy(getStringInput()) != "YES")
								strErrorText = m_strFileName + " exists and should not be overridden";
						}

						break;
					}

					case MustNotExist:
					{
						if (boost::filesystem::exists(m_strFileName))
							strErrorText = m_strFileName + " must not exists";

						break;
					}
				}
			}

			if (strErrorText.empty())
			{
				path /= fn;
				m_strFileName = path.string();
			}

		}
		catch (...)
		{
			strErrorText = ShortGenericExceptionHandler();
		}
	}

	return strErrorText;
}
