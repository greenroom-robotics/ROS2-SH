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

#ifndef _IS_SH_ROS2__SRC__IS_TYPE_NAME_HPP_
#define _IS_SH_ROS2__SRC__IS_TYPE_NAME_HPP_

#include <cstddef>
#include <string>

namespace eprosima {
namespace is {
namespace sh {
namespace ros2 {

//==============================================================================
/**
 * @brief Convert a Fast-DDS DynamicType name into the type name used as a key
 *        throughout the Integration Service (configuration, RequiredTypes,
 *        Factory registration) and by rclcpp generic endpoints.
 *
 * Fast-DDS only accepts fully-qualified IDL names, so a DynamicType built for a
 * ROS 2 type is named 'package::msg::Type' (or 'package::srv::Srv_Request' /
 * '..._Response'). The IS convention — and what rclcpp's generic
 * publisher/subscription expect — is the slash form 'package/msg/Type', and for
 * services 'package/srv/Srv:request' / ':response'. The legacy eprosima::xtypes
 * code stored that slash form directly as the type name (via DynamicType::name),
 * which Fast-DDS forbids, so the translation is now done at the lookup sites.
 *
 *   - every '::' becomes '/'
 *   - for service types, a trailing '_Request'/'_Response' becomes
 *     ':request'/':response'
 *
 * This helper lives in a private (non-installed) header so that touching it only
 * rebuilds the is-ros2 library, not every generated conversion translation unit.
 */
inline std::string fastdds_type_to_is_name(
        const std::string& fastdds_name)
{
    std::string result;
    result.reserve(fastdds_name.size());
    for (std::size_t i = 0; i < fastdds_name.size(); ++i)
    {
        if (fastdds_name[i] == ':' && i + 1 < fastdds_name.size() && fastdds_name[i + 1] == ':')
        {
            result.push_back('/');
            ++i;
        }
        else
        {
            result.push_back(fastdds_name[i]);
        }
    }

    if (result.find("/srv/") != std::string::npos)
    {
        const std::string req = "_Request";
        const std::string resp = "_Response";
        if (result.size() >= req.size() &&
                result.compare(result.size() - req.size(), req.size(), req) == 0)
        {
            result = result.substr(0, result.size() - req.size()) + ":request";
        }
        else if (result.size() >= resp.size() &&
                result.compare(result.size() - resp.size(), resp.size(), resp) == 0)
        {
            result = result.substr(0, result.size() - resp.size()) + ":response";
        }
    }
    return result;
}

} // namespace ros2
} // namespace sh
} // namespace is
} // namespace eprosima

#endif // _IS_SH_ROS2__SRC__IS_TYPE_NAME_HPP_
