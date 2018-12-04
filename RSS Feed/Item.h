#pragma once

#include <CustomLibrary/CustomSDL/Engine.h>
#include <CustomLibrary/CustomSDL/Texture.h>

std::string filter(const std::string &x)
{
	return std::string(std::find(x.rbegin(), x.rend(), '[').base(), std::find(x.begin(), x.end(), ']'));
}

class Item : public Object
{
public:
	Item(std::vector<std::string>::iterator &iter, C_SDL *engine)
		: Object(engine),
		m_image(engine)
	{
		//Set Engine
		for (auto &i : m_content)
			i.setEngine(engine);

		//Load fonts
		m_content[TITLE].loadFont("ABeeZee-Regular.otf", 20);
		m_content[DESCRIPTION].loadFont("ABeeZee-Regular.otf", 15);
		m_content[LINK].loadFont("ABeeZee-Regular.otf", 15);
		m_content[PUB_DATE].loadFont("ABeeZee-Regular.otf", 15);

		for (; *iter != "</item>"; ++iter)
			if (*iter == "<title>")
				m_content[TITLE].loadFromText(filter(*++iter), { 0, 0, 0, 0xFF }, Text::Type::BLENDED);

			else if (*iter == "<description>")
				m_content[DESCRIPTION].loadFromText(filter(*++iter), { 0, 0, 0, 0xFF }, Text::Type::BLENDED);

			else if (*iter == "<link>")
				m_content[LINK].loadFromText(*++iter, { 0, 0, 0, 0xFF }, Text::Type::BLENDED);

			else if (*iter == "<pubDate>")
				m_content[PUB_DATE].loadFromText(*++iter, { 0, 0, 0, 0xFF }, Text::Type::BLENDED);

			else if ((*iter)[1] == 'm')
			{
				auto last = std::find(iter->rbegin(), iter->rend(), '\"');
				std::string url(std::find(last + 1, iter->rend(), '\"').base(), (last + 1).base());

				url.erase(url.begin(), url.begin() + 7);

				std::string path(url.substr(url.find('/')));
				url.erase(url.find('/'));

				m_image.loadFromWeb(url, path);
			}
	}

	void render(int &offset)
	{
		if (m_engine->getCamera().y + m_engine->getCamera().h > offset && m_engine->getCamera().y < 635 + offset)
		{
			for (unsigned char i = 0; i < m_content.size(); offset += m_content[i].getDim().y + 5, ++i)
				m_content[i].render({ 0, offset });

			m_image.render({ 0, offset });
			offset += m_image.getTexture().getDim().y + 50;
		}
		else
			offset += 635;
	}

private:
	enum
	{
		TITLE,
		DESCRIPTION,
		LINK,
		PUB_DATE
	};
	std::array<Text, 4> m_content;
	WebImage m_image;
};