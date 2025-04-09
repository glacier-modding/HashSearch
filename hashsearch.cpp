#define _GLIBCXX_USE_CXX11_ABI 0

#include <stdlib.h>
#include <iostream>
#include <string>
#include "fcgio.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <stdint.h>
#include "json.hpp"

std::string hash_list_file_path = "hash_list.txt";

std::string hash_list_string;
int hash_list_version;
std::vector<std::string> hash_list_lines;

using json = nlohmann::json;

// Maximum bytes
const unsigned long STDIN_MAX = 1000000;

/**
 * Note this is not thread safe due to the static allocation of the
 * content_buffer.
 */
std::string get_request_content(const FCGX_Request & request) {
    char * content_length_str = FCGX_GetParam("CONTENT_LENGTH", request.envp);
    unsigned long content_length = STDIN_MAX;

    if (content_length_str) {
        content_length = strtol(content_length_str, &content_length_str, 10);
        if (*content_length_str) {
            std::cerr << "Can't Parse 'CONTENT_LENGTH='"
                 << FCGX_GetParam("CONTENT_LENGTH", request.envp)
                 << "'. Consuming stdin up to " << STDIN_MAX << std::endl;
        }

        if (content_length > STDIN_MAX) {
            content_length = STDIN_MAX;
        }
    } else {
        // Do not read from stdin if CONTENT_LENGTH is missing
        content_length = 0;
    }

    std::vector<char> content_buffer(content_length, 0);
    std::cin.read(reinterpret_cast<char*>(&content_buffer.data()[0]), content_length);
    content_length = std::cin.gcount();

    // Chew up any remaining stdin - this shouldn't be necessary
    // but is because mod_fastcgi doesn't handle it correctly.

    // ignore() doesn't set the eof bit in some versions of glibc++
    // so use gcount() instead of eof()...
    do std::cin.ignore(1024); while (std::cin.gcount() == 1024);

    std::string content(reinterpret_cast<char*>(&content_buffer.data()[0]), content_length);
    return content;
}

std::string to_lower_case(std::string s)
{
    for (uint64_t i = 0; i < s.length(); i++)
    {
        s[i] = std::tolower(s[i]);
    }

    return s;
}

std::string to_upper_case(std::string s)
{
    for (uint64_t i = 0; i < s.length(); i++)
    {
        s[i] = std::toupper(s[i]);
    }

    return s;
}

int main(void)
{
    std::ifstream hash_list_file = std::ifstream(hash_list_file_path, std::ifstream::binary);

    hash_list_file.seekg(0, hash_list_file.end);

    uint64_t hash_list_file_size = (uint64_t)hash_list_file.tellg();

    hash_list_file.seekg(0, hash_list_file.beg);

    std::vector<char> hash_list_data(hash_list_file_size, 0);

    hash_list_file.read(hash_list_data.data(), hash_list_file_size);

    bool done = false;
    uint64_t position = 0;
    uint64_t last_position = 0;
    uint64_t line_count = 0;

    while (!done)
    {
        if (hash_list_data.data()[position] == 0x0A)
        {
            line_count++;

            hash_list_data.data()[position] = 0x0;

            if (line_count == 3)
            {
                std::string temp_hash_string = std::string(reinterpret_cast<char*>(&hash_list_data.data()[last_position]));

                size_t pos = temp_hash_string.find_first_of('=');

                std::string hash_list_version_string = temp_hash_string.substr((pos + 1));

                hash_list_version = std::stoi(hash_list_version_string);
            }
            else if (line_count > 3)
            {
                std::string temp_hash_string = std::string(reinterpret_cast<char*>(&hash_list_data.data()[last_position]));

                temp_hash_string = to_lower_case(temp_hash_string);

                hash_list_lines.push_back(temp_hash_string);
            }

            last_position = position + 1;
        }

        position++;

        if (position > hash_list_file_size)
        {
            done = true;
        }
    }

    // Backup the stdio streambufs
    std::streambuf * cin_streambuf  = std::cin.rdbuf();
    std::streambuf * cout_streambuf = std::cout.rdbuf();
    std::streambuf * cerr_streambuf = std::cerr.rdbuf();

    FCGX_Request request;

    FCGX_Init();
    FCGX_InitRequest(&request, 0, 0);

    while (FCGX_Accept_r(&request) == 0) {
        fcgi_streambuf cin_fcgi_streambuf(request.in);
        fcgi_streambuf cout_fcgi_streambuf(request.out);
        fcgi_streambuf cerr_fcgi_streambuf(request.err);

        std::cin.rdbuf(&cin_fcgi_streambuf);
        std::cout.rdbuf(&cout_fcgi_streambuf);
        std::cerr.rdbuf(&cerr_fcgi_streambuf);

        const char * uri = FCGX_GetParam("REQUEST_URI", request.envp);

        std::string search_command = get_request_content(request);

        std::string results = "Content-Type: application/json\r\n";

        json reqJson = json::parse(search_command, nullptr, false);

        if (reqJson.is_discarded()) {
            results.append("Status: 400\r\n\r\n");
            results.append("{\"error\": \"invalid request json\"}");
        } else {
            std::string search_string = "";

            std::string results_per_page_string = "";
            int results_per_page = 0;

            std::string results_per_page_index_string = "";
            int results_per_page_index = 0;

            std::string resource_type_string = "";

            bool request_is_ok = false;

            int comma_count = 0;

            bool containsAll = (reqJson.contains("search_term") && reqJson.contains("number_of_results") && reqJson.contains("resource_type") && reqJson.contains("page_number"));
            bool correctTypes = (reqJson["search_term"].type_name() == "string" && reqJson["number_of_results"].type_name() == "number"
                                && reqJson["resource_type"].type_name() == "string" && reqJson["page_number"].type_name() == "number");

            if (containsAll && correctTypes) {
                try {
                    search_string = reqJson["search_term"].get<std::string>();

                    results_per_page = reqJson["number_of_results"].get<int>();
                    results_per_page_string = std::to_string(results_per_page);

                    if(results_per_page > 500) {
                        results.append("Status: 400\r\n\r\n");
                        results.append("{\"error\": \"number_of_results exceeds the limit of 500\"}");
                        std::cout << results;
                        continue;
                    }

                    resource_type_string = to_lower_case(reqJson["resource_type"].get<std::string>());

                    results_per_page_index = reqJson["page_number"].get<int>();
                    results_per_page_index_string = std::to_string(results_per_page_index);

                    request_is_ok = true;
                } catch (std::exception e) {
                    results.append("Status: 400\r\n\r\n");
                    results.append("{\"error\": \"invalid request json\"}");
                }
            } else {
                results.append("Status: 400\r\n\r\n");
                results.append("{\"error\": \"invalid request json\"}");
            }

            if (request_is_ok)
            {
                results.append("\r\n");
                search_string = to_lower_case(search_string);

                uint64_t current_index = 0;

                uint64_t starting_index = results_per_page * results_per_page_index;
                uint64_t ending_index = starting_index + results_per_page;

                json outJson;
                outJson["results"] = {};
                outJson["number_of_results"] = results_per_page;
                outJson["page_number"] = results_per_page_index;
                for (int i = 0; i < hash_list_lines.size(); i++)
                {
                    json temp_json;
                    if (hash_list_lines.at(i).find(search_string) != std::string::npos && ((hash_list_lines.at(i).substr(17,4).find(resource_type_string) != std::string::npos) || (resource_type_string == "any")))
                    {
                        if (current_index >= ending_index)
                        {
                            break;   
                        }
                        else if (current_index >= starting_index)
                        {
                            temp_json["hash"] = to_upper_case(hash_list_lines.at(i).substr(0,16));
                            temp_json["type"] = to_upper_case(hash_list_lines.at(i).substr(17,4));
                            temp_json["string"] = hash_list_lines.at(i).substr(22,hash_list_lines.at(i).size());

                            outJson["results"].push_back(temp_json);
                        }

                        current_index++;
                    }
                }

                results.append(outJson.dump());
            }
        }

        std::cout << results;
    }

    // restore stdio streambufs
    std::cin.rdbuf(cin_streambuf);
    std::cout.rdbuf(cout_streambuf);
    std::cerr.rdbuf(cerr_streambuf);

    return 0;
}
