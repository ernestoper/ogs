/**
 * \file
 * \author Thomas Fischer
 * \date   2010-06-16
 * \brief  Definition of string helper functions.
 *
 * \copyright
 * Copyright (c) 2012-2015, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#ifndef STRINGTOOLS_H
#define STRINGTOOLS_H

#include <boost/property_tree/ptree.hpp>

#include <string>
#include <list>
#include <sstream>

namespace BaseLib {

/**
 *   Splits a string into a list of strings.
 *  \param str String to be splitted
 *  \param delim Character indicating that the string should be splitted
 *  \return List of strings
 */
std::list<std::string> splitString(const std::string &str, char delim);

/**
 *   Replaces a substring with another in a string
 *  \param searchString Search for this string
 *  \param replaceString Replace with this string
 *  \param stringToReplace Search and replace in this string
 *  \return The modified string
 */
std::string replaceString(const std::string &searchString, const std::string &replaceString, std::string stringToReplace);

/**
 *   Converts a string into a number (double, float, int, ...)
 *  Example: std::size_t number (str2number<std::size_t> (str));
 *  \param str string to be converted
 *  \return the number
 */
template<typename T> T str2number (const std::string &str)
{
	std::stringstream strs (str, std::stringstream::in | std::stringstream::out);
	T v;
	strs >> v;
	return v;
}

/**
 * Strip whitespace (or other characters) from the beginning and end of a string.
 * Equivalent functionality to Qt::QString::trim().
 */
void trim(std::string &str, char ch=' ');

/**
 * Removes multiple whitespaces (or other characters) from within a string.
 * Equivalent functionality to Qt::QString::simplify().
 */
void simplify(std::string &str);

/**
 * Returns the string which is right aligned with padding on the left.
 */
std::string padLeft(std::string const& str, int maxlen, char ch=' ');

/**
 * Returns the JSON-representation of the given boost::property_tree.
 */
std::string propertyTreeToString(boost::property_tree::ptree const& tree);

/**
 * Remove a substring from a string.
 * \orig_string The string to be subtracted.
 * \sub_string The substring.
 * \return The string subtracted the substring,
 *         or the original string if the substring is empty or the size
 *         of the substring is large than that of the original one.
 */
std::string removeSubstringFromString(const std::string& orig_string,
                                      const std::string& sub_string);

} // end namespace BaseLib

#ifdef MSVC
void correctScientificNotation(std::string filename, std::size_t precision = 0);
#endif

#endif //STRINGTOOLS_H
