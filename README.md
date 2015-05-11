Building and running D3
=======================

What you need:

Download/Install from Microsoft:
- Visual C++ 2010 Express
- Windows Platform SDK 7.1
- SQL Server 2008 R2 Express
- SQL Server Management Studio 

Download from boost
- Boost libraries 1.57 (you can get a set that contains the ready built libraries
  for many Visual Studio versions, its a big download but it it saves a lot of time building the libs)
	
Download all the stuff you find here:
- https://www.dropbox.com/sh/p5zxg72fexnyxv6/AACCg9DaOIkqQwZPvcH3kEU0a?dl=0


To make things hopefully easy for you, follow these steps:

1. unpack Libs-Dmitriy.7z from the drop box to c:\Development (you can put it elsewhere but you then
   need to replace C:\Development where ever it is referenced below with what you were actually using)
2. extract boost 1.57 into C:\Development
3. extract D3 somewhere, it contains:
   - D3 (the Visual C++ 2010 Express solution directory)
	 - Docs (some documentation)
	 - Tools (a password generator you will need)
	 - schooldb.png image of the sample custom database I included the definitions for
4. Check the folder structure of C:\Development\Libs-Dmitriy, it should contain these folders:
   - boost_1_57_0
	 - CenterPoint XML DOM
	 - jsoncpp
	 - libodbc++
	 - wjelement++
5. wherever you extracted D3 in step 3, open the D3\D3\D3.sln file with Visual C++ 2010 Express (I recommend
   you set tab-stop to 2 everywhere to see the code formatted as intended)
6. the first step is to ensure the include and library files can be found, so open the property manager
   (View|Property Manager) and look for:
   * Debug | Win32
     * Microsoft.Cpp.Win32.user
	 and now change these two settings:
	 - Include Directories => C:\Development\Libs-Dimitry\boost_1_57_0;C:\Development\Libs-Dimitry\CenterPoint XML DOM\XML-02_01_07\include;C:\Development\Libs-Dimitry\curl-7.34.0\include;C:\Development\Libs-Dimitry\CxImage-7.02\CxImage;C:\Development\Libs-Dimitry\Ice-3.3.0-VC90\include;C:\Development\Libs-Dimitry\jsoncpp\include;C:\Development\Libs-Dimitry\libharu-2.0.8\include;C:\Development\Libs-Dimitry\libodbc++\include;C:\Development\Libs-Dimitry\openssl-1.0.1j\inc32\;C:\Development\Libs-Dimitry\OTL_V4\include;C:\Development\Libs-Dimitry\wjelement++\wjelement\include;C:\Development\Libs-Dimitry\wjelement++\wjelement-cpp\include;C:\Development\Libs-Dimitry\zlib-1.2.8\include;C:\Oracle\Win32\instantclient_11_2\sdk\include;$(IncludePath)
   - Library Directories => $(VSInstallDir)\lib;C:\Development\Libs-Dimitry\boost_1_57_0\lib32-msvc-10.0;C:\Development\Libs-Dimitry\CenterPoint XML DOM\XML-02_01_07\Win32\Debug-shared-vc90;C:\Development\Libs-Dimitry\CenterPoint XML DOM\XML-02_01_07\Win32\Debug-static-vc90;C:\Development\Libs-Dimitry\CenterPoint XML DOM\XML-02_01_07\Win32\Release-shared-vc90;C:\Development\Libs-Dimitry\CenterPoint XML DOM\XML-02_01_07\Win32\Release-static-vc90;C:\Development\Libs-Dimitry\curl-7.34.0\winbuild\libcurl\vc90_Win32_Release;C:\Development\Libs-Dimitry\curl-7.34.0\winbuild\libcurl\vc90_Win32_Release_DLL;C:\Development\Libs-Dimitry\curl-7.34.0\winbuild\libcurl\vc90_Win32_Debug;C:\Development\Libs-Dimitry\curl-7.34.0\winbuild\libcurl\vc90_Win32_Debug_DLL;C:\Development\Libs-Dimitry\CxImage-7.02\CxImage\build\Debug_Win32_vc90;C:\Development\Libs-Dimitry\CxImage-7.02\CxImage\build\Release_Win32_vc90;C:\Development\Libs-Dimitry\CxImage-7.02\CxImage\CxImageDLL\build\Debug_Win32_vc90;C:\Development\Libs-Dimitry\CxImage-7.02\CxImage\CxImageDLL\build\Release_Win32_vc90;C:\Development\Libs-Dimitry\CxImage-7.02\jasper\build\Debug_Win32_vc90;C:\Development\Libs-Dimitry\CxImage-7.02\jasper\build\Release_Win32_vc90;C:\Development\Libs-Dimitry\CxImage-7.02\jbig\build\Debug_Win32_vc90;C:\Development\Libs-Dimitry\CxImage-7.02\jbig\build\Release_Win32_vc90;C:\Development\Libs-Dimitry\CxImage-7.02\jpeg\build\Debug_Win32_vc90;C:\Development\Libs-Dimitry\CxImage-7.02\jpeg\build\Release_Win32_vc90;C:\Development\Libs-Dimitry\CxImage-7.02\libpsd\build\Debug_Win32_vc90;C:\Development\Libs-Dimitry\CxImage-7.02\libpsd\build\Release_Win32_vc90;C:\Development\Libs-Dimitry\CxImage-7.02\mng\build\Debug_Win32_vc90;C:\Development\Libs-Dimitry\CxImage-7.02\mng\build\Release_Win32_vc90;C:\Development\Libs-Dimitry\CxImage-7.02\png\build\Debug_Win32_vc90;C:\Development\Libs-Dimitry\CxImage-7.02\png\build\Release_Win32_vc90;C:\Development\Libs-Dimitry\CxImage-7.02\raw\build\Debug_Win32_vc90;C:\Development\Libs-Dimitry\CxImage-7.02\raw\build\Release_Win32_vc90;C:\Development\Libs-Dimitry\CxImage-7.02\tiff\build\Debug_Win32_vc90;C:\Development\Libs-Dimitry\CxImage-7.02\tiff\build\Release_Win32_vc90;C:\Development\Libs-Dimitry\Ice-3.3.0-VC90\lib;C:\Development\Libs-Dimitry\jsoncpp\build\vc90\debug;C:\Development\Libs-Dimitry\jsoncpp\build\vc90\release;C:\Development\Libs-Dimitry\libharu-2.0.8\build\VC90\Debug\Win32;C:\Development\Libs-Dimitry\libharu-2.0.8\build\VC90\Release\Win32;C:\Development\Libs-Dimitry\libodbc++\Debug-vc90-mt-gd\Win32;C:\Development\Libs-Dimitry\libodbc++\Release-vc90-mt-s\Win32;C:\Development\Libs-Dimitry\libssh2-1.4.3\win32\Win32\DLL Debug;C:\Development\Libs-Dimitry\libssh2-1.4.3\win32\Win32\DLL Release;C:\Development\Libs-Dimitry\libssh2-1.4.3\win32\Win32\LIB Debug;C:\Development\Libs-Dimitry\libssh2-1.4.3\win32\Win32\LIB Release;C:\Development\Libs-Dimitry\openssl-1.0.1j\out32;C:\Development\Libs-Dimitry\openssl-1.0.1j\out32.dbg;C:\Development\Libs-Dimitry\wjelement++\wjelement\windows\Win32\Debug;C:\Development\Libs-Dimitry\wjelement++\wjelement\windows\Win32\Release;C:\Development\Libs-Dimitry\wjelement++\wjelement-cpp\windows\Win32\Debug;C:\Development\Libs-Dimitry\wjelement++\wjelement-cpp\windows\Win32\Release;C:\Development\Libs-Dimitry\zlib-1.2.8\win32\Win32\Release;C:\Development\Libs-Dimitry\zlib-1.2.8\win32\Win32\Debug;C:\Oracle\Win32\instantclient_11_2\sdk\lib\msvc;C:\Oracle\Win32\instantclient_11_2\sdk\lib;$(LibraryPath)
7. open the file D3\Test\dbdefs.h from the Test project, you will need to modify this to match details
   on your machine. The file contains the details for 3 databases:
	 
	 - D3MDDB: D3 Meta Dictionary database
	 - D3HSDB: Help database
	 - SCHOOLDB: A sample DB
	 
	 The first 4 entries for each DB are most likely identical between definitions.
	 
			// Goto "Administrative tools/Data Sources (ODBC) and open the drivers tab and choose
			// one for SQL Server
 			m_strDriver					= "SQL Server Native Client 10.0";
				
			// Depending on what your SQLServer instance name is adjust the following entry
			// (default is [host]\SQLExpress, but [host] can be set to . indicating localhost)
			m_strServer					= ".\\SQLExpress";
				
			// This is an SQL Server which you should create. sa is the default system admin
			// SQL Server account and has sufficient privileges, but you could create another
			// account instead but you might get errors until you have permissions right
			m_strUserID					= "sa";
				
			// WARNING READ THIS!!! The password here is encoded. In the stuff you downloaded from
			// dropbox you'll find a tools directory which includes APAL_Password.exe. If you plain text
			// password is say MoNG0dB you run it like so from the command line:
			//   APAL_Password -p NoMoNG0dB
			// this yields: 
			//   plain......: NoMoNG0dB
      //   encrypted..: akoiPWpqe1Vw
			// You use the encrypted string here
			m_strUserPWD				= "akoiPWpqe1Vw";
			
		Everywhere in this file, change the sting D:\\Data\\Databases\\SCHOOL to a folder that exists
    on a drive with enough space to create the databases
		
		You can also change the time zone string from New Zealand to Ukraine (this doesn't include
		daylight saving details but is OK):
		m_strTimeZone				= "EET+2";

		Leave all else as is please!!!
8. Run the SQL Server configuration manager and make sure this is set:
   . SQL Server Service and SQL Server Browser are both running
	 . look for the 32 bit driver configuration you used above under m_strDriver:
     . client protocol TCP/IP must be enabled
9. Back in Visual C++, select the target you build as Debug | Win32 and build everything

Fix any issues you might have or ask me for help if you can't work out what is wrong.

Once all is built, do this:

Run D3/Test/Debug/Test.exe  and do this:

2  Create Meta Dictionary Database
4  Import Meta Dictionary
6  Load Meta Dictionary

All going well, at this point you should see a database in SQL Server Management Studio called 
SCHOOL_D3MDDB_6_07_0004 which contains the schema for the other databases.

On the next screen do:

1  Create Custom Database(s)

leave both db's selected and choose:

0  Create selected databases

If all this works you're off to a great start.

I've added an image of the SCHOOLDB (see Docs\school.png). You could now use:

4  Play with Custom Database(s)

and add new records to the school database.

A quick word about D3::DatabaseWorkspace objects: D3 maintains database workspace objects. If you want to use a database, 
you need a work space. Everything you load from a database or write to a database gets stored in the database workspace.
These objects remain loaded until you throw away a database workspace.
If you choose "4  Play with Custom Database(s)" a database workspace is created and all subsequent actions work on that 
database work space. When you exit the play menu, the database workspace is destroyed. Therefore, menu options in the
play menu with "Load" actually add load stuff from the database while all others work with data in the workspace.
In a sense, the database workspace is your personal; cache to data loaded from the DB.

Please note that not everything in the Test app works, I have ported as much of the stuff that is needed as
I could in the time given, but there are still a few items missing.

Next steps:

- have a quick look at the meta dictionary (see Docs\D3Brief\D3.html)
- doxygen documentation for D3 is in Docs\D3Doxy\index.html
- the D3 dll is the critical bit in the puzzle. I'd love to see it used from JavaScript, and a good starting point would be to:
  . expose D3::Settings as a JavaScript object (allows D3 to be configured)
	. expose all meta objects as JavaScript objects:
    D3::MetaDatabase
		D3::MetaEntity
		D3::MetaColumn
		D3::MetaColumnChoice
		D3::MetaKey
		D3::MetaKeyColumn
		D3::MetaRelation

		I think once we get that far I can add JS interfaces to other classes.
		
So let's discuss the next steps in TeamViewer :)
