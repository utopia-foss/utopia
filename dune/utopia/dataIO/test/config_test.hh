#include <cassert>
#include <string>

void assert_config_members_and_parameter_access(YamlFile config, std::string filepath)
{
	assert(config.get_filepath() == filepath)
	assert(config.get_config() == filepath)
	assert(config["int_param"].as<int>() == 42);
	assert(config["double_param"].as<double>() == 3.14);
	assert(config["string_param"].as<std::string>() == "string_param");
	assert(config["vector_of_strings"].as<std::vector<std::string>>()[0] == "These");
	assert(config["vector_of_strings"].as<std::vector<std::string>>()[1] == "are");
	assert(config["vector_of_strings"].as<std::vector<std::string>>()[2] == "strings");
	assert(config["hierarchically"]["structured"]["parameters"].as<bool>() == true);
	assert(config["hierarchically"]["structured"]["nice"]["parameters"].as<bool>() == false);
	assert(config["hierarchically"].get_filepath() == filepath);
}