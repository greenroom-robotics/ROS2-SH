// generated from is-ros2/resources/convert_srv.cpp.em
// generated code does not contain a copyright notice

@#######################################################################
@# EmPy template for generating is/rosidl/ros2/<package>/src/srv/convert__srv__<srv>.cpp files
@#
@# Context:
@#  - spec (rosidl_adapter.parser.ServiceSpecification)
@#    Parsed specification of the .srv/.idl file
@#  - get_header_filename_from_msg_name (function)
@#######################################################################

@{
camelcase_srv_type = spec.srv_name
underscore_srv_type = get_header_filename_from_msg_name(camelcase_srv_type)

cpp_srv_type = '{}::srv::{}'.format(spec.pkg_name, camelcase_srv_type)

srv_type_string = '{}/srv/{}'.format(spec.pkg_name, camelcase_srv_type)

namespace_parts_srv = [
    'convert', spec.pkg_name, 'srv', underscore_srv_type]
namespace_variable_srv = '__'.join(namespace_parts_srv)

namespace_parts_msg = [
    'convert', spec.pkg_name, 'msg', underscore_srv_type]
namespace_variable_msg = '__'.join(namespace_parts_msg)

ros2_srv_dependency = '{}/srv/{}.hpp'.format(
      spec.pkg_name, underscore_srv_type)

conversion_dependencies = {}
for type, msg in {"request": spec.request, "response": spec.response}.items():
    for field in msg.fields:
        if field.type.is_primitive_type():
          continue

        key = 'is/rosidl/ros2/{}/msg/convert__msg__{}.hpp'.format(
            field.type.pkg_name, field.type.type)
        if key not in conversion_dependencies:
            conversion_dependencies[key] = set([])
        conversion_dependencies[key].add(type+'::'+field.name)

alphabetical_request_fields = sorted(spec.request.fields, key=lambda x: x.name)
alphabetical_response_fields = sorted(spec.response.fields, key=lambda x: x.name)
}@

#include <stdexcept>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <string>

#include <unistd.h>

// Fast-DDS DynamicData JSON serialization (json_serialize) for trace logging
#include <fastdds/dds/xtypes/utils.hpp>

// Include the header for the generic message type
// #include <is/core/Message.hpp>

// Include the header for the conversions
#include <is/utils/Convert.hpp>

// Convert<> specialization for rosidl::Buffer<T> (storage for primitive
// array/sequence fields in newer ROS distributions).
#include <is/sh/ros2/RosidlBufferConvert.hpp>

// IDL sanitiser: strips 'verbatim' comment annotations and duplicate type
// definitions that the Fast-DDS IDL parser rejects.
#include <is/sh/ros2/IdlPreprocess.hpp>

// Include the header for the logger
#include <is/utils/Log.hpp>

// Include the header for the concrete service type
#include <@(ros2_srv_dependency)>

// Include the headers for the Integration Service message dependencies
@[for key in sorted(conversion_dependencies.keys())]@
#include <@(key)> // @(', '.join(conversion_dependencies[key]))
@[end for]@

// Include the Factory header so we can add this message type to the Factory
#include <is/sh/ros2/Factory.hpp>

// Include the Node API so we can provide and request services
#include <rclcpp/node.hpp>

#include <chrono>

namespace eprosima {
namespace is {
namespace sh {
namespace ros2 {
namespace @(namespace_variable_srv) {

static eprosima::is::utils::Logger logger ("is::sh::ROS2");

using Ros2_Srv = @(cpp_srv_type);
using Ros2_Request = Ros2_Srv::Request;
using Ros2_Response = Ros2_Srv::Response;
const std::string g_srv_name = "@(srv_type_string)";
const std::string g_request_name = g_srv_name + ":request";
const std::string g_response_name = g_srv_name + ":response";
const std::string g_idl = R"~~~(
@(idl)
)~~~";

namespace {

// Build a DynamicType by parsing the embedded, fully self-contained IDL.
// Fast-DDS 3.x only parses IDL from a file URI (create_type_w_uri); string
// parsing (create_type_w_document) is declared but unimplemented. We spill
// g_idl to a temporary file, parse the requested type once and cache it.
xtypes::DynamicType build_type_from_idl(
        const std::string& cache_key,
        const std::string& fq_type_name)
{
    namespace fs = std::filesystem;

    std::string sanitized = cache_key;
    std::replace(sanitized.begin(), sanitized.end(), '/', '_');
    std::replace(sanitized.begin(), sanitized.end(), ':', '_');

    const fs::path idl_path = fs::temp_directory_path() /
        ("is_ros2_" + sanitized + "_" + std::to_string(::getpid()) + ".idl");

    {
        std::ofstream idl_file(idl_path);
        idl_file << idl_preprocess::preprocess(g_idl);
    }

    auto builder = xtypes::DynamicTypeBuilderFactory::get_instance()
        ->create_type_w_uri(idl_path.string(), fq_type_name, {});

    std::error_code ec;
    fs::remove(idl_path, ec);

    if (!builder)
    {
        throw std::runtime_error("Failed while parsing type " + fq_type_name);
    }
    return builder->build();
}

xtypes::DynamicType request_type()
{
    static const xtypes::DynamicType cached_type =
        build_type_from_idl(g_request_name, "@(cpp_srv_type)_Request");
    return cached_type;
}

TypeToFactoryRegistrar register_request_type(g_request_name, &request_type);

xtypes::DynamicType response_type()
{
    static const xtypes::DynamicType cached_type =
        build_type_from_idl(g_response_name, "@(cpp_srv_type)_Response");
    return cached_type;
}

TypeToFactoryRegistrar register_response_type(g_response_name, &response_type);
} // anonymous namespace


//==============================================================================
void request_to_ros2(const xtypes::DynamicData& from, Ros2_Request& to)
{
@[for field in alphabetical_request_fields]@
    utils::Convert<Ros2_Request::_@(field.name)_type>::from_xtype_field(from, from->get_member_id_by_name("@(field.name)"), to.@(field.name));
@[end for]@

    // Suppress possible unused variable warnings
    (void)from;
    (void)to;
}

//==============================================================================
void request_to_xtype(const Ros2_Request& from, xtypes::DynamicData& to)
{
@[for field in alphabetical_request_fields]@
    utils::Convert<Ros2_Request::_@(field.name)_type>::to_xtype_field(from.@(field.name), to, to->get_member_id_by_name("@(field.name)"));
@[end for]@

    // Suppress possible unused variable warnings
    (void)from;
    (void)to;
}

//==============================================================================
void response_to_ros2(const xtypes::DynamicData& from, Ros2_Response& to)
{
@[for field in alphabetical_response_fields]@
    utils::Convert<Ros2_Response::_@(field.name)_type>::from_xtype_field(from, from->get_member_id_by_name("@(field.name)"), to.@(field.name));
@[end for]@

    // Suppress possible unused variable warnings
    (void)from;
    (void)to;
}

//==============================================================================
void response_to_xtype(const Ros2_Response& from, xtypes::DynamicData& to)
{
@[for field in alphabetical_response_fields]@
    utils::Convert<Ros2_Response::_@(field.name)_type>::to_xtype_field(from.@(field.name), to, to->get_member_id_by_name("@(field.name)"));
@[end for]@

    // Suppress possible unused variable warnings
    (void)from;
    (void)to;
}

//==============================================================================
class ClientProxy final : public virtual is::ServiceClient
{
public:

    ClientProxy(
            rclcpp::Node& node,
            const std::string& service_name,
            ServiceClientSystem::RequestCallback* callback,
            const rmw_qos_profile_t& qos_profile)
        : _callback(callback)
        , _handle(std::make_shared<PromiseHolder>())
        , _request_data(eprosima::fastdds::dds::DynamicDataFactory::get_instance()->create_data(request_type()))
        , _service_name(service_name)
    {
        _service = node.create_service<Ros2_Srv>(
            service_name,
            [=](const std::shared_ptr<rmw_request_id_t> request_header,
                const std::shared_ptr<Ros2_Request> request,
                const std::shared_ptr<Ros2_Response> response)
                {
                    this->service_callback(request_header, request, response);
                },
            rclcpp::QoS(rclcpp::QoSInitialization::from_rmw(qos_profile), qos_profile));
    }

    void receive_response(
            std::shared_ptr<void> call_handle,
            const xtypes::DynamicData& result) override
    {
        const std::shared_ptr<PromiseHolder>& handle =
            std::static_pointer_cast<PromiseHolder>(call_handle);

        response_to_ros2(result, _response);

        {
            std::ostringstream json_oss;
            eprosima::fastdds::dds::json_serialize(
                result, eprosima::fastdds::dds::DynamicDataJsonFormat::EPROSIMA, json_oss);
            logger << utils::Logger::Level::INFO
                   << "Translating reply from Integration Service to ROS 2 for service reply topic '"
                   << _service_name << "_Reply': [[ " << json_oss.str() << " ]]" << std::endl;
        }

        handle->promise->set_value(_response);
    }

private:

    void service_callback(
            const std::shared_ptr<rmw_request_id_t>&, //request_header
            const std::shared_ptr<Ros2_Request>& request,
            const std::shared_ptr<Ros2_Response>& response)
    {
        logger << utils::Logger::Level::INFO
               << "Receiving request from ROS 2 for service request topic '"
               << _service_name << "_Request'" << std::endl;

        request_to_xtype(*request, _request_data);

        std::promise<Ros2_Response> response_promise;
        _handle->promise = &response_promise;

        std::future<Ros2_Response> future_response = response_promise.get_future();
        (*_callback)(_request_data, *this, _handle);

        if (std::future_status::ready == future_response.wait_for(std::chrono::milliseconds(5000))) // TODO: Make waiting time configurable.
        {
            *response = future_response.get();
        }
        else
        {
            std::cout << "Request timeout." << std::endl;
        }
    }

    struct PromiseHolder
    {
        std::promise<Ros2_Response>* promise;
    };

    ServiceClientSystem::RequestCallback* _callback;
    std::string _service_name;
    const std::shared_ptr<PromiseHolder> _handle;
    xtypes::DynamicData _request_data;
    Ros2_Response _response;
    rclcpp::Service<Ros2_Srv>::SharedPtr _service;
};

//==============================================================================
std::shared_ptr<is::ServiceClient> make_client(
        rclcpp::Node& node,
        const std::string& service_name,
        ServiceClientSystem::RequestCallback* callback,
        const rmw_qos_profile_t& qos_profile)
{
    return std::make_shared<ClientProxy>(node, service_name, callback, qos_profile);
}

namespace {
ServiceClientToFactoryRegistrar register_client(g_response_name, &make_client);
} // anonymous namespace

//==============================================================================
xtypes::DynamicData initialize_response()
{
    return eprosima::fastdds::dds::DynamicDataFactory::get_instance()->create_data(response_type());
}

class ServerProxy final : public virtual is::ServiceProvider
{
public:

    ServerProxy(
            rclcpp::Node& node,
            const std::string& service_name,
            const rmw_qos_profile_t& qos_profile)
        : _service_name(service_name)
        , _request_pool(1)
        , _response_pool(1)
    {
        _ros2_client = node.create_client<Ros2_Srv>(
            service_name,
            rclcpp::QoS(rclcpp::QoSInitialization::from_rmw(qos_profile), qos_profile));
    }

    void call_service(
            const xtypes::DynamicData& request,
            ServiceClient& is_client,
            std::shared_ptr<void> call_handle) override
    {
        if (!_ros2_client->wait_for_service(std::chrono::milliseconds(10)))
        {
            return;
        }

        {
            std::ostringstream json_oss;
            eprosima::fastdds::dds::json_serialize(
                request, eprosima::fastdds::dds::DynamicDataJsonFormat::EPROSIMA, json_oss);
            logger << utils::Logger::Level::INFO
                   << "Translating request from Integration Service to ROS 2 for service request topic '"
                   << _service_name << "_Request': [[ " << json_oss.str() << " ]]" << std::endl;
        }

        // This helps the lambda to value-capture the address of the Integration Service client.
        // TODO(MXG): Would it be dangerous for the lambda to reference-capture the
        // Integration Service client? The lambda might be called after this reference has left
        // scope, so when a lambda does a reference-capture of a reference, does it
        // require the reference to stay alive or does it only require the
        // referred-to object to stay alive? For now we'll use this value-capture
        // technique since it's sure to be safe.
        ServiceClient* const ptr_to_is_client = &is_client;

        Ros2_Request::SharedPtr ros2_request = _request_pool.pop();
        request_to_ros2(request, *ros2_request);
        _ros2_client->async_send_request(
            ros2_request,
            [=](const rclcpp::Client<Ros2_Srv>::SharedFuture future_response)
            {
                this->_wait_for_response(*ptr_to_is_client, std::move(call_handle), future_response, ros2_request);
            });
    }

private:

    void _wait_for_response(
            ServiceClient& is_client,
            std::shared_ptr<void> call_handle,
            const rclcpp::Client<Ros2_Srv>::SharedFuture& future_response,
            Ros2_Request::SharedPtr used_request)
    {
        future_response.wait();

        const Ros2_Response::SharedPtr& response = future_response.get();
        xtypes::DynamicData is_response = _response_pool.pop();
        response_to_xtype(*response, is_response);

        is_client.receive_response(std::move(call_handle), is_response);

        _response_pool.recycle(std::move(is_response));
        _request_pool.recycle(std::move(used_request));
    }

    const std::string _service_name;
    utils::SharedResourcePool<Ros2_Request> _request_pool;
    utils::ResourcePool<xtypes::DynamicData, &initialize_response> _response_pool;
    rclcpp::Client<Ros2_Srv>::SharedPtr _ros2_client;
};

//==============================================================================
std::shared_ptr<is::ServiceProvider> make_server(
        rclcpp::Node& node,
        const std::string& service_name,
        const rmw_qos_profile_t& qos_profile)
{
    return std::make_shared<ServerProxy>(node, service_name, qos_profile);
}

namespace {
ServiceProviderToFactoryRegistrar register_server(g_request_name, &make_server);
}

} //  namespace @(namespace_variable_srv)
} //  namespace ros2
} //  namespace sh
} //  namespace is
} //  namespace eprosima
