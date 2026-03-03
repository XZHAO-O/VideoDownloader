#include <QtCore/QCoreApplication>

#include "code_generator.h"

void generateCode()
{
	using namespace nexusdl::database;
	bool result = CodeGenerator::generateFromSql(
		"../VideoDownloader/all.sql",
		CodeGenerator::GenerateAll,
		"../VideoDownloader/",
		true,
		DatabaseId::Download
	);
}

int main(int argc, char* argv[])
{
	//QCoreApplication app(argc, argv);
	//return app.exec();
	generateCode();
}
