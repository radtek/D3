#include "types.h"
#include "MenuMDLoaded.h"
#include "MenuPlay.h"

using namespace D3;
using namespace std;


// helpers




/*
		cout << "1  Create Custom Database(s)" << endl;
		cout << "2  Create Custom Database(s) Source Code" << endl;
		cout << "3  Dump Meta Data for Database(s)" << endl;
		cout << "4  Play with Custom Database(s)" << endl;
		cout << "5  Export Custom Database(s)" << endl;
		cout << "6  Import Custom Database(s)" << endl;
		cout << "7  Export Meta Dictionary" << endl;
		cout << "8  Export RBAC Data" << endl;
*/


class MOCreateCustomDatabase : public MenuOption
{
	public:
		MOCreateCustomDatabase(Menu* pMenu) : MenuOption("Create Custom Database(s)", pMenu) {}

		virtual void			invoke()
		{
			vector<MetaDatabasePtr>			vectMD;
			MenuSelectMetaDatabases			menuSelectMetaDatabases("Select custom database(s) to create:", Menu::SelectAll|Menu::Abort|Menu::Exit, MenuSelectMetaDatabases::ExcludeMetaDictionary); 

			menuSelectMetaDatabases.exitText("Create selected databases");
			menuSelectMetaDatabases.run();

			if (menuSelectMetaDatabases.shouldExit())
			{
				vectMD = menuSelectMetaDatabases.getSelection();

				if (!vectMD.empty())
				{
					// generate warning meessage
					std::cout << endl << "WARNING: This action will destroy the following databases if they exist:\n\n";

					for (unsigned int i=0; i < vectMD.size(); i++)
					{
						MetaDatabasePtr		pMD = vectMD[i];
						cout << pMD->GetName() << " (" << pMD->GetAlias() << ")\n";
					}

					cout << endl << "Type YES to confirm: ";

					if (boost::to_upper_copy(getStringInput()) == "YES")
					{
						for (unsigned int i=0; i < vectMD.size(); i++)
						{
							MetaDatabasePtr		pMD = vectMD[i];

							if (pMD->CreatePhysicalDatabase())
								cout << "SUCCESS: Database " << pMD->GetName() << " created." << endl;
							else
								cout << "ERROR: Failed to create Database " << pMD->GetName() << "." << endl;
						}
					}
				}
			}
		}
};



class MOCreateCustomDatabaseSourceCode : public MenuOption
{
	public:
		MOCreateCustomDatabaseSourceCode(Menu* pMenu) : MenuOption("Create Database(s) Source Code", pMenu) {}

		virtual void			invoke()
		{
			vector<MetaDatabasePtr>			vectMD;
			MenuSelectMetaDatabases			menuSelectMetaDatabases("Select the database(s) for which you would like to generate source code:", Menu::SelectAll|Menu::Abort|Menu::Exit); 

			menuSelectMetaDatabases.exitText("Generate source code for selected databases");
			menuSelectMetaDatabases.run();

			if (menuSelectMetaDatabases.shouldExit())
			{
				vectMD = menuSelectMetaDatabases.getSelection();

				for (unsigned int i=0; i < vectMD.size(); i++)
				{
					MetaDatabasePtr		pMD = vectMD[i];

					if (pMD->GenerateSourceCode())
						cout << "SUCCESS: Source code for database " << pMD->GetName() << " has been placed in ./generated." << endl;
					else
						cout << "ERROR: Failed to create source code for database " << pMD->GetName() << ". Check if the destination folder ./generated exists." << endl;
				}
			}
		}
};



class MODumpMetaDictionary : public MenuOption
{
	public:
		MODumpMetaDictionary(Menu* pMenu) : MenuOption("Dump Meta Data for Database(s)", pMenu) {}

		virtual void			invoke()
		{
			vector<MetaDatabasePtr>			vectMD;
			MenuSelectMetaDatabases			menuSelectMetaDatabases("Select the database(s) which you would like to dump the meta data to this screen:", Menu::SelectAll|Menu::Abort|Menu::Exit); 

			menuSelectMetaDatabases.exitText("Dump selected databases meta data to screen");
			menuSelectMetaDatabases.run();

			if (menuSelectMetaDatabases.shouldExit())
			{
				vectMD = menuSelectMetaDatabases.getSelection();

				for (unsigned int i=0; i < vectMD.size(); i++)
				{
					MetaDatabasePtr		pMD = vectMD[i];
					pMD->Dump();
				}
			}
		}
};



class MOPlayWithCustomDatabase : public MenuOption
{
	public:
		MOPlayWithCustomDatabase(Menu* pMenu) : MenuOption("Play with Custom Database(s)", pMenu) {}

		void			invoke()
		{
			MenuPlay			menu;
			menu.run();
		}
};



class MOExportCustomDatabase : public MenuOption
{
	public:
		MOExportCustomDatabase(Menu* pMenu) : MenuOption("Export Custom Database(s)", pMenu) {}

		virtual void			invoke()
		{
			MenuSelectMetaDatabases			menuSelectMetaDatabases("Select custom database to export:", Menu::SelectOne|Menu::Abort|Menu::Exit, MenuSelectMetaDatabases::ExcludeMetaDictionary); 
			bool												bExit = false;

			menuSelectMetaDatabases.exitText("Export data from selected databases");

			while (!bExit)
			{
				menuSelectMetaDatabases.run();

				if (menuSelectMetaDatabases.shouldExit())
				{
					MetaDatabasePtr				pMD = menuSelectMetaDatabases.getSelected();
					MetaEntityPtrList			listEntities;
					vector<MetaEntityPtr>	vectME;

					if (pMD)
						listEntities = pMD->GetDependencyOrderedMetaEntities();

					if (listEntities.empty())
						break;

					vectME.reserve(listEntities.size());

					for (MetaEntityPtrListItr itr = listEntities.begin(); itr != listEntities.end(); itr++)
						vectME.push_back(*itr);

					MenuSelectMetaEntities		menuSelectMetaEntities("Select the entities you would like to export", vectME);

					menuSelectMetaEntities.exitText("Export data from selected entities");
					menuSelectMetaEntities.run();

					if (menuSelectMetaEntities.shouldExit())
					{
						vector<MetaEntityPtr>			vectMESelected;

						vectMESelected = menuSelectMetaEntities.getSelection();

						if (!vectMESelected.empty())
						{
							MenuGetFileName			menuGetFileName("Enter the path and/or name of the export file to create:", Menu::Back|Menu::Abort|Menu::Exit, MenuGetFileName::PromptBeforeOverwrite);

							menuGetFileName.run();
							
							if (menuGetFileName.shouldExit())
							{
								try
								{
									DatabaseWorkspace		ws;
									DatabasePtr					pDB;

									pDB = ws.GetDatabase(pMD);

									if (pDB)
									{
										MetaEntityPtrList		listMESelected;

										for (unsigned int i=0; i < vectMESelected.size(); i++)
											listMESelected.push_back(vectMESelected[i]);

										pDB->ExportToXML(APP_NAME, menuGetFileName.fileName(), &listMESelected);

										cout << "SUCCESS: Exported data to " << menuGetFileName.fileName() << endl;
									}
									else
									{
										cout << "ERROR: Failed to open database\n";
									}
								}
								catch(...)
								{
									cout << "ERROR: " << ShortGenericExceptionHandler() << endl;
								}
							}

							if (menuGetFileName.shouldBacktrack())
								continue;

							break;
						}
					}

					if (menuSelectMetaEntities.shouldBacktrack())
						continue;

					break;		// abort
				}
			}
		}
};



class MOImportCustomDatabase : public MenuOption
{
	public:
		MOImportCustomDatabase(Menu* pMenu) : MenuOption("Import Custom Database(s)", pMenu) {}

		virtual void			invoke()
		{
			MenuSelectMetaDatabases			menuSelectMetaDatabases("Select custom database to import:", Menu::SelectOne|Menu::Abort|Menu::Exit, MenuSelectMetaDatabases::ExcludeMetaDictionary); 
			bool												bExit = false;

			menuSelectMetaDatabases.exitText("Import data from selected databases");

			while (!bExit)
			{
				menuSelectMetaDatabases.run();

				if (menuSelectMetaDatabases.shouldExit())
				{
					MetaDatabasePtr				pMD = menuSelectMetaDatabases.getSelected();
					MetaEntityPtrList			listEntities;
					vector<MetaEntityPtr>	vectME;

					if (pMD)
						listEntities = pMD->GetDependencyOrderedMetaEntities();

					if (listEntities.empty())
						break;

					vectME.reserve(listEntities.size());

					for (MetaEntityPtrListItr itr = listEntities.begin(); itr != listEntities.end(); itr++)
						vectME.push_back(*itr);

					MenuSelectMetaEntities		menuSelectMetaEntities("Select the entities you would like to import", vectME);

					menuSelectMetaEntities.exitText("Import data from selected entities");
					menuSelectMetaEntities.run();

					if (menuSelectMetaEntities.shouldExit())
					{
						vector<MetaEntityPtr>			vectMESelected;

						vectMESelected = menuSelectMetaEntities.getSelection();

						if (!vectMESelected.empty())
						{
							MenuGetFileName			menuGetFileName("Enter the path and/or name of the import file:", Menu::Back|Menu::Abort|Menu::Exit, MenuGetFileName::MustExist);

							menuGetFileName.run();
							
							if (menuGetFileName.shouldExit())
							{
								try
								{
									DatabaseWorkspace		ws;
									DatabasePtr					pDB;

									pDB = ws.GetDatabase(pMD);

									if (pDB)
									{
										MetaEntityPtrList		listMESelected;

										for (unsigned int i=0; i < vectMESelected.size(); i++)
											listMESelected.push_back(vectMESelected[i]);

										pDB->ImportFromXML(APP_NAME, menuGetFileName.fileName(), &listMESelected);

										cout << "SUCCESS: Imported data from " << menuGetFileName.fileName() << endl;
									}
									else
									{
										cout << "ERROR: Failed to open database\n";
									}
								}
								catch(...)
								{
									cout << "ERROR: " << ShortGenericExceptionHandler() << endl;
								}
							}

							if (menuGetFileName.shouldBacktrack())
								continue;

							break;
						}
					}

					if (menuSelectMetaEntities.shouldBacktrack())
						continue;

					break;		// abort
				}
			}
		}
};



class MOExportMetaDictionary : public MenuOption
{
	public:
		MOExportMetaDictionary(Menu* pMenu) : MenuOption("Export Meta Dictionary", pMenu) {}

		virtual void			invoke()
		{
			MetaDatabasePtr						pMD = MetaDatabase::GetMetaDictionary();
			MetaEntityPtrList					listME;
			D3MDDB_Tables							aryEntityID[] = { D3MDDB_D3MetaDatabase,
																									D3MDDB_D3MetaEntity,
																									D3MDDB_D3MetaColumn,
																									D3MDDB_D3MetaColumnChoice,
																									D3MDDB_D3MetaKey,
																									D3MDDB_D3MetaKeyColumn,
																									D3MDDB_D3MetaRelation
																								};
			DatabaseWorkspace					dbWS;
			DatabasePtr								pDB;
			string										strFN = GetNativeAppPath() + "\\" + APP_NAME + "MD.xml";

			// Set MetaEntity list content
			for (unsigned int idx = 0; idx < sizeof(aryEntityID)/sizeof(D3MDDB_Tables); idx++)
				listME.push_back(pMD->GetMetaEntity(aryEntityID[idx]));

			// Get the instance we need to work with
			pDB = dbWS.GetDatabase(pMD);

			if (pDB->ExportToXML(APP_NAME, strFN, &listME) > 0)
				cout << "Export Meta Dictionary succeeded!" << endl;
			else
				cout << "Importing Meta Dictionary failed!" << endl;
		}
};



class MOExportRBACData : public MenuOption
{
	public:
		MOExportRBACData(Menu* pMenu) : MenuOption("Export RBAC Data", pMenu) {}

		virtual void			invoke()
		{
		}
};



MenuMDLoaded::MenuMDLoaded()
{
	addOption(MenuOptionPtr(new MOCreateCustomDatabase(this)));
	addOption(MenuOptionPtr(new MOCreateCustomDatabaseSourceCode(this)));
	addOption(MenuOptionPtr(new MODumpMetaDictionary(this)));
	addOption(MenuOptionPtr(new MOPlayWithCustomDatabase(this)));
	addOption(MenuOptionPtr(new MOExportCustomDatabase(this)));
	addOption(MenuOptionPtr(new MOImportCustomDatabase(this)));
	addOption(MenuOptionPtr(new MOExportMetaDictionary(this)));
	addOption(MenuOptionPtr(new MOExportRBACData(this)));
};
