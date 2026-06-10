/*
 * Copyright (C) 2019 Open Source Robotics Foundation
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

#include "MetaPublisher.hpp"

#include <is/core/runtime/StringTemplate.hpp>

#include <is/sh/ros2/Factory.hpp>

#include "IsTypeName.hpp"

#include <utility>

#include <fastdds/dds/xtypes/utils.hpp>
#include <sstream>


namespace eprosima {
namespace is {
namespace sh {
namespace ros2 {

/**
 * @class MetaPublisher
 *        Create a TopicPublisher using the StringTemplate substitution paradigm.
 */
class MetaPublisher : public is::TopicPublisher
{
public:

    MetaPublisher(
//            const core::StringTemplate&& topic_template,
            std::string topic_name,
            const eprosima::xtypes::DynamicType& message_type,
            rclcpp::Node& node,
            const rclcpp::QoS& qos_profile,
            const YAML::Node& /*unused*/)
        : _topic_name(std::move(topic_name))
//        _topic_template(std::move(topic_template))
        , _message_type(message_type)
        , _node(node)
        , _qos_profile(qos_profile)
        , logger_("is::sh::ROS2::MetaPublisher")
    {
        _publisher = _node.create_generic_publisher(
                _topic_name,
                fastdds_type_to_is_name(std::string(_message_type->get_name())),
                _qos_profile);
        _serialise_to_ros2 = Factory::instance().get_serialise_function(_message_type);
    }

    bool publish(
            const eprosima::xtypes::DynamicData& message) override final
    {
//        const std::string topic_name = _topic_template.compute_string(message);

//        const auto insertion = _publishers.insert(
//            std::make_pair(_topic_name, nullptr));
//        const bool inserted = insertion.second;
//        TopicPublisherPtr& publisher = insertion.first->second;

//        if (inserted)
//        {
//          _publisher = _node.create_generic_publisher(
//                  _topic_name,
//                  _message_type.name(),
//                  _qos_profile);
//        }

        {
            std::ostringstream json_oss;
            eprosima::fastdds::dds::json_serialize(message, eprosima::fastdds::dds::DynamicDataJsonFormat::EPROSIMA, json_oss);
            logger_ << utils::Logger::Level::DEBUG
                << "Sending message from Integration Service to ROS 2 for topic '" << _topic_name
                << "': [[ " << json_oss.str() << " ]]" << std::endl;
        }

        rclcpp::SerializedMessage ros2_serialised;
        (*_serialise_to_ros2)(message, ros2_serialised);

        _publisher->publish(ros2_serialised);
        return true;
    }

private:

//    const core::StringTemplate _topic_template;
    const std::string _topic_name;
    // Held by value (DynamicType is a shared_ptr): storing a reference dangled
    // once the caller's DynamicType went out of scope, crashing the async
    // publish/serialise path.
    const eprosima::xtypes::DynamicType _message_type;
    rclcpp::Node& _node;
    const rclcpp::QoS _qos_profile;
    utils::Logger logger_;

    Factory::SerialiseToROS2Function * _serialise_to_ros2;
    rclcpp::GenericPublisher::SharedPtr _publisher;


//    using TopicPublisherPtr = rclcpp::GenericPublisher::SharedPtr;
//    using PublisherMap = std::unordered_map<std::string, TopicPublisherPtr>;
//    PublisherMap _publishers;
};

namespace {
//==============================================================================
std::string make_detail_string(
        const std::string& topic_name,
        const std::string& message_type)
{
    return
        "[Middleware: ROS2, topic template: "
        + topic_name + ", message type: " + message_type + "]";
}

} // anonymous namespace

std::shared_ptr<is::TopicPublisher> make_meta_publisher(
        const std::string& topic_name,
        const eprosima::xtypes::DynamicType& message_type,
        rclcpp::Node& node,
        const rclcpp::QoS& qos_profile,
        const YAML::Node& configuration)
{
    return std::make_shared<MetaPublisher>(
//        core::StringTemplate(topic_name, make_detail_string(topic_name, message_type.name())),
        topic_name,
        message_type, node, qos_profile, configuration);
}

class MetaSubscriber
{

public:
    MetaSubscriber(
//            const core::StringTemplate&& topic_template,
            std::string topic_name,
            const xtypes::DynamicType& message_type,
            TopicSubscriberSystem::SubscriptionCallback* callback,
            rclcpp::Node& node,
            const rclcpp::QoS& qos_profile,
            const YAML::Node& /*unused*/)
            : _callback(callback)
//            _topic_template(std::move(topic_template))
            , _message_type(message_type)
            , _topic_name(std::move(topic_name))
            , logger_("is::sh::ROS2::MetaSubscriber")

    {
//      const std::string topic_name = _topic_template.compute_string(message);

      _deserialise_to_xtypes = Factory::instance().get_deserialise_function(_message_type);
      auto subscription_options = rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>>();
      subscription_options.ignore_local_publications = true; // Enable ignore_local_publications option

      _subscription = node.create_generic_subscription(
              _topic_name, fastdds_type_to_is_name(std::string(_message_type->get_name())), qos_profile,
              #ifdef ROS_IRON
              [=](const std::shared_ptr<rclcpp::SerializedMessage> msg) {
                  this->subscription_callback(msg);
              },
              #else
              [=](const std::shared_ptr<rclcpp::SerializedMessage> msg, const rclcpp::MessageInfo& message_info) {
                  this->subscription_callback(msg, message_info);
              },
              #endif
              subscription_options);
    }

private:

    void subscription_callback(
        #ifdef ROS_IRON
            const std::shared_ptr<rclcpp::SerializedMessage> msg)
        #else
            const std::shared_ptr<rclcpp::SerializedMessage> msg, [[maybe_unused]] const rclcpp::MessageInfo & message_info)
        #endif
    {
      logger_ << utils::Logger::Level::DEBUG
             << "Receiving message from ROS 2 for topic '"
             << _topic_name << "'" << std::endl;

      xtypes::DynamicData data = eprosima::fastdds::dds::DynamicDataFactory::get_instance()->create_data(_message_type);
      (*_deserialise_to_xtypes)(*msg, data);

      {
          std::ostringstream json_oss;
          eprosima::fastdds::dds::json_serialize(data, eprosima::fastdds::dds::DynamicDataJsonFormat::EPROSIMA, json_oss);
          logger_ << utils::Logger::Level::DEBUG
              << "Received message: [[ " << json_oss.str() << " ]]" << std::endl;
      }

      (*_callback)(data, nullptr);
    }

//    const core::StringTemplate _topic_template;

    TopicSubscriberSystem::SubscriptionCallback* _callback;
    // Held by value (DynamicType is a shared_ptr): storing a reference dangled
    // by the time a message arrived, crashing the subscription callback.
    const xtypes::DynamicType _message_type;
    std::string _topic_name;
    utils::Logger logger_;

    rclcpp::GenericSubscription::SharedPtr _subscription;
    Factory::DeserialiseToXtypeFunction * _deserialise_to_xtypes;

};

std::shared_ptr<void> make_meta_subscriber(
        const std::string& topic_name,
        const eprosima::xtypes::DynamicType& message_type,
        TopicSubscriberSystem::SubscriptionCallback* callback,
        rclcpp::Node& node,
        const rclcpp::QoS& qos_profile,
        const YAML::Node& configuration)
{
  return std::make_shared<MetaSubscriber>(
          topic_name, message_type, callback, node,
//          core::StringTemplate(topic_name, make_detail_string(topic_name, message_type.name())),
          qos_profile, configuration);
}

} //  namespace ros2
} //  namespace sh
} //  namespace is
} //  namespace eprosima
