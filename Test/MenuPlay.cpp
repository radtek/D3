#include "types.h"
#include "MenuPlay.h"

using namespace D3;
using namespace std;

#define USE_VIEW

void QuickTest(DatabaseWorkspace& ws)
{
	// let's execute a query as JSON
	DatabasePtr			pDB = ws.GetDatabase("SCHOOLDB");
	ostringstream		strm;

#ifdef USE_VIEW
	const char*			pszSQL =  "SELECT    sub.name,\n"
														"          sub.level,\n"
														"          cls.room,\n"
														"          tch.firstname,\n"
														"          tch.familyname,\n"
														"          std.firstname AS StudentFirstName,\n"
														"          std.familyname AS StudentName\n"
														"FROM      Enrolment e LEFT JOIN\n"
														"          SubjectClass cls on e.SubjectClassID=cls.ID LEFT JOIN\n"
														"          Subject sub on cls.SubjectID=sub.ID LEFT JOIN\n"
														"          Teacher tch on cls.TeacherID=tch.ID LEFT JOIN\n"
														"          Student std on e.StudentID=std.ID\n"
														"ORDER BY  sub.name,\n"
														"          sub.level,\n"
														"          cls.room,\n"
														"          tch.familyname,\n"
														"          tch.firstname,\n"
														"          std.familyname,\n"
														"          std.firstname";
#else
	const char*			pszSQL =  "SELECT * FROM Student";
#endif

	cout << "SQL Query:" << endl;
	cout << "----------" << endl;
	cout << pszSQL << endl;
	cout << endl;
	
	pDB->QueryToJSONWithMetaData(pszSQL, strm);

	cout << "Result:" << endl;
	cout << "-------" << endl;
	cout << strm.str() << endl;
	cout << endl;
}


void InspectObject(EntityPtr pEntity);

bool EditObject(EntityPtr pEntity)
{
	ColumnPtr			pCol;
	unsigned int	idx, iWidth = 0;
	int						iChoice;
	std::string		strInput;


	// Get the longest column label
	//
	for (idx = 0; idx < pEntity->GetColumnCount(); idx++)
	{
		pCol = pEntity->GetColumn(idx);

		if (iWidth < pCol->GetMetaColumn()->GetName().size())
			iWidth = pCol->GetMetaColumn()->GetName().size();
	}

	iWidth += 3;

	while (true)
	{
		std::cout << std::endl;
		std::cout << std::endl;
		std::cout << "Select the column to change:" << std::endl;
		std::cout << "----------------------------" << std::endl;

		for (idx = 0; idx < pEntity->GetColumnCount(); idx++)
		{
			pCol = pEntity->GetColumn(idx);

			std::cout << idx+1 << "  " << pCol->GetMetaColumn()->GetName();
			std::cout << std::setw(iWidth - pCol->GetMetaColumn()->GetName().size()) << ": ";
			std::cout << pCol->AsString() << std::endl;
		}

		std::cout << idx+1 << "  Abort update" << std::endl;
		std::cout << std::endl;
		std::cout << 0 << "  Finished" << std::endl;
		std::cout << std::endl;
		std::cout << std::endl;

		std::cout << "Select option: ";

		iChoice = getIntegerInput();

		if (iChoice == 0)
			return true;

		if (iChoice == pEntity->GetColumnCount() + 1)
			return false;

		if (iChoice < 0 || iChoice > pEntity->GetColumnCount())
		{
			std::cout << "Invalid option, please try again" << std::endl;
			continue;
		}

		idx = (unsigned int) iChoice;
		idx--;

		pCol = pEntity->GetColumn(idx);

		std::cout << pCol->GetMetaColumn()->GetName() << ": ";
		strInput = getStringInput();

		try
		{
			if (!pCol->SetValue(strInput))
				std::cout << "Failed to change value!" << std::endl;
		}
		catch (Exception & e)
		{
			e.LogError();
			std::cout << "A D3::Exception occurred, check the log!" << std::endl;
		}

		catch (...)
		{
			std::cout << "An unknown exception occurred." << std::endl;
		}
	}
}



bool EditKey(KeyPtr pKey)
{
	ColumnPtr					pCol;
	ColumnPtrListItr	itrCol;
	int								iWidth = 0;
	int								iChoice, idx;
	std::string				strInput;


	// Get the longest column label
	//
	for ( itrCol =  pKey->GetColumns().begin();
				itrCol != pKey->GetColumns().end();
				itrCol++)
	{
		pCol = *itrCol;

		if (iWidth < pCol->GetMetaColumn()->GetName().size())
			iWidth = pCol->GetMetaColumn()->GetName().size();
	}

	iWidth += 3;

	while (true)
	{
		std::cout << std::endl;
		std::cout << std::endl;
		std::cout << "Select the key column to change:" << std::endl;
		std::cout << "--------------------------------" << std::endl;

		for ( itrCol =  pKey->GetColumns().begin(), idx = 1;
					itrCol != pKey->GetColumns().end();
					itrCol++,                             idx++)

		{
			pCol = *itrCol;

			std::cout << idx << "  " << pCol->GetMetaColumn()->GetName();
			std::cout << std::setw(iWidth - pCol->GetMetaColumn()->GetName().size()) << ": ";
			std::cout << pCol->AsString() << std::endl;
		}

		std::cout << idx << "  Abort editing key" << std::endl;
		std::cout << std::endl;
		std::cout << 0 << "  Finished" << std::endl;
		std::cout << std::endl;
		std::cout << std::endl;

		std::cout << "Select option: ";

		iChoice = getIntegerInput();

		if (iChoice == 0)
			return true;

		if (iChoice == pKey->GetColumnCount() + 1)
			return false;

		if (iChoice < 0 || iChoice > pKey->GetColumnCount())
		{
			std::cout << iChoice << " is an invalid option, try again." << std::endl;
			continue;
		}

		pCol = NULL;

		for ( itrCol =  pKey->GetColumns().begin(), idx = 1;
					itrCol != pKey->GetColumns().end();
					itrCol++,                             idx++)
		{
			if (idx == iChoice)
			{
				pCol = *itrCol;
				break;
			}
		}

		std::cout << pCol->GetMetaColumn()->GetName() << ": ";
		strInput = getStringInput();

		try
		{
			if (!pCol->SetValue(strInput))
				std::cout << "Failed to change value!" << std::endl;
		}
		catch (Exception & e)
		{
			e.LogError();
			std::cout << "A D3::Exception occurred, check the log!" << std::endl;
		}
		catch (...)
		{
			std::cout << "An unknown exception occurred." << std::endl;
		}
	}
}






MetaRelationPtr SelectChildMetaRelation(EntityPtr pEntity, const std::string & strTitle, bool bLoad)
{
	MetaRelationPtr						pMR;
	unsigned int							idx;
	int												iChoice;
	std::string								strAction;


	if (bLoad)
		strAction = "  Load And View ";
	else
		strAction = "  View ";


	std::cout << std::endl;

	while (true)
	{
		std::cout << "Current Object:" << std::endl;
		std::cout << "---------------" << std::endl;
		std::cout << "Name:       " << pEntity->GetMetaEntity()->GetFullName() << std::endl;
		std::cout << "PrimaryKey: " << pEntity->GetPrimaryKey()->AsString() << std::endl;
		std::cout << std::endl;

		std::cout << strTitle << std::endl;

		for (idx = 0; idx < strTitle.size(); idx++)
			std::cout << "-";

		std::cout << std::endl;

		// Display child choices
		//
		for (idx = 0; idx < pEntity->GetMetaEntity()->GetChildMetaRelationCount(); idx++)
		{
			pMR = pEntity->GetMetaEntity()->GetChildMetaRelation(idx);

			std::cout << idx + 1 << strAction << pMR->GetReverseName() << "'s " << pMR->GetName() << std::endl;
		}

		std::cout << std::endl;
		std::cout << "0  Exit" << std::endl;
		std::cout << std::endl;
		std::cout << std::endl;

		std::cout << "Select option: ";

		iChoice = getIntegerInput();

		if (iChoice == 0)
			return NULL;

		if (iChoice > 0 && iChoice <= pEntity->GetMetaEntity()->GetChildMetaRelationCount() && pEntity->GetMetaEntity()->GetChildMetaRelationCount() > 0)
		{
			return pEntity->GetMetaEntity()->GetChildMetaRelation(iChoice - 1);
		}
		else
		{
			std::cout << "Invalid option, please try again!" << std::endl;
			std::cout << std::endl;
			std::cout << std::endl;
		}
	}

	return NULL;
}



MetaRelationPtr SelectParentMetaRelation(EntityPtr pEntity, const std::string & strTitle)
{
	MetaRelationPtr						pMR;
	unsigned int							idx;
	int												iChoice;


	std::cout << std::endl;

	while (true)
	{
		std::cout << "Current Object:" << std::endl;
		std::cout << "---------------" << std::endl;
		std::cout << "Name:       " << pEntity->GetMetaEntity()->GetFullName() << std::endl;
		std::cout << "PrimaryKey: " << pEntity->GetPrimaryKey()->AsString() << std::endl;
		std::cout << std::endl;

		std::cout << strTitle << std::endl;

		for (idx = 0; idx < strTitle.size(); idx++)
			std::cout << "-";

		std::cout << std::endl;

		// Display parent choices
		//
		for (idx = 0; idx < pEntity->GetMetaEntity()->GetParentMetaRelationCount(); idx++)
		{
			pMR = pEntity->GetMetaEntity()->GetParentMetaRelation(idx);

			std::cout << idx + 1 << "  View " << pMR->GetName() << "'s " << pMR->GetReverseName() << std::endl;
		}

		std::cout << std::endl;
		std::cout << "0  Exit" << std::endl;
		std::cout << std::endl;
		std::cout << std::endl;

		std::cout << "Select option: ";

		iChoice = getIntegerInput();

		if (iChoice == 0)
			return NULL;

		if (iChoice > 0 && iChoice <= pEntity->GetMetaEntity()->GetParentMetaRelationCount() && pEntity->GetMetaEntity()->GetParentMetaRelationCount() > 0)
		{
			return pEntity->GetMetaEntity()->GetParentMetaRelation(iChoice - 1);
		}
		else
		{
			std::cout << "Invalid option, please try again!" << std::endl;
			std::cout << std::endl;
			std::cout << std::endl;
		}
	}

	return NULL;
}



void ViewAllObjects(DatabaseWorkspace & dbWS, bool bLoad)
{
	DatabasePtr						pDB;
	MetaEntityPtr					pME;
	MetaKeyPtr						pMK;
	InstanceKeyPtrSetItr	itrKey;
	KeyPtr								pKey;
	EntityPtr							pEntity;
	std::string						strInput;
	bool									bDumpRecords = true;
	bool									bDumpAsJSON = false;
	bool									bDumpAsSQL = false;
	bool									bCoutRedirected = false;
	int										iChoice;
	string								strFileName;
	std::streambuf*				pCoutSB;
	std::ofstream					osFile;
	std::string						strError;
	bool									bFirst;



	MenuSelectMetaEntities			menu("Select the entity of which you'd like to view all instances", Menu::SelectOne|Menu::Exit);

	menu.run();

	if (!(pME = menu.getSelected()))
		return;

	try
	{
		pDB = dbWS.GetDatabase(pME->GetMetaDatabase());
		pMK = pME->GetPrimaryMetaKey();

		if (bLoad)
		{
			std::cout << std::endl;
			std::cout << D3Date::Now().AsString() << " - Loading all instances of " << pME->GetName() << "..." << std::endl;
			pME->LoadAll(pDB);
			std::cout << D3Date::Now().AsString() << " - Completed loading all instances of " << pME->GetName() << "!" << std::endl;
		}

		std::cout << std::endl;

		if (pMK->GetInstanceKeySet(pDB)->empty())
		{
			bDumpRecords  = false;
			std::cout << std::endl;
			std::cout << "Found no resident " << pME->GetName() << " records.\n";
			std::cout << std::endl;
		}
		else
		{
			std::cout << "Found " << pMK->GetInstanceKeySet(pDB)->size() << " resident " << pME->GetName() << " records.\n";
		}

		// Ask where to dump the output
		while (bDumpRecords && true)
		{
			std::cout << std::endl;
			std::cout << "Specify output type and location:\n";
			std::cout << "---------------------------------\n";
			std::cout << "1  Dump all " << pME->GetName() << " records as prosa text to screen\n";
			std::cout << "2  Dump all " << pME->GetName() << " records as prosa text to file\n";
			std::cout << "3  Dump all " << pME->GetName() << " records as JSON to screen\n";
			std::cout << "4  Dump all " << pME->GetName() << " records as JSON to file\n";
			std::cout << "5  Dump all " << pME->GetName() << " records as SQL to screen\n";
			std::cout << "6  Dump all " << pME->GetName() << " records as SQL to file\n";
			std::cout << std::endl;
			std::cout << "0  Exit" << std::endl;
			std::cout << std::endl;
			std::cout << std::endl;

			std::cout << "Select option: ";

			iChoice = getIntegerInput();

			if (iChoice == 0)
				return;

			switch (iChoice)
			{
				case 0:
					return;

				case 1:
				case 3:
				case 5:
				{
					// Deal with output to screen
					bDumpAsJSON = iChoice == 3;
					bDumpAsSQL = iChoice == 5;
					break;
				}

				case 2:
				case 4:
				case 6:
				{
					// Deal with output to file
					int							iChoiceFN;
					FILE*						f;

					switch (iChoice)
					{
						case 2:
							strFileName = pME->GetName() + ".txt";
							break;
						case 4:
							strFileName = pME->GetName() + ".json";
							break;
						case 6:
							strFileName = pME->GetName() + ".sql";
							break;
					}

					while (true)
					{
						std::cout << std::endl;

						std::cout << "Specify output file name for the " << pME->GetName() << "\n";
						std::cout << "----------------------------------------------------\n";
						std::cout << "1  Enter/update output file name \"" << strFileName << "\"\n";
						std::cout << std::endl;
						std::cout << "2  Abort\n";
						std::cout << "0  Dump " << pME->GetName() << "\n";
						std::cout << std::endl;

						std::cout << "Select option: ";

						iChoiceFN = getIntegerInput();

						std::cout << std::endl;

						// Valid choice?
						//
						switch (iChoiceFN)
						{
							case 0:
							{
								// Make sure we have a file name
								//
								if (strFileName.empty())
								{
									std::cout << "Please enter filename first!" << std::endl;
									continue;
								}

								f = fopen(strFileName.c_str(), "wb");

								if (!f)
								{
									std::cout << "Failed to create the file, try another name!" << std::endl;
									continue;
								}
								else
								{
									fclose(f);
								}

								break;
							}

							case 1:
							{
								// Get a filename
								//
								std::cout << std::endl;
								std::cout << "Enter export file name: ";
								strFileName = getStringInput();
								continue;
							}

							case 2:
								return;

							default:
							{
								std::cout << "Invalid option, please try again!" << std::endl;
								continue;
							}
						}

						break;
					}

					bDumpAsJSON = iChoice == 4;
					bDumpAsSQL = iChoice == 6;
					break;
				}

				default:
				{
					std::cout << "Invalid option, please try again!" << std::endl;
					std::cout << std::endl;
					std::cout << std::endl;
					continue;
				}
			}

			break;
		}

		if (bDumpRecords)
		{
			// redirect cout
			if (!strFileName.empty())
			{
				osFile.open(strFileName.c_str());
				pCoutSB = cout.rdbuf();
				cout.rdbuf(osFile.rdbuf());
				bCoutRedirected = true;
			}

			if (bDumpAsJSON)
			{
				std::cout << "{\"Entities\":[";

				for ( itrKey =  pMK->GetInstanceKeySet(pDB)->begin(), bFirst = true;
							itrKey != pMK->GetInstanceKeySet(pDB)->end();
							itrKey++)
				{
					pKey = *itrKey;
					pKey->GetEntity()->AsJSON(NULL, std::cout, &bFirst);
				}

				std::cout << "]}\n";
			}
			else if (bDumpAsSQL)
			{
				for ( itrKey =  pMK->GetInstanceKeySet(pDB)->begin();
							itrKey != pMK->GetInstanceKeySet(pDB)->end();
							itrKey++)
				{
					pKey = *itrKey;

					pKey->GetEntity()->AsSQL(std::cout);
					std::cout << std::endl;
				}
			}
			else
			{
				std::cout << "Dumping instances of " << pME->GetName() << ":" << std::endl;
				std::cout << std::endl;

				for ( itrKey =  pMK->GetInstanceKeySet(pDB)->begin();
							itrKey != pMK->GetInstanceKeySet(pDB)->end();
							itrKey++)
				{
					pKey = *itrKey;
					pEntity = pKey->GetEntity();
					pEntity->Dump();
					std::cout << std::endl;
				}

				std::cout << std::endl;
			}
		}
	}
	catch (Exception & e)
	{
		e.LogError();
		strError = "A D3::Exception occurred, check the log!";
	}
	catch (...)
	{
		strError = "An unknown exception occurred.";
	}

	if (bCoutRedirected)
	{
		cout.rdbuf(pCoutSB);
		osFile.close();
	}

	if (!strError.empty())
	{
		std::cout << strError << std::endl;
		std::cout << std::endl;
	}
}



void ViewParentRelations(DatabaseWorkspace & dbWS, EntityPtr pEntity, MetaRelationPtr pMR)
{
	DatabasePtr						pDB;
	RelationPtr						pRelation;
	EntityPtr							pRelEntity;


	try
	{
		pDB = dbWS.GetDatabase(pEntity->GetMetaEntity()->GetMetaDatabase());

		// Determine the relation
		//
		pRelation = pEntity->GetParentRelation(pMR, pDB);

		if (!pRelation)
		{
			std::cout << std::endl;
			std::cout << "Related parent " << pMR->GetReverseName() << " is NULL!" << std::endl;
			return;
		}

		std::cout << std::endl;

		while (true)
		{
			int			iChoice;

			std::cout << "Current Object:" << std::endl;
			std::cout << "---------------" << std::endl;
			std::cout << "Name:       " << pEntity->GetMetaEntity()->GetFullName() << std::endl;
			std::cout << "PrimaryKey: " << pEntity->GetPrimaryKey()->AsString() << std::endl;
			std::cout << std::endl;

			std::cout << "Dumping related " << pMR->GetReverseName() << ":" << std::endl;
			std::cout << std::endl;

			pRelEntity = pRelation->GetParent();
			pRelEntity->Dump();
			std::cout << std::endl;

			std::cout << "Choose one of the following options:" << std::endl;
			std::cout << "------------------------------------" << std::endl;
			std::cout << "1  Inspect this Parent" << std::endl;
			std::cout << std::endl;
			std::cout << "0  Return" << std::endl;
			std::cout << std::endl;
			std::cout << std::endl;

			std::cout << "Select option: ";

			iChoice = getIntegerInput();

			if (iChoice == 0)
				break;

			if (iChoice == 1)
			{
				InspectObject(pRelEntity);
				break;
			}
		}
	}
	catch (Exception & e)
	{
		e.LogError();
		std::cout << "A D3::Exception occurred, check the log!" << std::endl;
	}
	catch (...)
	{
		std::cout << "An unknown exception occurred." << std::endl;
	}
}



void ViewChildRelations(DatabaseWorkspace & dbWS, EntityPtr pEntity, MetaRelationPtr pMR, bool bLoad)
{
	DatabasePtr						pDB;
	RelationPtr						pRelation;
	Relation::iterator		itrRelation;
	EntityPtr							pRelEntity;
	long									idx;


	try
	{
		pDB = dbWS.GetDatabase(pEntity->GetMetaEntity()->GetMetaDatabase());

		if (pMR->GetParentMetaKey()->GetMetaEntity() == pEntity->GetMetaEntity())
		{
			pRelation = pEntity->GetChildRelation(pMR, pDB);

			if (bLoad)
			{
				std::cout << "Loading all related " << pMR->GetName() << "..." << std::endl;
				pRelation->LoadAll();
			}

			if (!pRelation->empty())
			{
				std::cout << std::endl;

				while (true)
				{
					int			iChoice;

					std::cout << "Current Object:" << std::endl;
					std::cout << "---------------" << std::endl;
					std::cout << "Name:       " << pEntity->GetMetaEntity()->GetFullName() << std::endl;
					std::cout << "PrimaryKey: " << pEntity->GetPrimaryKey()->AsString() << std::endl;
					std::cout << std::endl;

					std::cout << "Dumping all related " << pMR->GetName() << ":" << std::endl;
					std::cout << std::endl;

					for ( itrRelation =  pRelation->begin(), idx=0;
								itrRelation != pRelation->end();
								itrRelation++, idx++)
					{
						pRelEntity = *itrRelation;
						std::cout << idx+1 << "    : ";
						pRelEntity->Dump();
						std::cout << std::endl;
					}

					std::cout << "Choose one of the following options:" << std::endl;
					std::cout << "------------------------------------" << std::endl;
					std::cout << "1..." << idx << " Inspect this child" << std::endl;
					std::cout << std::endl;
					std::cout << "0  Return" << std::endl;
					std::cout << std::endl;
					std::cout << std::endl;

					std::cout << "Select option: ";

					iChoice = getIntegerInput();

					if (iChoice == 0)
						break;

					if (iChoice < 1 || iChoice > idx+1)
						continue;

					for ( itrRelation =  pRelation->begin(), idx=0;
								itrRelation != pRelation->end();
								itrRelation++, idx++)
					{
						if (idx+1 == iChoice)
						{
							pRelEntity = *itrRelation;
							std::cout << std::endl;
							InspectObject(pRelEntity);
							break;
						}
					}
				}
			}
			else
			{
				std::cout << "Relation " << pRelation->GetMetaRelation()->GetFullName() << " has no related " << pRelation->GetMetaRelation()->GetChildMetaKey()->GetMetaEntity()->GetName() << " objects resident in memory!" << std::endl;
				std::cout << std::endl;
			}
		}
		else
		{
			std::cout << "Something is screwed here as the entity is not the parent of relation " << pMR->GetName() << ".\n";
			return;
		}
	}
	catch (Exception & e)
	{
		e.LogError();
		std::cout << "A D3::Exception occurred, check the log!" << std::endl;
	}
	catch (...)
	{
		std::cout << "An unknown exception occurred." << std::endl;
	}
}



void AddObject(DatabaseWorkspace & dbWS)
{
	DatabasePtr						pDB;
	MetaEntityPtr					pME;
	EntityPtr							pEntity;


	MenuSelectMetaEntities			menu("Select the entity of which you'd like to create an instances", Menu::SelectOne|Menu::Exit);

	menu.run();

	if (!(pME = menu.getSelected()))
		return;

	try
	{
		pDB = dbWS.GetDatabase(pME->GetMetaDatabase());

		pEntity = pME->CreateInstance(pDB);

		if (EditObject(pEntity))
		{
			pDB->BeginTransaction();

			if (!pEntity->Update(pDB))
			{
				pDB->RollbackTransaction();
				delete pEntity;
				std::cout << "Update failed!" << std::endl;
				return;
			}

			pDB->CommitTransaction();
			std::cout << "Update succeeded!" << std::endl;
		}
		else
		{
			delete pEntity;
		}
	}
	catch (Exception & e)
	{
		if (pDB) pDB->RollbackTransaction();
		e.LogError();
		std::cout << "A D3::Exception occurred, check the log!" << std::endl;
	}
	catch (...)
	{
		if (pDB) pDB->RollbackTransaction();
		std::cout << "An unknown exception occurred." << std::endl;
	}
}



void DeleteObject(DatabaseWorkspace & dbWS)
{
	DatabasePtr						pDB;
	MetaEntityPtr					pME;
	MetaKeyPtr						pPrimaryMK;
	EntityPtr							pEntity;


	MenuSelectMetaEntities			menu("Select the entity from which you'd like to delete an object:", Menu::SelectOne|Menu::Exit);

	menu.run();

	if (!(pME = menu.getSelected()))
		return;

	pPrimaryMK = pME->GetPrimaryMetaKey();

	try
	{
		TemporaryKey		aTempPK(*pPrimaryMK);

		if (EditKey(&aTempPK))
		{
			pDB = dbWS.GetDatabase(pME->GetMetaDatabase());

			pEntity = pPrimaryMK->LoadObject(&aTempPK, pDB);

			if (pEntity)
			{
				pDB->BeginTransaction();

				if (!pEntity->Delete(pDB))
				{
					pDB->RollbackTransaction();
					std::cout << "Delete failed!" << std::endl;
					return;
				}

				pDB->CommitTransaction();
				std::cout << "Delete succeeded!" << std::endl;
			}
			else
			{
				std::cout << "Requested object not found!" << std::endl;
			}
		}
	}
	catch (Exception & e)
	{
		if (pDB) pDB->RollbackTransaction();
		e.LogError();
		std::cout << "A D3::Exception occurred, check the log!" << std::endl;
	}
	catch (...)
	{
		if (pDB) pDB->RollbackTransaction();
		std::cout << "An unknown exception occurred." << std::endl;
	}
}



void UpdateObject(DatabaseWorkspace & dbWS)
{
	DatabasePtr						pDB = NULL;
	MetaEntityPtr					pME;
	MetaKeyPtr						pPrimaryMK;
	EntityPtr							pEntity;


	MenuSelectMetaEntities			menu("Select the entity from which you'd like to update an object:", Menu::SelectOne|Menu::Exit);

	menu.run();

	if (!(pME = menu.getSelected()))
		return;

	pPrimaryMK = pME->GetPrimaryMetaKey();

	try
	{
		TemporaryKey		aTempPK(*pPrimaryMK);

		if (EditKey(&aTempPK))
		{
			pDB = dbWS.GetDatabase(pME->GetMetaDatabase());

			pEntity = pPrimaryMK->LoadObject(&aTempPK, pDB);

			if (pEntity)
			{
				if (EditObject(pEntity))
				{
					pDB->BeginTransaction();

					if (!pEntity->Update(pDB))
					{
						pDB->RollbackTransaction();
						std::cout << "Update failed!" << std::endl;

						return;
					}

					pDB->CommitTransaction();
					std::cout << "Update succeeded!" << std::endl;
				}
			}
			else
			{
				std::cout << "Requested object not found!" << std::endl;
			}
		}
	}
	catch (Exception & e)
	{
		if (pDB) pDB->RollbackTransaction();
		e.LogError();
		std::cout << "A D3::Exception occurred, check the log!" << std::endl;
	}
	catch (...)
	{
		if (pDB) pDB->RollbackTransaction();
		std::cout << "An unknown exception occurred." << std::endl;
	}
}



void InspectObject(EntityPtr pEntity)
{
	MetaRelationPtr				pMR;
	int										iChoice;


	std::cout << std::endl;

	while (true)
	{
		std::cout << "Current Object:" << std::endl;
		std::cout << "---------------" << std::endl;
		std::cout << "Name:       " << pEntity->GetMetaEntity()->GetFullName() << std::endl;
		std::cout << "PrimaryKey: " << pEntity->GetPrimaryKey()->AsString() << std::endl;
		std::cout << std::endl;

		std::cout << "Choose one of the following options:" << std::endl;
		std::cout << "------------------------------------" << std::endl;
		std::cout << "1  View Parents" << std::endl;
		std::cout << "2  View Children" << std::endl;
		std::cout << "3  Load and View Children" << std::endl;
		std::cout << "4  Show brief JSON" << std::endl;
		std::cout << "5  Show verbose JSON" << std::endl;
		std::cout << std::endl;
		std::cout << "0  Exit" << std::endl;
		std::cout << std::endl;
		std::cout << std::endl;

		std::cout << "Select option: ";

		iChoice = getIntegerInput();

		switch (iChoice)
		{
			case 0:
				return;

			case 1:
			{
				while(true)
				{
					pMR = SelectParentMetaRelation(pEntity, "Select the relation to browse");

					if (!pMR)
						break;

					ViewParentRelations(*pEntity->GetDatabase()->GetDatabaseWorkspace(), pEntity, pMR);
				}

				break;
			}

			case 2:
			case 3:
			{
				while(true)
				{
					pMR = SelectChildMetaRelation(pEntity, "Select the relation to browse", iChoice == 3);

					if (!pMR)
						break;

					ViewChildRelations(*pEntity->GetDatabase()->GetDatabaseWorkspace(), pEntity, pMR, iChoice == 3);
				}

				break;
			}

			case 4:
			case 5:
			{
				if (iChoice == 4)
				{
					// brief
					std::cout << "{\"Entity\":";
					pEntity->AsJSON(NULL, std::cout);
					std::cout << "}\n";
				}
				else
				{
					// Full
					std::cout << "{\"MetaEntity\":";
					pEntity->GetMetaEntity()->AsJSON(NULL, std::cout, false);
					std::cout << ",\"Entity\":";
					pEntity->AsJSON(NULL, std::cout);
					std::cout << "}\n";
				}

				break;
			}

			default:
				std::cout << "Invalid option, please try again!" << std::endl;
				std::cout << std::endl;
				std::cout << std::endl;
		}
	}
}




void InspectObject(DatabaseWorkspace & dbWS, bool bLoad)
{
	DatabasePtr						pDB;
	MetaEntityPtr					pME;
	MetaKeyPtr						pPrimaryMK;
	EntityPtr							pEntity = NULL;


	MenuSelectMetaEntities			menu("Select the entity from which you'd like to inspect an object:", Menu::SelectOne|Menu::Exit);

	menu.run();

	if (!(pME = menu.getSelected()))
		return;

	pPrimaryMK = pME->GetPrimaryMetaKey();

	try
	{
		TemporaryKey		aTempPK(*pPrimaryMK);

		if (EditKey(&aTempPK))
		{
			pDB = dbWS.GetDatabase(pME->GetMetaDatabase());

			if (bLoad)
			{
				pEntity = pPrimaryMK->LoadObject(&aTempPK, pDB);
			}
			else
			{
				InstanceKeyPtr	pIK;


				pIK = pPrimaryMK->FindInstanceKey(&aTempPK, pDB);

				if (pIK)
					pEntity = pIK->GetEntity();
			}

			if (pEntity)
			{
				std::cout << std::endl;
				pEntity->Dump();

				InspectObject(pEntity);
			}
			else
			{
				std::cout << "Requested object not found!" << std::endl;
			}
		}
	}
	catch (Exception & e)
	{
		e.LogError();
		std::cout << "A D3::Exception occurred, check the log!" << std::endl;
	}
	catch (...)
	{
		std::cout << "An unknown exception occurred." << std::endl;
	}
}






class	MOPlayMenu : public MenuOption
{
	protected:
		DatabaseWorkspace&		m_ws;

	public:
		MOPlayMenu(const string& strTitle, Menu* pMenu, DatabaseWorkspace& ws)
		: MenuOption(strTitle, pMenu), m_ws(ws)
	{}
};


class MOLoadAndViewAllObjects : public MOPlayMenu
{
	public:
		MOLoadAndViewAllObjects(Menu* pMenu, DatabaseWorkspace& ws)
		: MOPlayMenu("Load and view all objects", pMenu, ws)
		{}

		void		invoke()
		{
			ViewAllObjects(m_ws, true);
		}
};


class MOViewAllObjects : public MOPlayMenu
{
	public:
		MOViewAllObjects(Menu* pMenu, DatabaseWorkspace& ws)
		: MOPlayMenu("View all objects", pMenu, ws)
		{}

		void		invoke()
		{
			ViewAllObjects(m_ws, false);
		}
};


class MOAddAnObject : public MOPlayMenu
{
	public:
		MOAddAnObject(Menu* pMenu, DatabaseWorkspace& ws)
		: MOPlayMenu("Add an object", pMenu, ws)
		{}

		void		invoke()
		{
			AddObject(m_ws);
		}
};


class MODeleteAnObject : public MOPlayMenu
{
	public:
		MODeleteAnObject(Menu* pMenu, DatabaseWorkspace& ws)
		: MOPlayMenu("Delete an object", pMenu, ws)
		{}

		void		invoke()
		{
			DeleteObject(m_ws);
		}
};


class MOUpdateAnObject : public MOPlayMenu
{
	public:
		MOUpdateAnObject(Menu* pMenu, DatabaseWorkspace& ws)
		: MOPlayMenu("Update an object", pMenu, ws)
		{}

		void		invoke()
		{
			UpdateObject(m_ws);
		}
};


class MOLoadAndInspectAnObject : public MOPlayMenu
{
	public:
		MOLoadAndInspectAnObject(Menu* pMenu, DatabaseWorkspace& ws)
		: MOPlayMenu("Load and inspect an object", pMenu, ws)
		{}

		void		invoke()
		{
			InspectObject(m_ws, true);
		}
};


class MOInspectAnObject : public MOPlayMenu
{
	public:
		MOInspectAnObject(Menu* pMenu, DatabaseWorkspace& ws)
		: MOPlayMenu("Inspect an object", pMenu, ws)
		{}

		void		invoke()
		{
			InspectObject(m_ws, false);
		}
};


class MOInvokeQuickTest : public MOPlayMenu
{
	public:
		MOInvokeQuickTest(Menu* pMenu, DatabaseWorkspace& ws)
		: MOPlayMenu("Invoke quick test", pMenu, ws)
		{}

		void		invoke()
		{
			QuickTest(m_ws);
		}
};


MenuPlay::MenuPlay()
{
	addOption(MenuOptionPtr(new MOLoadAndViewAllObjects(this, m_ws)));
	addOption(MenuOptionPtr(new MOViewAllObjects(this, m_ws)));
	addOption(MenuOptionPtr(new MOViewAllObjects(this, m_ws)));
	addOption(MenuOptionPtr(new MOAddAnObject(this, m_ws)));
	addOption(MenuOptionPtr(new MODeleteAnObject(this, m_ws)));
	addOption(MenuOptionPtr(new MOUpdateAnObject(this, m_ws)));
	addOption(MenuOptionPtr(new MOLoadAndInspectAnObject(this, m_ws)));
	addOption(MenuOptionPtr(new MOInspectAnObject(this, m_ws)));
	addOption(MenuOptionPtr(new MOInvokeQuickTest(this, m_ws)));
}
