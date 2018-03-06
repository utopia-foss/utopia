#include <cassert>
#include <string>

template<typename ConfigClass>
void assert_config_members_and_parameter_access(ConfigClass& config, std::string filepath)
{
	assert(config.get_filepath() == filepath);

	assert(config["int_param"].template as<int>() == 42);
	assert(config["double_param"].template as<double>() == 3.14);
	assert(config["string_param"].template as<std::string>() == "string_param");
	assert(config["vector_of_strings"].template as<std::vector<std::string>>()[0] == "These");
	assert(config["vector_of_strings"].template as<std::vector<std::string>>()[1] == "are");
	assert(config["vector_of_strings"].template as<std::vector<std::string>>()[2] == "strings");

	assert(config["hierarchically"]["structured"]["parameters"].template as<bool>() == true);
	assert(config["hierarchically"]["structured"]["nice"]["parameters"].template as<bool>() == false);
	assert(config["hierarchically"].get_filepath() == filepath);
}