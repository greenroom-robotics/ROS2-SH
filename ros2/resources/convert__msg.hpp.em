// generated from is-ros2/resources/convert__msg.hpp.em
// generated code does not contain a copyright notice

@#######################################################################
@# EmPy template for generating is/rosidl/ros2/<package>/include/is/rosidl/ros2/<package>/msg/convert__msg__<msg>.hpp files
@#
@# Context:
@#  - spec (rosidl_adapter.parser.MessageSpecification)
@#    Parsed specification of the .msg/.idl file
@#  - subfolder (string)
@#    The subfolder / subnamespace of the message
@#    Either 'msg' or 'srv'
@#  - get_header_filename_from_msg_name (function)
@#######################################################################

@{
camelcase_msg_type = spec.base_type.type
underscore_msg_type = get_header_filename_from_msg_name(camelcase_msg_type)

cpp_msg_type = '{}::msg::{}'.format(
      spec.base_type.pkg_name, camelcase_msg_type)

msg_type_string = '{}/msg/{}'.format(
      spec.base_type.pkg_name, camelcase_msg_type)

header_guard_parts = [
    '_IS_SH_ROS2_ROSIDL__ROS2', spec.base_type.pkg_name, 'MSG__CONVERT__MSG',
    underscore_msg_type + '_HPP_']
header_guard_variable = '__'.join([x.upper() for x in header_guard_parts])

namespace_parts = [
    'convert', spec.base_type.pkg_name, 'msg', underscore_msg_type]
namespace_variable = '__'.join(namespace_parts)

ros2_msg_dependency = '{}/msg/{}.hpp'.format(
      spec.base_type.pkg_name, underscore_msg_type)

conversion_dependencies = {}
for field in spec.fields:
    if field.type.is_primitive_type():
        continue

    key = 'is/rosidl/ros2/{}/msg/convert__msg__{}.hpp'.format(
        field.type.pkg_name, field.type.type)
    if key not in conversion_dependencies:
        conversion_dependencies[key] = set([])
    conversion_dependencies[key].add(field.name)

alphabetical_fields = sorted(spec.fields, key=lambda x: x.name)
}@

#ifndef @(header_guard_variable)
#define @(header_guard_variable)

#include <stdexcept>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <string>

#include <unistd.h>

// Include the header for the generic message type
// #include <is/core/Message.hpp>

#include <is/sh/ros2/Factory.hpp>
#include <is/utils/Convert.hpp>

// Convert<> specialization for rosidl::Buffer<T> (storage for primitive
// array/sequence fields in newer ROS distributions).
#include <is/sh/ros2/RosidlBufferConvert.hpp>

// IDL sanitiser: strips 'verbatim' comment annotations and duplicate type
// definitions that the Fast-DDS IDL parser rejects.
#include <is/sh/ros2/IdlPreprocess.hpp>

#include "rclcpp/serialization.hpp"
#include "rclcpp/serialized_message.hpp"

// Include the header for the concrete ros2 message type
#include <@(ros2_msg_dependency)>

// Include the headers for the message conversion dependencies
@[if conversion_dependencies.keys()]@
@[    for key in sorted(conversion_dependencies.keys())]@
#include <@(key)> // @(', '.join(conversion_dependencies[key]))
@[    end for]@
@[else]@
// <none>
@[end if]@

namespace eprosima {
namespace is {
namespace sh {
namespace ros2 {
namespace @(namespace_variable) {

using Ros2_Msg = @(cpp_msg_type);
const std::string g_msg_name = "@(msg_type_string)";
const std::string g_idl = R"~~~(
@(idl)
)~~~";

//==============================================================================
// Build the DynamicType for this message by parsing the embedded IDL.
//
// Fast-DDS 3.x only exposes an IDL parser that reads from a file URI
// (create_type_w_uri); parsing directly from a string (create_type_w_document)
// is declared but not implemented. The generated IDL inlines every dependency
// type, so it is fully self-contained and needs no preprocessor / include paths.
// We therefore spill g_idl to a temporary file, parse it once and cache the
// resulting (immutable) type.
inline eprosima::xtypes::DynamicType type()
{
    static const eprosima::xtypes::DynamicType cached_type =
        []() -> eprosima::xtypes::DynamicType
        {
            namespace fs = std::filesystem;

            std::string sanitized = g_msg_name;
            std::replace(sanitized.begin(), sanitized.end(), '/', '_');
            std::replace(sanitized.begin(), sanitized.end(), ':', '_');

            const fs::path idl_path = fs::temp_directory_path() /
                ("is_ros2_" + sanitized + "_" + std::to_string(::getpid()) + ".idl");

            {
                std::ofstream idl_file(idl_path);
                idl_file << idl_preprocess::preprocess(g_idl);
            }

            auto builder = eprosima::xtypes::DynamicTypeBuilderFactory::get_instance()
                ->create_type_w_uri(idl_path.string(), "@(cpp_msg_type)", {});

            std::error_code ec;
            fs::remove(idl_path, ec);

            if (!builder)
            {
                throw std::runtime_error("Failed while parsing type @(cpp_msg_type)");
            }
            return builder->build();
        }();
    return cached_type;
}

// Top-level conversion: operates directly on the message-level DynamicData.
void convert_to_ros2([[maybe_unused]] const eprosima::xtypes::DynamicData& from, [[maybe_unused]] Ros2_Msg& to);
void convert_to_xtype([[maybe_unused]] const Ros2_Msg& from, [[maybe_unused]] eprosima::xtypes::DynamicData& to);

// Field-level conversion: loans the member identified by 'id' from its parent
// DynamicData. These overloads are the ones registered into utils::MessageConvert
// so that this message type can be nested as a field inside another message.
void convert_to_ros2(const eprosima::xtypes::DynamicData& from, eprosima::xtypes::MemberId id, Ros2_Msg& to);
void convert_to_xtype(const Ros2_Msg& from, eprosima::xtypes::DynamicData& to, eprosima::xtypes::MemberId id);

void serialise(const eprosima::xtypes::DynamicData& from, rclcpp::SerializedMessage& to);
void deserialise(const rclcpp::SerializedMessage& message, eprosima::xtypes::DynamicData to);

} //  namespace @(namespace_variable)
} //  namespace ros2
} //  namespace sh

namespace utils {
template<>
struct Convert<sh::ros2::@(namespace_variable)::Ros2_Msg>
    : MessageConvert<
     sh::ros2::@(namespace_variable)::Ros2_Msg,
    &sh::ros2::@(namespace_variable)::convert_to_ros2,
    &sh::ros2::@(namespace_variable)::convert_to_xtype
    > { };

} //  namespace utils
} //  namespace is
} //  namespace eprosima

#endif // @(header_guard_variable)
