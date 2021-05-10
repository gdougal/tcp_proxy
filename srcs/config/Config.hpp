//
// Created by Gilberto Dougal on 4/26/21.
//

#ifndef HELLO_SFML_CONFIG_HPP
#define HELLO_SFML_CONFIG_HPP

#include <unordered_map>
#include <fstream>
#include <istream>
#include <exception>
#include <arpa/inet.h>


class Config
{
public:
	class Section;
	explicit	Config(const std::string& config_name);

	const Section			&getSection(const std::string& section_name)	const;

	class Section {
		friend Config;
		typedef std::unordered_map<std::string, std::string>	t_section;

		t_section			content;
		void setContent(const std::string& key, const std::string& value);
		const std::string& throw_or_not(const std::string& Key) const;

	public:
		Section() {}
		const std::string	getStringVal(const std::string& Key)	const;
		float				getFloatVal(const std::string& Key)		const;
		int					getIntVal(const std::string& Key)			const;
		bool				getBoolVal(const std::string& Key)		const;
		uint32_t		getAddrVal(const std::string& Key)		const;
		uint16_t		getPortVal(const std::string& Key)		const;
	};

private:
	std::unordered_map<std::string, Section> sections;
	void create_config(std::ifstream & ifs);
};



#endif //HELLO_SFML_CONFIG_HPP
