#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <regex>
static std::vector<std::regex> vec_user_regexes{100};
static std::vector<std::string> vec_excluded_files{ 100 };
static std::vector<std::string> vec_edited_files{ 100 };
static std::vector<std::regex> vec_line_regexes{ 100 };

static bool is_forbidden_path(const std::filesystem::path& path)
{
	if (vec_user_regexes.empty())
		return false;

	const auto rgx_it = std::find_if(std::begin(vec_user_regexes), std::end(vec_user_regexes), [dir = path.string().c_str()](const std::regex& rgx)
		{
			std::cmatch m;
			if (std::regex_match(dir, m, rgx))
			{
				vec_excluded_files.push_back(dir);
				return true;
			}
			return false;
		}
	);

	if (rgx_it == std::end(vec_user_regexes))
		return false;

	return true;
}

static bool is_forbidden_line(const std::string& line)
{
	if (vec_line_regexes.empty())
		return false;

	const auto it = std::find_if(std::begin(vec_line_regexes), std::end(vec_line_regexes), [a_line = line.c_str()](const std::regex& rgx)
		{
			std::cmatch m;
			return std::regex_match(a_line, m, rgx);
		}
	);

	if (it == std::end(vec_line_regexes))
		return false;

	return true;
}
static void remove_comments(const std::string& file_path) {
	std::ifstream input_file(file_path);
	std::string line;
	std::string output;

	bool in_block_comment = false;
	bool in_line_comment = false;

	while (std::getline(input_file, line)) {
		
		const bool is_excluded = is_forbidden_line(line);
		
		for (size_t i = 0; i < line.size(); i++) {
			// Check for start of block comment
			if (!is_excluded && line[i] == '/' && line[i + 1] == '*') {
				in_block_comment = true;
				i++; // Skip the next character
				continue;
			}

			// Check for end of block comment
			if (!is_excluded && line[i] == '*' && line[i + 1] == '/') {
				in_block_comment = false;
				i++; // Skip the next character
				continue;
			}

			// Check for start of line comment
			if (!is_excluded && line[i] == '/' && line[i + 1] == '/') {
				in_line_comment = true;
				break; // Skip the rest of the line
			}

			// Add the character to the output if we are not in a comment
			if (!in_block_comment && !in_line_comment) {
				output += line[i];
			}
		}

		// Reset the line comment flag
		in_line_comment = false;

		// Add a newline character after each line
		output += '\n';
	}
	vec_edited_files.push_back(file_path);
	// Write the output to the file
	std::ofstream output_file(file_path);
	output_file << output;
}

static void scan_directory(const std::string& directory_path) {
	for (const auto& entry : std::filesystem::recursive_directory_iterator(directory_path)) {
		auto extension = entry.path().extension();
		if ((extension == ".cpp" || extension == ".h") && !is_forbidden_path(entry)) {
			remove_comments(entry.path().string());
		}
	}
}

template <class T>
static void extract_from_file(const std::filesystem::path& file_filter_path, std::vector<T>* vec)
{
	if (!std::filesystem::exists(file_filter_path))
		return;
	auto str_file = file_filter_path.string();

	std::ifstream input_file(str_file);
	std::string line;
	if (!input_file.is_open() || !input_file.good())
		return;

	while (std::getline(input_file, line))
	{
		if (!line.empty())
			vec->emplace_back(line);
	}
}

static void extract_all_filters(const std::filesystem::path& file_filter_path)
{
	extract_from_file(file_filter_path, &vec_user_regexes);
}

static void extract_all_file_line_regexes(const std::filesystem::path& file_filter_path)
{
	extract_from_file(file_filter_path, &vec_line_regexes);
}

int main() 
{
	auto currPath = std::filesystem::current_path();
	std::string filter_path_str = currPath.string() + "\\filter_file.txt";
	std::string line_regex_path_str = currPath.string() + "\\line_regexes.txt";

	std::cout << "config files:\n" << filter_path_str << "\n" << line_regex_path_str << "\n";
	std::cout << "are you sure you want to remove all comments? y/n" << "\n";
	std::string answer{};
	std::cin >> answer;

	if (answer.find("y") == std::string::npos)
		return 0;

	std::string directory_path;
	std::cout << "enter directory for recursive cleanup : ";
	std::cin >> directory_path;
	std::cout << "\n";

	auto filter_path = std::filesystem::path(filter_path_str);
	auto line_regex_path = std::filesystem::path(line_regex_path_str);
	extract_all_filters(filter_path);
	extract_all_file_line_regexes(line_regex_path);
	if(std::filesystem::exists(directory_path))
		scan_directory(directory_path);
	std::cout << "all excluded files:\n";

	std::for_each(vec_excluded_files.begin(), vec_excluded_files.end(), [](const auto& file)
		{
			std::cout << file << "\n"; 
		});


	std::cout << "all edited files:\n";

	std::for_each(vec_edited_files.begin(), vec_edited_files.end(), [](const auto& file)
		{
			std::cout << file << "\n";
		});

	std::cout << "success";
	int retval = 0;
	std::cin >> retval;
	return retval;
}

