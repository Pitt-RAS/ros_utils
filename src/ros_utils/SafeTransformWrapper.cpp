#include <ros/ros.h>
#include <string>

#include <geometry_msgs/TransformStamped.h>

#include "ros_utils/SafeTransformWrapper.hpp"

namespace ros_utils {

SafeTransformWrapper::SafeTransformWrapper()
    : tf_buffer_(),
      tf_listener_(tf_buffer_)
{
}

bool SafeTransformWrapper::getTransformAtTime(
        geometry_msgs::TransformStamped& transform,
        const std::string& target_frame,
        const std::string& source_frame,
        const ros::Time& time,
        const ros::Duration& timeout) const
{
    const ros::Time start_time = ros::Time::now();

    // Catch TransformException exceptions
    try
    {
        // While we aren't supposed to be shutting down
        while (ros::ok())
        {
            if (ros::Time::now() > start_time + timeout)
            {
                geometry_msgs::TransformStamped last_tf;
                if (tf_buffer_.canTransform(target_frame, source_frame, ros::Time(0))) {
                    last_tf = tf_buffer_.lookupTransform(target_frame, source_frame, ros::Time(0));
                    ROS_ERROR_STREAM("Transform timed out ("
                                  << "current time: " << ros::Time::now() << ", "
                                  << "request time: " << start_time
                                  << ") for frame " << source_frame
                                  << " to frame " << target_frame
                                  << ", latest available at " << last_tf.header.stamp);
                } else {
                    ROS_ERROR_STREAM("Transform timed out ("
                                  << "current time: " << ros::Time::now() << ", "
                                  << "request time: " << start_time
                                  << ") for frame " << source_frame
                                  << " to frame " << target_frame
                                  << ", none available");
                }
                return false;
            }

            // Check if the transform from map to quad can be made right now
            if (tf_buffer_.canTransform(target_frame, source_frame, time))
            {
                // Get the transform
                transform = tf_buffer_.lookupTransform(target_frame, source_frame, time);
                return true;
            }

            // Handle callbacks and sleep for a small amount of time
            // before looping again
            ros::spinOnce();
            ros::Duration(0.005).sleep();
        }
    }
    // Catch any exceptions that might happen while transforming
    catch (tf2::TransformException& ex)
    {
        ROS_ERROR("Exception transforming %s to %s: %s",
                  target_frame.c_str(),
                  source_frame.c_str(),
                  ex.what());
    }

    ROS_ERROR("Exception or ros::ok was false while waiting for transform");
    return false;
}

} // namespace ros_utils
