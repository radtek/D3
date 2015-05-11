#include "types.h"
#include "MenuMain.h"

using namespace std;
using namespace D3;



int main(int argc, char** argv)
{
	ExceptionContext		gEC("Test.log", Exception_diagnostic);
	Settings&						d3Settings = Settings::Singleton();
	MenuMain						menuMain;

	menuMain.run();

	return 0;
}