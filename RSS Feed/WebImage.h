#pragma once

#include <CustomLibrary/CustomSDL/Engine.h>
#include <CustomLibrary/CustomSDL/Texture.h>
#include <sstream>

class WebImage : public Object
{
public:
	WebImage(C_SDL *engine)
		: Object(engine),
		m_image(engine) {}

	void loadFromWeb(const std::string &host, const std::string &path)
	{
		std::stringstream header;
		std::stringstream content;

		try
		{
			g_client.connect(host, path, content, header);
			g_io.run();
		}
		catch (const std::exception &e)
		{
			std::cerr << "Exeption: " << host << path << " " << e.what() << std::endl;
			g_client.close();
			g_io.reset();
			return;
		}
		g_client.close();
		g_io.reset();

		m_image.loadFromStream(content, &FORMAT_Y);
	}

	void render(const Vector2D &xy)
	{
		m_image.render(xy);
	}

	constexpr const Texture& getTexture() const { return m_image; }

private:
	static constexpr int FORMAT_Y = 500;

	Texture m_image;
};

class Image : public Object
{
public:
	Image(C_SDL *engine)
		: Object(engine),
		m_image(engine) {}

	void load(std::vector<std::string>::iterator &iter)
	{
		for (; *iter != "</image>"; iter++)
			if (*iter == "<url>")
				m_image.loadFromFile("news_logo.gif");
	}

	void render(const int &offset)
	{
		m_image.render({ 0, offset });
	}

	constexpr const Texture& getTexture() const { return m_image; }

private:
	Texture m_image;
};