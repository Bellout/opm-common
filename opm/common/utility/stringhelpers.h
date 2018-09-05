/******************************************************************************
   Created by einar on 9/5/18.
   Copyright (C) 2017 Einar J.M. Baumann <einar.baumann@gmail.com>

   This file is part of the FieldOpt project.

   FieldOpt is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   FieldOpt is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with FieldOpt.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#ifndef OPM_STRINGHELPERS_H
#define OPM_STRINGHELPERS_H

#include <string>
#include <sstream>

template <typename T>
inline std::string ToString(T val)
{
    std::stringstream stream;
    stream << val;
    return stream.str();
}

#endif //OPM_STRINGHELPERS_H
