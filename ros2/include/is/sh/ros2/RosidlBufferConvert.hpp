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

#ifndef _IS_SH_ROS2__INCLUDE__ROSIDL_BUFFER_CONVERT_HPP_
#define _IS_SH_ROS2__INCLUDE__ROSIDL_BUFFER_CONVERT_HPP_

#include <is/utils/Convert.hpp>

#include <rosidl_buffer/buffer.hpp>

#include <cstddef>
#include <cstdint>

namespace eprosima {
namespace is {
namespace utils {

//==============================================================================
/**
 * @brief Convert<> specialization for rosidl::Buffer<T>.
 *
 * Newer ROS distributions store primitive array/sequence message fields in
 * rosidl::Buffer<T> (a drop-in std::vector<T> replacement supporting
 * vendor-specific memory backends) instead of std::vector<T>. The generated
 * ROS 2 glue therefore instantiates Convert<rosidl::Buffer<T>>.
 *
 * On the xTypes side these fields are still modelled as TK_SEQUENCE, so this
 * mirrors the std::vector specialization in is/utils/Convert.hpp exactly,
 * swapping the std::vector operations for their Buffer equivalents
 * (size()/resize()/operator[], all of which Buffer provides).
 *
 * This specialization is kept in ROS2-SH (rather than is-core's Convert.hpp)
 * so that is-core remains free of any ROS dependency.
 */
template<typename ElementType, typename Allocator>
struct Convert<rosidl::Buffer<ElementType, Allocator>>
{
    using native_type = rosidl::Buffer<ElementType, Allocator>;
    static constexpr bool type_is_primitive = Convert<ElementType>::type_is_primitive;

    static void from_xtype_field(
            const xtypes::DynamicData& from,
            xtypes::MemberId seq_id,
            native_type& to)
    {
        auto seq_data = from->loan_value(seq_id);
        const std::size_t N = static_cast<std::size_t>(seq_data->get_item_count());
        to.resize(N);
        for (std::size_t i = 0; i < N; ++i)
        {
            xtypes::MemberId elem_id = seq_data->get_member_id_at_index(
                static_cast<uint32_t>(i));
            Convert<ElementType>::from_xtype_field(seq_data, elem_id, to[i]);
        }
        from->return_loaned_value(seq_data);
    }

    static void to_xtype_field(
            const native_type& from,
            xtypes::DynamicData& to,
            xtypes::MemberId seq_id)
    {
        auto seq_data = to->loan_value(seq_id);
        const std::size_t N = from.size();
        seq_data->clear_all_values();
        for (std::size_t i = 0; i < N; ++i)
        {
            xtypes::MemberId elem_id = static_cast<xtypes::MemberId>(i);
            Convert<ElementType>::to_xtype_field(from[i], seq_data, elem_id);
        }
        to->return_loaned_value(seq_data);
    }
};

} // namespace utils
} // namespace is
} // namespace eprosima

#endif // _IS_SH_ROS2__INCLUDE__ROSIDL_BUFFER_CONVERT_HPP_
