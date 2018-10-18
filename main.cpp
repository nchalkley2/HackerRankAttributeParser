#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <vector>

using namespace std;

struct tag
{
	string name = "";

	map<string, string> attributes = {};

	// An tag can contain a child tag
	unique_ptr<tag> child = nullptr;
};

// Contains the tag in a string and the opening/closing position of the tag.
// The opening/closing position is used for getting the substring of the
// input string without the opening/closing tag
using tag_str_t = pair<string, string::size_type>;
enum tag_type
{
	opening = 1,
	closing = 0
};

// Gets an opening or closing tag from a string and returns the position of
// those tags
template<tag_type opening>
static optional<tag_str_t>
get_tag(const string& str)
{
	auto opening_tag_pos = string::npos;
	auto closing_tag_pos = string::npos;

	// If we are looking for an opening tag, search from the beginning of the
	// string
	if constexpr (opening)
	{
		opening_tag_pos = str.find_first_of('<');
		closing_tag_pos = str.find_first_of('>');
	}
	// If it's a closing tag, search from the end
	else
	{
		opening_tag_pos = str.find_last_of('<');
		closing_tag_pos = str.find_last_of('>');
	}

	if (opening_tag_pos != string::npos && closing_tag_pos != string::npos)
	{
		const auto tag_str = str.substr(opening_tag_pos, closing_tag_pos + 1);
		if constexpr (opening)
			return optional<tag_str_t>({ tag_str, closing_tag_pos + 1 });
		else
			return optional<tag_str_t>({ tag_str, opening_tag_pos });
	}
	else
		return nullopt;
};

template<typename T>
vector<T>
slice(vector<T> const& v, typename vector<T>::size_type m,
	  optional<typename vector<T>::size_type> n = {})
{
	const auto first = v.cbegin() + m;
	auto last		 = v.cbegin();
	if (n.has_value())
		last = last + (n.value() + 1);
	else
		last = v.cend();

	std::vector<T> vec(first, last);
	return vec;
}

static optional<tag>
parsetags(const string& str)
{
	const auto opening_tag = get_tag<opening>(str);
	const auto closing_tag = get_tag<closing>(str);

	if (!opening_tag.has_value() || !closing_tag.has_value())
		return {};

	// Get the tag minus the open/close chevron "<>"
	const auto opening_tag_str
		= opening_tag->first.substr(1, opening_tag->first.length() - 2);

	// Break up that tag into its elements. The elements are delimited by spaces
	const std::vector<string> elems = [](const auto& str) {
		vector<string> outvec;
		stringstream sstr(str);
		string token;
		while (getline(sstr, token, ' '))
		{
			outvec.push_back(token);
		}

		return outvec;
	}(opening_tag_str);

	const auto tag_name = elems[0];

	using elem_t	  = decltype(elems);
	using attribute_t = decltype(tag::attributes);

	// This function recursively gets attributes and returns a map of the
	// attributes
	const function<attribute_t(elem_t)> get_attributes
		= [&get_attributes](elem_t elems) -> attribute_t {
		const auto map_append
			= [](attribute_t lhs, const attribute_t& rhs) -> attribute_t {
			lhs.insert(rhs.begin(), rhs.end());
			return lhs;
		};

		// Make sure there are elements and that the amount of elements is a
		// multiple of 3. It has to be a multiple of three because each
		// attribute takes up three elements.
		// For example: {"value", "=", "\"HelloWorld\""}
		if (elems.size() && elems.size() % 3 == 0)
		{
			// Create the new attribute and recursively append the rest of the
			// attributes to it
			const auto attr
				= pair(elems[0], elems[2].substr(1, elems[2].length() - 2));
			return map_append(attribute_t({ attr }),
							  get_attributes(slice(elems, 3)));
		}
		else
			return {};
	};

	const auto inner_tag = str.substr(
		opening_tag->second, closing_tag->second - opening_tag->second);

	if (auto child_node = parsetags(inner_tag); child_node.has_value())
	{
		return tag{ elems[0], get_attributes(slice(elems, 1)),
					unique_ptr<tag>(new tag(move(child_node.value()))) };
	}
	else
		return tag{ elems[0], get_attributes(slice(elems, 1)), nullptr };
}

using query_elem_t = string;
using query_t	  = vector<query_elem_t>;
query_t
string_to_query(const string& str)
{
	const auto append = [](vector<string> lhs, const vector<string>& rhs) {
		lhs.insert(std::end(lhs), std::begin(rhs), std::end(rhs));
		return lhs;
	};

	if (str.length() == 0)
		return {};

	const auto tag_pos		 = str.find_first_of('.');
	const auto attribute_pos = str.find_first_of('~');

	// if if didnt find a period or a tilde
	if (tag_pos == string::npos && attribute_pos == string::npos)
	{
		return { str };
	}

	// Append the rest of the querys to this vector
	const auto pos = min(tag_pos, attribute_pos);
	return append({ str.substr(0, pos) }, string_to_query(str.substr(pos + 1)));
}

int
main()
{
	// Get the number of lines and queries from the first line
	// The specification does not require error checking so I will omit that
	const auto[lines, queries] = []() {
		string lines_str, queries_str;
		getline(cin, lines_str, ' ');
		getline(cin, queries_str);

		return pair(stoi(lines_str), stoi(queries_str));
	}();

	const auto getlines = [](const int numlines) {
		// Create a vector of input lines
		vector<string> lines(numlines);
		generate(lines.begin(), lines.end(), []() {
			string line;
			getline(cin, line);
			return line;
		});

		return lines;
	};

	// get the tags from cin and concat them to a string
	const string tag_str = [&getlines](const int lines) {
		// Create a vector of input lines
		auto tag_strs = getlines(lines);

		// Fold the lines into a single string
		return accumulate(tag_strs.begin(), tag_strs.end(), string(""));
	}(lines);

	const auto root_tag = parsetags(tag_str);

	if (!root_tag.has_value())
		return -1;

	const vector<string> query_vec = getlines(queries);
	for (const auto& query_str : query_vec)
	{
		const auto query = string_to_query(query_str);

		// Check if the query has anything, then check if the first element
		// is the root element
		if (const auto size = query.size();
			size == 0 || query[0] != root_tag->name)
		{
			if (size > 0)
				cout << "Not Found!\n";
		}
		else
		{
			const auto* head = &root_tag.value();
			for (int i = 2; i <= query.size(); i++)
			{
				// If we are not at the end of the query then we are querying a
				// tag
				if (i != query.size())
				{
					if (head->child && head->child->name == query[i - 1])
					{
						head = head->child.get();
					}
					else
					{
						cout << "Not Found!\n";
						break;
					}
				}
				else
				{
					// If we are at the end of the query then it is querying an
					// attribute
					if (auto itr = head->attributes.find(query[i - 1]);
						itr != head->attributes.end())
					{
						cout << itr->second << "\n";
					}
					else
					{
						cout << "Not Found!\n";
						break;
					}
				}
			}
		}
	}

	return 0;
}

