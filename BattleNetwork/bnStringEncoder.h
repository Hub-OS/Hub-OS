/*! \file bnStringEncoder
 * 
 * These ascii codes are based off a specific true type font
 * and will not work with other fonts
 */

#pragma once

#include <string>

/*!
 * \brief Add EX at the end of a mob name
 */
#define EX(str) std::string(str)+";"

/*!
 * \brief add SP at the end of a mob name
 */
#define SP(str) std::string(str)+"<"
