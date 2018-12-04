#include <iostream>
#include <string>
#include <vector>

#include "boost/asio.hpp"

#include <CustomLibrary/CustomSDL/Engine.h>
#include <CustomLibrary/CustomSDL/Text.h>
#include <CustomLibrary/Client.h>

boost::asio::io_context g_io;
Client g_client(g_io);

#include "Parse.h"
#include "WebImage.h"
#include "Item.h"

class RSS : public C_SDL
{
public:
	RSS()
		: C_SDL({ 1280, 960 }, "RSS Feed", 30, false),
		m_title(this),
		m_description(this),
		m_link(this),
		m_image(this)
	{
		g_client.connect("feeds.bbci.co.uk", "/news/world/rss.xml", m_content, m_header);

		//Load
		m_title.loadFont("ABeeZee-Regular.otf", 30);
		m_description.loadFont("ABeeZee-Regular.otf", 15);
		m_link.loadFont("ABeeZee-Regular.otf", 15);
		g_io.run();

		//Parse content
		for (Parse buf; m_content >> buf;)
			m_source.push_back(std::move(buf.m_dat));
		m_source.shrink_to_fit();

		g_client.close();
		g_io.reset();
		m_header.str("");

		//Clean front and end
		m_source.erase(m_source.begin(), std::find(m_source.begin(), m_source.end(), "<channel>") + 1);
		m_source.erase(std::find(m_source.begin(), m_source.end(), "</channel>"), m_source.end());

		//Load required channel elements and load items
		for (auto iter = m_source.begin(); iter != m_source.end(); ++iter)
			if (*iter == "<item>")
				m_items.push_back(Item(++iter, this));

			else if (*iter == "<title>")
				m_title.loadBlended(filter(*++iter), { 0, 0, 0, 0xFF });

			else if (*iter == "<description>")
				m_description.loadBlended(filter(*++iter), { 0, 0, 0, 0xFF });

			else if (*iter == "<link>")
				m_link.loadBlended(*++iter, { 0, 0, 0, 0xFF });

			else if (*iter == "<image>")
				m_image.load(++iter);
		m_items.shrink_to_fit();
	}

private:
	void handleEvent(const SDL_Event &e) override
	{
		if (e.type == SDL_MOUSEWHEEL)
				m_camera[1] -= e.wheel.y * 20;
	}

	void renderOrder() override
	{
		m_title.render({ (m_dim[0] - m_title.getDim()[0]) / 2, 0 });
		m_description.render({ 0, 30 });
		m_link.render({ 0, 60 });
		m_image.render(90);

		int offset = 150 + m_image.getTexture().getDim()[1];

		for (auto& i : m_items)
			i.render(offset);
	}

	std::vector<std::string> m_source;

	Text m_title;
	Text m_description;
	Text m_link;
	Image m_image;

	std::vector<Item> m_items;

	std::stringstream m_header;
	std::stringstream m_content;
};

int main(int argc, char* argv[])
{
	try
	{
		RSS rss;
		rss.run();
	}
	catch (const std::exception& e)
	{
		std::cout << "Exception: " << e.what() << "\n";
		getchar();
	}

	return 0;
}