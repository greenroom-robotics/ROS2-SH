/*
 * Copyright (C) 2018 Open Source Robotics Foundation
 * Copyright (C) 2020 - present Proyectos y Sistemas de Mantenimiento SL (eProsima).
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef _IS_SH_ROS2__INCLUDE__IDL_PREPROCESS_HPP_
#define _IS_SH_ROS2__INCLUDE__IDL_PREPROCESS_HPP_

#include <cctype>
#include <cstddef>
#include <set>
#include <string>
#include <vector>

namespace eprosima {
namespace is {
namespace sh {
namespace ros2 {
namespace idl_preprocess {

// The rosidl-generated IDL embedded in the glue code is written for the legacy
// eprosima::xtypes parser, which accepted constructs that the Fast-DDS 3.x IDL
// parser (DynamicTypeBuilderFactory::create_type_w_uri) rejects:
//
//   1. '@verbatim (language="comment", text=...)' annotations carrying the
//      original .msg comments. The legacy parser set allow_keyword_identifiers;
//      Fast-DDS fails to parse these annotations outright.
//   2. Repeated type definitions. The legacy parser set ignore_redefinition;
//      Fast-DDS errors on redefinition. Service IDL in particular concatenates
//      the full dependency closure of both the request and the response, so
//      shared types (Header, Time, Point, ...) are defined several times.
//
// The two helpers below sanitise the IDL before it is handed to Fast-DDS:
// '@verbatim (...)' blocks are dropped, and duplicate top-level 'module' blocks
// (keyed by the fully-qualified names of the structs they define) are removed.

//==============================================================================
inline std::string strip_verbatim(
        const std::string& idl)
{
    std::string out;
    out.reserve(idl.size());

    const std::string tag = "@verbatim";
    std::size_t i = 0;
    while (i < idl.size())
    {
        if (idl.compare(i, tag.size(), tag) == 0)
        {
            // Advance to the opening parenthesis of the annotation.
            std::size_t j = i + tag.size();
            while (j < idl.size() && idl[j] != '(')
            {
                ++j;
            }

            if (j < idl.size())
            {
                // Skip up to (and including) the matching close parenthesis,
                // honouring string literals so quoted parens/escapes inside the
                // comment text do not unbalance the scan.
                int depth = 0;
                bool in_string = false;
                for (; j < idl.size(); ++j)
                {
                    const char c = idl[j];
                    if (in_string)
                    {
                        if (c == '\\')
                        {
                            ++j;
                            continue;
                        }
                        if (c == '"')
                        {
                            in_string = false;
                        }
                    }
                    else if (c == '"')
                    {
                        in_string = true;
                    }
                    else if (c == '(')
                    {
                        ++depth;
                    }
                    else if (c == ')')
                    {
                        if (--depth == 0)
                        {
                            ++j;
                            break;
                        }
                    }
                }
                i = j;
                continue;
            }
        }
        out.push_back(idl[i++]);
    }
    return out;
}

//==============================================================================
inline std::string dedup_modules(
        const std::string& idl)
{
    const auto is_ident = [](char c) -> bool
            {
                return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
            };

    std::string out;
    std::set<std::string> seen;
    std::size_t i = 0;
    while (i < idl.size())
    {
        if (idl.compare(i, 6, "module") == 0 && (i == 0 || !is_ident(idl[i - 1])))
        {
            // Find the extent of this top-level 'module { ... };' block by
            // matching braces, then consume an optional trailing ';'.
            std::size_t brace = i;
            while (brace < idl.size() && idl[brace] != '{')
            {
                ++brace;
            }

            int depth = 0;
            std::size_t j = brace;
            for (; j < idl.size(); ++j)
            {
                if (idl[j] == '{')
                {
                    ++depth;
                }
                else if (idl[j] == '}')
                {
                    if (--depth == 0)
                    {
                        ++j;
                        break;
                    }
                }
            }
            while (j < idl.size() && idl[j] == ';')
            {
                ++j;
                break;
            }

            const std::string block = idl.substr(i, j - i);

            // Build a key from the fully-qualified names of the structs defined
            // in the block (module identifiers joined with '::').
            std::string key;
            std::vector<std::string> modules;
            std::size_t p = 0;
            while (p < block.size())
            {
                if (block.compare(p, 6, "module") == 0 && (p == 0 || !is_ident(block[p - 1])))
                {
                    p += 6;
                    while (p < block.size() && std::isspace(static_cast<unsigned char>(block[p])))
                    {
                        ++p;
                    }
                    std::string id;
                    while (p < block.size() && is_ident(block[p]))
                    {
                        id.push_back(block[p++]);
                    }
                    modules.push_back(id);
                }
                else if (block.compare(p, 6, "struct") == 0 && (p == 0 || !is_ident(block[p - 1])))
                {
                    p += 6;
                    while (p < block.size() && std::isspace(static_cast<unsigned char>(block[p])))
                    {
                        ++p;
                    }
                    std::string id;
                    while (p < block.size() && is_ident(block[p]))
                    {
                        id.push_back(block[p++]);
                    }
                    std::string qualified;
                    for (const std::string& m : modules)
                    {
                        qualified += m + "::";
                    }
                    qualified += id;
                    key += qualified + ";";
                }
                else
                {
                    ++p;
                }
            }

            // Keep the block if it defines no struct (nothing to deduplicate) or
            // if this is the first time we have seen this set of structs.
            if (key.empty() || seen.insert(key).second)
            {
                out += block;
            }
            i = j;
            continue;
        }
        out.push_back(idl[i++]);
    }
    return out;
}

//==============================================================================
inline std::string preprocess(
        const std::string& idl)
{
    return dedup_modules(strip_verbatim(idl));
}

} // namespace idl_preprocess
} // namespace ros2
} // namespace sh
} // namespace is
} // namespace eprosima

#endif // _IS_SH_ROS2__INCLUDE__IDL_PREPROCESS_HPP_
