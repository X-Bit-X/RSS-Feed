#include <iostream>
#include <fstream>

#include <Error.h>
#include <Client.h>

int main(int argc, char* argv[])
{
	try
	{
		if (argc != 3)
			throw Error("Usage: <host> <path>");

		boost::asio::io_context io;
		boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
		ctx.set_default_verify_paths();

		std::stringstream content;
		std::stringstream header;

		SSL_Client c(io, ctx, argv[1], argv[2], content, header);

		io.run();

		std::fstream fileOut("testFile.xml", std::fstream::out | std::fstream::binary);
		fileOut << content.str();
		fileOut.close();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << "\n";
	}

	std::cout << "Done.";
	getchar();
	return 0;
}