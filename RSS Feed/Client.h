#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>

#include <iostream>
#include <memory>

#include <Error.h>

using boost::asio::ip::tcp;
class Client
{
public:
	Client(boost::asio::io_context &io)
		: m_resolver(io), m_io(io)
	{
	}

	void connect(const std::string &server, const std::string &path, std::iostream &content, std::iostream &header)
	{
		m_contents = &content;
		m_headers = &header;

		m_socket.reset(new tcp::socket(m_io));

		//Form the request
		std::ostream request_s(&m_request);
		request_s << "GET " << path << " HTTP/1.0\r\n"
			<< "Host: " << server << "\r\n"
			<< "Accept: */*\r\n"
			//Close connection IMPORTANT
			<< "Connection: close\r\n\r\n";

		//Start an asynchronous resolve for endpoints
		tcp::resolver::query q(server, "http");
		m_resolver.async_resolve(q, boost::bind(&Client::_resolve_, this, boost::asio::placeholders::error, boost::asio::placeholders::iterator));
	}

	void close()
	{
		if (m_socket->is_open())
			m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
	}

	~Client()
	{
		close();
	}

private:
	//Resolve connection
	void _resolve_(const boost::system::error_code &err, tcp::resolver::iterator endIter)
	{
		if (!err)
		{
			//Attempt a connection to each endpoint in the list
			boost::asio::async_connect(*m_socket.get(), endIter,
				boost::bind(&Client::_connect_, this, boost::asio::placeholders::error));
		}
		else
			throw Error(err.message());
	}

	//Establish a connection
	void _connect_(const boost::system::error_code &err)
	{
		if (!err)
		{
			//Send the request
			boost::asio::async_write(*m_socket.get(), m_request,
				boost::bind(&Client::_writeRequest_, this, boost::asio::placeholders::error));
		}
		else
			throw Error(err.message());
	}

	//Request response
	void _writeRequest_(const boost::system::error_code &err)
	{
		if (!err)
		{
			//Read the response status line
			boost::asio::async_read_until(*m_socket.get(), m_response, "\r\n",
				boost::bind(&Client::_readStatusLine_, this, boost::asio::placeholders::error));
		}
		else
			throw Error(err.message());
	}

	//Get status
	void _readStatusLine_(const boost::system::error_code &err)
	{
		if (!err)
		{
			//Get status version and code
			std::istream response_s(&m_response);
			std::string httpVersion;
			unsigned int statusCode;
			response_s >> httpVersion >> statusCode;

			//Get status message
			std::string statusMessage;
			std::getline(response_s, statusMessage);

			//Check if it's OK
			if (!response_s || httpVersion.substr(0, 5) != "HTTP/")
				throw std::string("Invalid response\n");
			if (statusCode != 200)
				throw std::string("Response returned with status code " + std::to_string(statusCode));

			//Read the response headers
			boost::asio::async_read_until(*m_socket.get(), m_response, "\r\n\r\n",
				boost::bind(&Client::_readHeaders_, this, boost::asio::placeholders::error));
		}
		else
			throw Error(err.message());
	}

	//Get headers
	void _readHeaders_(const boost::system::error_code &err)
	{
		if (!err)
		{
			//Put response headers into stream
			std::istream response_s(&m_response);

			//Write headers
			std::string header;
			while (std::getline(response_s, header) && header != "\r")
				*m_headers << header << '\n';

			//Start reading remaining data until EOF
			boost::asio::async_read(*m_socket.get(), m_response, boost::asio::transfer_at_least(1),
				boost::bind(&Client::_readContent_, this, boost::asio::placeholders::error));
		}
		else
			throw Error(err.message());
	}

	//Get content
	void _readContent_(const boost::system::error_code &err)
	{
		if (!err)
		{
			//Get data that was read so far
			*m_contents << &m_response;

			//Continue reading data until EOF
			boost::asio::async_read(*m_socket.get(), m_response, boost::asio::transfer_at_least(1),
				boost::bind(&Client::_readContent_, this, boost::asio::placeholders::error));
		}
		else if (err != boost::asio::error::eof)
			throw Error(err.message());
	}

	boost::asio::io_context &m_io;

	tcp::resolver m_resolver;
	std::unique_ptr<tcp::socket> m_socket;

	boost::asio::streambuf m_request;
	boost::asio::streambuf m_response;

	std::iostream *m_headers;
	std::iostream *m_contents;
};

//--------------------SSL------------------------

class SSL_Client
{
public:
	SSL_Client(boost::asio::io_context& io, boost::asio::ssl::context &ctx, const std::string &host,
		const std::string &path, std::iostream &contents, std::iostream &headers)
		: m_socket(io, ctx), m_resolver(io), m_contents(contents), m_headers(headers)
	{
		//Form the request
		std::ostream request_s(&m_request);
		request_s << "GET " << path << " HTTP/1.1\r\n"
			<< "Host: " << host << "\r\n"
			<< "Accept: */*\r\n"
			//Close connection IMPORTANT
			<< "Connection: close\r\n\r\n";

		//Query and start async chain
		tcp::resolver::query q(host, "https");
		m_resolver.async_resolve(q,
			boost::bind(&SSL_Client::resolve, this, boost::asio::placeholders::error, boost::asio::placeholders::iterator));
	}

	void close()
	{
		m_socket.shutdown();
	}

	~SSL_Client()
	{
		close();
	}

	//Resolve connection
	void resolve(const boost::system::error_code &err, tcp::resolver::iterator endIter)
	{
		if (err)
			throw Error(err.message());

		//Attempt a connection to each endpoint in the list
		boost::asio::async_connect(m_socket.lowest_layer(), endIter,
			boost::bind(&SSL_Client::connect, this, boost::asio::placeholders::error));
	}

	void connect(const boost::system::error_code& err)
	{
		if (err)
			throw Error(err.message());

		//Get Handshake
		m_socket.async_handshake(boost::asio::ssl::stream_base::client,
			boost::bind(&SSL_Client::handshake, this, boost::asio::placeholders::error));
	}

	//Handshake function
	void handshake(const boost::system::error_code& err)
	{
		if (err)
			throw Error(err.message());

		boost::asio::async_write(m_socket, m_request,
			boost::bind(&SSL_Client::write, this, boost::asio::placeholders::error));
	}

	void write(const boost::system::error_code& err)
	{
		if (!err)
		{
			boost::asio::async_read_until(m_socket, m_response, "\r\n",
				boost::bind(&SSL_Client::readStatusLine, this, boost::asio::placeholders::error));
		}
		else
			throw Error(err.message());
	}

	//Get status
	void readStatusLine(const boost::system::error_code &err)
	{
		if (!err)
		{
			//Get status version and code
			std::istream response_s(&m_response);
			std::string httpVersion;
			unsigned int statusCode;
			response_s >> httpVersion >> statusCode;

			//Get status message
			std::string statusMessage;
			std::getline(response_s, statusMessage);

			//Check if it's OK
			if (!response_s || httpVersion.substr(0, 5) != "HTTP/")
				throw Error("Invalid response\n");
			if (statusCode != 200)
				throw Error("Response returned with status code " + std::to_string(statusCode));

			//Read the response headers
			boost::asio::async_read_until(m_socket, m_response, "\r\n\r\n",
				boost::bind(&SSL_Client::readHeaders, this, boost::asio::placeholders::error));
		}
		else
			throw Error(err.message());
	}

	//Get headers
	void readHeaders(const boost::system::error_code &err)
	{
		if (!err)
		{
			//Put response headers into stream
			std::istream response_s(&m_response);

			//Write headers
			std::string header;
			while (std::getline(response_s, header) && header != "\r")
				m_headers << header << '\n';

			//Start reading remaining data until EOF
			boost::asio::async_read(m_socket, m_response, boost::asio::transfer_at_least(1),
				boost::bind(&SSL_Client::readContent, this, boost::asio::placeholders::error));
		}
		else
			throw Error(err.message());
	}

	//Get content
	void readContent(const boost::system::error_code &err)
	{
		if (!err)
		{
			//Get data that was read so far
			m_contents << &m_response;

			//Continue reading data until EOF
			boost::asio::async_read(m_socket, m_response, boost::asio::transfer_at_least(1),
				boost::bind(&SSL_Client::readContent, this, boost::asio::placeholders::error));
		}
		else if (err != boost::asio::error::eof)
			throw Error(err.message());
	}

private:
	boost::asio::io_context &m_io;

	tcp::resolver m_resolver;
	std::unique_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> m_socket;

	boost::asio::streambuf m_request;
	boost::asio::streambuf m_response;

	std::iostream *m_headers;
	std::iostream *m_contents;
};