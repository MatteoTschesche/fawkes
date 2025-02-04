
/***************************************************************************
 *  argparser.cpp - Implementation of the argument parser
 *
 *  Generated: Mon May 30 13:25:33 2005 (from FireVision)
 *  Copyright  2005-2006  Tim Niemueller [www.niemueller.de]
 *
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version. A runtime exception applies to
 *  this software (see LICENSE.GPL_WRE file mentioned below for details).
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL_WRE file in the doc directory.
 */

#include <core/exceptions/software.h>
#include <utils/system/argparser.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <libgen.h>
#include <string>

namespace fawkes {

/** @class ArgumentParser <utils/system/argparser.h>
 * Parse command line arguments.
 * Interface to GNU getopt and getopt_long. Parses command line arguments and
 * separates long and short options.
 *
 * The supplied opt_string is a string containing the legitimate option
 * characters. A character c denotes an option of the type "-c" (single dash).
 * If such a character is followed by a colon, the option requires an argument,
 * Two colons mean an option takes an optional arg.
 *
 * If long_options is supplied options started out by two dashes are recognized.
 * Long option names may be abbreviated if the abbreviation is unique or is an
 * exact match for some defined option. A long option may take a parameter, of
 * the form --arg=param or --arg param.
 *
 * long_options is a pointer to the first element of an array of struct option
 * declared in <getopt.h> as
 *
 * @code
 * struct option {
 *   const char *name;
 *   int has_arg;
 *   int *flag;
 *   int val;
 * };
 * @endcode
 *
 * The meanings of the different fields are:
 *
 * name   is the name of the long option.
 *
 *  has_arg   is: no_argument (or 0) if the option does not take  an  argument;
 *            required_argument  (or  1)  if  the  option  requires  an  argument;
 *            or optional_argument (or 2) if the option takes an optional argument.
 *
 *     flag   specifies  how results are returned for a long option.  If flag is
 *            NULL, then getopt_long() returns val.  (For example, the calling
 *            program may set val to the equivalent short option character.)
 *            Otherwise, getopt_long() returns 0, and flag points to a variable
 *            which is set to val if the option is found, but left unchanged if the
 *            option is not found. Handled internally in ArgumentParser
 *
 * For more information see man 3 getopt.
 *
 * All arguments that do not belong to parsed options are stored as items and can
 * be retrieved via items().
 */

/** Constructor
 * @param argc argument count.
 * @param argv argument vector
 * @param opt_string option string, see ArgumentParser
 * @param long_options long options, see ArgumentParser
 */
ArgumentParser::ArgumentParser(int argc, char **argv, const char *opt_string, option *long_options)
{
	argc_ = argc;
	argv_ = argv;

	opt_string_ = opt_string;

	if (long_options) {
		option *tmplo = long_options;
		while (tmplo->name != 0) {
			long_opts_.push_back(*tmplo);
			tmplo += 1;
		}
	}

	opts_.clear();
	items_.clear();

#ifdef _GNU_SOURCE
	program_name_ = strdup(basename(argv[0]));
#else
	// Non-GNU variants may modify the sting in place
	char *tmp     = strdup(argv[0]);
	program_name_ = strdup(basename(tmp));
	free(tmp);
#endif

	if (long_options == NULL) {
		int  c;
		char tmp[2];

		while ((c = getopt(argc, argv, opt_string)) != -1) {
			if (c == '?') {
				throw UnknownArgumentException(c);
			} else if (c == ':') {
				throw MissingArgumentException(c);
			}
			sprintf(tmp, "%c", c);
			opts_[tmp] = optarg;
		}
	} else {
		int opt_ind = 0;
		int c;
		while ((c = getopt_long(argc, argv, opt_string, long_options, &opt_ind)) != -1) {
			if (c == '?') {
				throw UnknownArgumentException(c);
			} else if (c == 0) {
				// long options
				opts_[long_options[opt_ind].name] = optarg;
			} else {
				char tmp[2];
				sprintf(tmp, "%c", c);
				opts_[tmp] = optarg;
			}
		}
	}

	items_.clear();
	int ind = optind;
	while (ind < argc) {
		items_.push_back(argv[ind++]);
	}
}

/** Destructor. */
ArgumentParser::~ArgumentParser()
{
	free(program_name_);
	opts_.clear();
}

/** Check if argument has been supplied.
 * @param argn argument name to check for
 * @return true, if the argument was given on the command line, false otherwise
 */
bool
ArgumentParser::has_arg(const char *argn)
{
	return (opts_.count((char *)argn) > 0);
}

/** Get argument value.
 * Use this method to get the value supplied to the given option.
 * @param argn argument name to retrieve
 * @return the argument value. Pointer to static program array. Do not free!
 * Returns NULL if argument was not supplied on command line.
 */
const char *
ArgumentParser::arg(const char *argn)
{
	if ((opts_.count(argn) > 0) && (opts_[argn] != NULL)) {
		return opts_[(char *)argn];
	} else {
		return NULL;
	}
}

/** Get argument while checking availability.
 * The argument will be a newly allocated copy of the string. You have to
 * free it after you are done with it.
 * @param argn argument name to retrieve
 * @param value a pointer to a newly allocated copy of the argument value will
 * be stored here if the argument has been found.
 * The value is unchanged if argument was not supplied.
 * @return true, if the argument was supplied, false otherwise
 */
bool
ArgumentParser::arg(const char *argn, char **value)
{
	if ((opts_.count(argn) > 0) && (opts_[argn] != NULL)) {
		*value = strdup(opts_[(char *)argn]);
		return true;
	} else {
		return false;
	}
}

/** Parse host:port string.
 * The value referenced by the given argn is parsed for the pattern "host:port".
 * If the string does not match this pattern an exception is thrown.
 * The host will be a newly allocated copy of the string. You have to
 * free it after you are done with it. If no port is supplied in the string (plain
 * hostname string) the port argument is left unchanged. If the argument has not
 * been supplied at all both values are left unchanged. Thus it is safe to put the
 * default values into the variables before passing them to this method. Note
 * however that you have to free the returned host string in case of a successful
 * return, and only in that case probably!
 * @param argn argument name to retrieve
 * @param host Upon successful return contains a pointer to a newly alloated string
 * with the hostname part. Free it after you are finished.
 * @param port upon successful return contains the port part
 * @return true, if the argument was supplied, false otherwise
 * @exception OutOfBoundsException thrown if port is not in the range [0..65535]
 */
bool
ArgumentParser::parse_hostport(const char *argn, char **host, unsigned short int *port)
{
	if ((opts_.count(argn) > 0) && (opts_[argn] != NULL)) {
		parse_hostport_s(opts_[(char *)argn], host, port);
		return true;
	} else {
		return false;
	}
}

/** Parse host:port string.
 * The value referenced by the given argn is parsed for the pattern "host:port".
 * If the string does not match this pattern an exception is thrown.
 * The host will be a newly allocated copy of the string. You have to
 * free it after you are done with it. If no port is supplied in the string (plain
 * hostname string) the port argument is left unchanged. If the argument has not
 * been supplied at all both values are left unchanged. Thus it is safe to put the
 * default values into the variables before passing them to this method. Note
 * however that you have to free the returned host string in case of a successful
 * return, and only in that case probably!
 * @param s string to parse
 * @param host Upon successful return contains a pointer to a newly alloated string
 * with the hostname part. Free it after you are finished.
 * @param port upon successful return contains the port part
 * @exception Exception thrown on parsing error
 */
void
ArgumentParser::parse_hostport_s(const char *s, char **host, unsigned short int *port)
{
	std::string            tmp        = s;
	size_t                 num_colons = 0;
	std::string::size_type idx        = 0;
	while ((idx = tmp.find(':', idx)) != std::string::npos) {
		idx += 1;
		num_colons += 1;
	}

	if (num_colons == 1) {
		idx   = tmp.find(':');
		*host = strdup(tmp.substr(0, idx).c_str());
		if (!tmp.substr(idx + 1).empty()) {
			*port = atoi(tmp.substr(idx + 1).c_str());
		}
	} else if (num_colons > 1) {
		// IPv6
		if (tmp[0] == '[') {
			// notation that actually contains a port
			std::string::size_type closing_idx = tmp.find(']');
			if (closing_idx == std::string::npos) {
				throw Exception("No closing bracket for IPv6 address");
			} else if (closing_idx < (tmp.length() - 1)) {
				// there might be a port
				if (tmp[closing_idx + 1] != ':') {
					throw Exception("Expected colon after closing IPv6 address bracket");
				} else if (closing_idx > tmp.length() - 3) {
					throw Exception(
					  "Malformed IPv6 address with port, not enough characters after closing bracket");
				} else {
					*host = strdup(tmp.substr(1, closing_idx - 1).c_str());
					*port = atoi(tmp.substr(closing_idx + 2).c_str());
				}
			} else {
				// Just an IPv6 in bracket notation
				*host = strdup(tmp.substr(1, closing_idx - 2).c_str());
			}
		} else {
			// no port, just an IPv6 address
			*host = strdup(tmp.c_str());
		}
	} else {
		// no port given
		*host = strdup(tmp.c_str());
	}
}

/** Parse host:port string.
 * The value referenced by the given argn is parsed for the pattern "host:port". If the
 * string does not match this pattern an exception is thrown.
 * If no port is supplied in the string (plain
 * hostname string) the port argument is left unchanged. If the argument has not
 * been supplied at all both values are left unchanged. Thus it is safe to put the default
 * values into the variables before passing them to this method.
 * @param argn argument name to retrieve
 * @param host Upon successful return contains the hostname part
 * @param port upon successful return contains the port part (unchanged if not supplied)
 * @return true, if the argument was supplied, false otherwise
 * @exception OutOfBoundsException thrown if port is not in the range [0..65535]
 */
bool
ArgumentParser::parse_hostport(const char *argn, std::string &host, unsigned short int &port)
{
	if ((opts_.count(argn) == 0) || (opts_[argn] == NULL))
		return false;

	char *             tmp_host = NULL;
	unsigned short int tmp_port = port;
	if (parse_hostport(argn, &tmp_host, &tmp_port)) {
		host = tmp_host;
		port = tmp_port;
		return true;
	}
	return false;
}

/** Parse host:port string.
 * The value referenced by the given argn is parsed for the pattern "host:port". If the
 * string does not match this pattern an exception is thrown.
 * If no port is supplied in the string (plain
 * hostname string) the port argument is left unchanged. If the argument has not
 * been supplied at all both values are left unchanged. Thus it is safe to put the default
 * values into the variables before passing them to this method.
 * @param s string to parse
 * @param host Upon successful return contains the hostname part
 * @param port upon successful return contains the port part (unchanged if not supplied)
 * @exception OutOfBoundsException thrown if port is not in the range [0..65535]
 */
void
ArgumentParser::parse_hostport_s(const char *s, std::string &host, unsigned short int &port)
{
	char *             tmp_host = NULL;
	unsigned short int tmp_port = port;
	parse_hostport_s(s, &tmp_host, &tmp_port);
	host = tmp_host;
	port = tmp_port;
}

/** Parse argument as integer.
 * Converts the value of the given argument to an integer.
 * @param argn argument name to retrieve
 * @return value of string as long int
 * @exception IllegalArgumentException thrown if the value cannot be properly
 * converted to an integer
 * @exception Exception thrown if the argument has not been supplied
 */
long int
ArgumentParser::parse_int(const char *argn)
{
	if ((opts_.count(argn) > 0) && (opts_[argn] != NULL)) {
		char *   endptr;
		long int rv = strtol(opts_[argn], &endptr, 10);
		if (endptr[0] != 0) {
			throw IllegalArgumentException("Supplied argument is not of type int");
		}
		return rv;
	} else {
		throw Exception("Value for '%s' not available", argn);
	}
}

/** Parse argument as double.
 * Converts the value of the given argument to a double.
 * @param argn argument name to retrieve
 * @return value of string as double
 * @exception IllegalArgumentException thrown if the value cannot be properly
 * converted to a double
 * @exception Exception thrown if the argument has not been supplied
 */
double
ArgumentParser::parse_float(const char *argn)
{
	if ((opts_.count(argn) > 0) && (opts_[argn] != NULL)) {
		char * endptr;
		double rv = strtod(opts_[argn], &endptr);
		if (endptr[0] != 0) {
			throw IllegalArgumentException("Supplied argument is not of type double");
		}
		return rv;
	} else {
		throw Exception("Value for '%s' not available", argn);
	}
}

/** Parse item as integer.
 * Converts the value of the given item to an integer.
 * @param index item index
 * @return value of string as long int
 * @exception IllegalArgumentException thrown if the value cannot be properly
 * converted to an integer
 * @exception Exception thrown if the argument has not been supplied
 */
long int
ArgumentParser::parse_item_int(unsigned int index)
{
	if (index < items_.size()) {
		char *   endptr;
		long int rv = strtol(items_[index], &endptr, 10);
		if (endptr[0] != 0) {
			throw IllegalArgumentException("Supplied argument is not of type int");
		}
		return rv;
	} else {
		throw Exception("Value for item %u not available", index);
	}
}

/** Parse item as double.
 * Converts the value of the given item to a double.
 * @param index item index
 * @return value of string as double
 * @exception IllegalArgumentException thrown if the value cannot be properly
 * converted to a double
 * @exception Exception thrown if the argument has not been supplied
 */
double
ArgumentParser::parse_item_float(unsigned int index)
{
	if (index < items_.size()) {
		char * endptr;
		double rv = strtod(items_[index], &endptr);
		if (endptr[0] != 0) {
			throw IllegalArgumentException("Supplied argument is not of type double");
		}
		return rv;
	} else {
		throw Exception("Value for item %u not available", index);
	}
}

/** Get non-option items.
 * @return pointer to vector of pointer to non-argument values. Handled internally,
 * do not free or delete!
 */
const std::vector<const char *> &
ArgumentParser::items() const
{
	return items_;
}

/** Get number of non-option items.
 * @return number of non-opt items.
 */
std::vector<const char *>::size_type
ArgumentParser::num_items() const
{
	return items_.size();
}

/** Get number of arguments.
 * @return number of arguments
 */
int
ArgumentParser::argc() const
{
	return argc_;
}

/** Program argument array as supplied to constructor.
 * @return argument array.
 */
const char **
ArgumentParser::argv() const
{
	return (const char **)argv_;
}

/** Get name of program.
 * @return the name of the program (argv[0] of argument vector supplied to constructor).
 */
const char *
ArgumentParser::program_name() const
{
	return program_name_;
}

} // end namespace fawkes
