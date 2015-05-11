#pragma once

#include <D3Types.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>


using namespace std;
using namespace D3;


string				getStringInput();
int						getIntegerInput();

class Menu;


class MenuOption
{
	friend class Menu;

	public:
		enum Separators
		{
			blankbefore=1,
			blankafter=2
		};

	protected:
		string					m_name;
		Menu*						m_pMenu;
		bool						m_enabled;
		bool						m_selected;
		unsigned int		m_separators;

	public:
		MenuOption(	const string&			strName, 
								Menu*							pMenu,
								bool							enabled = true,
								bool							selected = false,
								unsigned int			separators = 0)
		: m_name(strName),
			m_pMenu(pMenu),
			m_enabled(enabled),
			m_selected(selected),
			m_separators((Separators) separators)
		{}

		// subclass this to process a selection (not needed for Selection menus)
		virtual void					invoke()										{}

		void									menuShouldAbort();
		void									menuShouldBacktrack();
		void									menuShouldExit();

		virtual string				name()											{ return m_name; }
		virtual bool					enabled()										{ return m_enabled; }
		virtual bool					selected()									{ return m_selected; }
		virtual unsigned int	separators()								{ return m_separators; }

		virtual void					name(const string& name)		{ m_name = name; }
		virtual void					enable(bool bEnable)				{ m_enabled = bEnable; }
		virtual void					select(bool bSelect)				{ m_selected = bSelect; }
};
	
typedef boost::shared_ptr<MenuOption>			MenuOptionPtr;
typedef vector<MenuOptionPtr>							MenuOptionVect;



class Menu
{
	friend class MenuOption;

	public:
		enum AutoOptions
		{
			SelectOne=1,				// If set, remembers the MenuOption (use getChoice to get it) and terminates the menu
			SelectAll=2,				// this is a selection and all are selected initially
			SelectNone=4,				// this is a selection and none are selected initially
			Abort=8,						// show Abort option
			Back=16,						// show Back option
			Exit=32							// show Exit option
		};

	protected:
		string							m_strTitle;
		string							m_strSelectAllText;
		string							m_strSelectNoneText;
		string							m_strToggleText;
		string							m_strAbortText;
		string							m_strBackText;
		string							m_strExitText;
		MenuOptionVect			m_vectMenuOptions;
		MenuOptionPtr				m_pChoice;
		bool								m_bShouldExit;
		bool								m_bShouldAbort;
		bool								m_bShouldBacktrack;
		int									m_autoOptions;

	public:
		Menu(const string& strTitle = "Choose one of the following options:", int autoOptions = Exit)
			: m_strTitle(strTitle),
				m_bShouldExit(false),
				m_bShouldAbort(false),
				m_bShouldBacktrack(false),
				m_autoOptions(autoOptions),
				m_strSelectAllText("Select All"),
				m_strSelectNoneText("Unselect All"),
				m_strToggleText("Toggle selection"),
				m_strAbortText("Abort"),
				m_strBackText("Back"),
				m_strExitText("Exit"),
				m_pChoice(NULL)
		{}

		virtual unsigned int	optionCount()																	{ return m_vectMenuOptions.size(); }
		virtual void					run();

		virtual void					shouldExit(bool bShouldExit)									{ m_bShouldExit = bShouldExit; }
		virtual void					shouldAbort(bool bShouldAbort)								{ m_bShouldAbort = bShouldAbort; }
		virtual void					shouldBacktrack(bool bShouldBacktrack)				{ m_bShouldBacktrack = bShouldBacktrack; }

		virtual bool					shouldExit()																	{ return m_bShouldExit; }
		virtual bool					shouldAbort()																	{ return m_bShouldAbort; }
		virtual bool					shouldBacktrack()															{ return m_bShouldBacktrack; }

		virtual void					reset();

		virtual MenuOptionPtr	getChoice()																		{ return m_pChoice; }

		// set extra obtions text
		virtual const string& selectAllText()																{ return m_strSelectAllText; }
		virtual const string& selectNoneText()															{ return m_strSelectNoneText; }
		virtual const string& toggleText()																	{ return m_strToggleText; }
		virtual const string& abortText()																		{ return m_strAbortText; }
		virtual const string& backText()																		{ return m_strBackText; }
		virtual const string& exitText()																		{ return m_strExitText; }

		// setters
		virtual void					selectAllText(const string& strSelectAll)			{ m_strSelectAllText = strSelectAll; }
		virtual void					selectNoneText(const string& strSelectNone)		{ m_strSelectNoneText = strSelectNone; }
		virtual void					toggleText(const string& strToggle)						{ m_strToggleText = strToggle; }
		virtual void					abortText(const string& strAbort)							{ m_strAbortText = strAbort; }
		virtual void					backText(const string& strBack)								{ m_strBackText = strBack; }
		virtual void					exitText(const string& strExit)								{ m_strExitText = strExit; }

		virtual void					addOption(MenuOptionPtr pMO);

	protected:
		// set extra obtions text
		virtual const string& title()																				{ return m_strTitle; }

		// notifications
		virtual void					on_Selection(MenuOptionPtr pMO);					// fires when one of the options added with addOption is selected

		virtual void					on_SelectAll();
		virtual void					on_SelectNone();
		virtual void					on_Toggle();
		virtual void					on_Abort()																		{ m_bShouldAbort = true; }
		virtual void					on_Back()																			{ m_bShouldBacktrack = true; }
		virtual void					on_Exit()																			{ m_bShouldExit = true; }
};


inline 	void	MenuOption::menuShouldAbort()									{ m_pMenu->shouldAbort(true); }
inline 	void	MenuOption::menuShouldBacktrack()							{ m_pMenu->shouldBacktrack(true); }
inline 	void	MenuOption::menuShouldExit()									{ m_pMenu->shouldExit(true); }



// Generic menus follow

// Allows the user to select one or all meta database objects in the system a invoke an action on them
class MenuSelectMetaDatabases : public Menu
{
	public:
		enum Mode
		{
			All,
			ExcludeMetaDictionary,
			ExcludeHelpDatabase,
			ExcludeSystemDatabase
		};

	protected:
		Mode							m_eMode;

	public:
		MenuSelectMetaDatabases(const string& strTitle, int autoOptions = SelectAll|Back|Abort|Exit, Mode eMode = All);

		vector<MetaDatabasePtr>		getSelection();			// use this when this was constructed with auto options SelectAll or SelectNone
		MetaDatabasePtr						getSelected();			// use this when this was constructed with auto options SelectOne
};



// Allows the user to select meta entities of a meta database object
class MenuSelectMetaEntities : public Menu
{
	public:
		enum Mode
		{
			All,
			ExcludeVersionInfo
		};

	public:
		MenuSelectMetaEntities(const string& strTitle, int autoOptions = SelectAll|Back|Abort|Exit, Mode eMode = ExcludeVersionInfo);
		MenuSelectMetaEntities(const string& strTitle, MetaDatabasePtr pMD, int autoOptions = SelectAll|Back|Abort|Exit, Mode eMode = ExcludeVersionInfo);
		MenuSelectMetaEntities(const string& strTitle, vector<MetaEntityPtr>& vectME, int autoOptions = SelectAll|Back|Abort|Exit, Mode eMode = ExcludeVersionInfo);

		vector<MetaEntityPtr>			getSelection();			// use this when this was constructed with auto options SelectAll or SelectNone
		MetaEntityPtr							getSelected();			// use this when this was constructed with auto options SelectOne
};



// Allows the user to enter the name of a file
class MenuGetFileName : public Menu
{
	public:
		enum Mode
		{
			Any,
			MustExist,
			PromptBeforeOverwrite,
			MustNotExist
		};

	protected:
		string					m_strFileName;
		Mode						m_eMode;
		bool						m_bBackTrack;					// true after user chooses menu option "Back"

	public:
		MenuGetFileName(const string& strTitle, int autoOptions = Back|Abort|Exit, Mode eMode = Any);

		string&					fileName()															{ return m_strFileName; }
		bool						fileName(const string& strFileName)			{ m_strFileName = strFileName; }

		//! Returns an error message if the file name isn't valid, otherwise the string is empty.
		string					validateFileName();
};
