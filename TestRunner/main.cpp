#include <QtCore/QCoreApplication>

#include "CodeGenerator.h"

int main(int argc, char* argv[])
{
	//QCoreApplication app(argc, argv);

	bool result = CodeGenerator::generateFromSql(
		"../VideoDownloader/all.sql",
		CodeGenerator::GenerateAll,
		"../VideoDownloader/",
		true
	);

	//return app.exec();
}
