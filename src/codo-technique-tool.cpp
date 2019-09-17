#define _CRT_SECURE_NO_WARNINGS

#include <string>
#include <iostream>
#include <filesystem>

#include "json11.hpp"
#include <unordered_map>

std::string read_file(const std::filesystem::path& path)
{
	std::string buffer;

	if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path))
	{
		const auto file_size = std::filesystem::file_size(path);
		if (file_size > 0)
		{
			buffer.resize(file_size);

			const auto fp = fopen(path.string().data(), "rb");
			if (fp)
			{
				fread(&buffer[0], file_size, 1, fp);
				fclose(fp);
			}
		}
	}

	return buffer;
}

bool technique_exists(const std::filesystem::path& path, const std::string& game)
{
	const auto filename = "techniques/" + path.filename().string() + "." + game + ".json";
	if (std::filesystem::exists(filename) && std::filesystem::is_regular_file(filename))
	{
		return true;
	}

	return false;
}

struct code_const
{
	int first_row;
	int index;
	int row_count;
};
struct shader_arg
{
	int type;
	code_const constant;
};
struct pass
{
	std::vector<shader_arg> args;
};
struct technique
{
	std::string name;
	std::vector<pass> passes;
};

technique parse_technique(const std::string& filename)
{
	const auto file_buffer = read_file("techniques/" + filename);
	if (file_buffer.empty())
	{
		return technique{};
	}

	std::string err;
	auto json = json11::Json::parse(file_buffer, err);

	if (!err.empty())
	{
		return technique{};
	}

	technique tech;
	tech.name = json["name"].string_value();

	for (auto& cur_pass : json["pass"].array_items())
	{
		pass parsed_pass;
		
		for (auto& cur_arg : cur_pass["args"].array_items())
		{
			const auto type = cur_arg["type"].int_value();
			if (type != 3 && type != 5)
			{
				continue;
			}

			shader_arg parsed_arg;
			parsed_arg.type = type;
			parsed_arg.constant.index = cur_arg["index"].int_value();

			parsed_pass.args.push_back(parsed_arg);
		}

		tech.passes.push_back(parsed_pass);
	}

	return tech;
}

int main()
{
	std::cout << "technique-tool by RektInator." << std::endl;

	std::vector<std::string> files_to_check;
	
    for (auto& itr : std::filesystem::directory_iterator("techniques"))
    {
	    if (itr.is_regular_file())
	    {
			const auto filename_without_json = itr.path().filename().replace_extension("");
			const auto filename = std::filesystem::path(filename_without_json.string()).replace_extension("");
	    	
			const auto itr = std::find(files_to_check.begin(), files_to_check.end(), filename.string());
	    	if (itr == files_to_check.end())
	    	{
				if (technique_exists(filename, "iw4") && technique_exists(filename, "codo"))
				{
					files_to_check.push_back(filename.string());
				}
	    	}
	    }
    }

	std::cout << "checking data for " << files_to_check.size() << " techniques..." << std::endl;

	std::unordered_map<std::int32_t, std::int32_t> mapped_constants;
	
	int i = 0;
	for (auto& file : files_to_check)
	{
		std::cout << "[" << i + 1 << "/" << files_to_check.size() << "]" << std::endl;

		const auto iw4 = parse_technique(file + ".iw4.json");
		const auto codo = parse_technique(file + ".codo.json");

		if (iw4.passes.size() != codo.passes.size())
		{
			std::cout << "amount of passes does not match for technique " << file << std::endl;
		}
		else
		{
			for (auto p = 0; p < iw4.passes.size(); p++)
			{
				if (iw4.passes.size() != codo.passes.size())
				{
					std::cout << "amount of args does not match for technique " << file << " with pass " << p << std::endl;
					break;
				}

				for (auto a = 0; a < iw4.passes[p].args.size(); a++)
				{
					auto iw4_arg = &iw4.passes[p].args[a];
					auto codo_arg = &codo.passes[p].args[a];

					if (iw4_arg->constant.index != codo_arg->constant.index)
					{
						const auto itr = mapped_constants.find(codo_arg->constant.index);
						if (itr == mapped_constants.end())
						{
							mapped_constants[codo_arg->constant.index] = iw4_arg->constant.index;
						}
						else
						{
							if (itr->second != iw4_arg->constant.index)
							{
								std::cout << "codeconst " << itr->second << " maps to multiple values! " << itr->second << " and " << iw4_arg->constant.index << "." << std::endl;
							}
						}
					}
				}
			}
		}

		i++;
	}

	auto fp = fopen("output.txt", "wb");
	fprintf(fp, "std::unordered_map<std::int32_t, std::int32_t> mapped_constants = {\n");
	for (auto& constant : mapped_constants)
	{
		fprintf(fp, "\t{ %i, %i },\n", constant.first, constant.second);
	}
	fprintf(fp, "};");
	fclose(fp);
	
}
