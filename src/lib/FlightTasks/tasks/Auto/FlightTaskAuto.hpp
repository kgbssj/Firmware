/****************************************************************************
 *
 *   Copyright (c) 2018 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file FlightTaskAuto.hpp
 *
 * Map from global triplet to local quadruple.
 */

#pragma once

#include "FlightTask.hpp"
#include <uORB/topics/position_setpoint_triplet.h>
#include <uORB/topics/position_setpoint.h>
#include <uORB/topics/home_position.h>
#include <lib/ecl/geo/geo.h>

/**
 * This enum has to agree with position_setpoint_s type definition
 * The only reason for not using the struct position_setpoint is because
 * of the size
 */
enum class WaypointType : int {
	position = 0,
	velocity,
	loiter,
	takeoff,
	land,
	idle,
	offboard, // only part of this structure due to legacy reason. It is not used within the Auto flighttasks
	follow_target
};

enum class State {
	offtrack, /**< Vehicle is more than cruise speed away from track */
	target_behind, /**< Vehicle is in front of target. */
	previous_infront, /**< Vehilce is behind previous waypoint.*/
	none /**< Vehicle is in normal tracking mode from triplet previous to triplet target */
};

class FlightTaskAuto : public FlightTask
{
public:
	FlightTaskAuto() = default;

	virtual ~FlightTaskAuto() = default;
	bool initializeSubscriptions(SubscriptionArray &subscription_array) override;
	bool activate() override;
	bool updateInitialize() override;

protected:
	void _setDefaultConstraints() override;
	float _getMaxCruiseSpeed() {return MPC_XY_CRUISE.get();} /**< getter for default cruise speed */
	matrix::Vector2f _getTargetVelocityXY(); /**< only used for follow-me and only here because of legacy reason.*/
	void _updateInternalWaypoints(); /**< Depending on state of vehicle, the internal waypoints might differ from target (for instance if offtrack). */
	bool _compute_heading_from_2D_vector(float &heading, matrix::Vector2f v); /**< Computes and sets heading a 2D vector */
	void _updateAvoidanceWaypoints(); /**< fill desired_waypoints with the triplets. */

	matrix::Vector3f _prev_prev_wp{}; /**< Pre-previous waypoint (local frame). This will be used for smoothing trajectories -> not used yet. */
	matrix::Vector3f _prev_wp{}; /**< Previous waypoint  (local frame). If no previous triplet is available, the prev_wp is set to current position. */
	matrix::Vector3f _target{}; /**< Target waypoint  (local frame).*/
	matrix::Vector3f _next_wp{}; /**< The next waypoint after target (local frame). If no next setpoint is available, next is set to target. */
	float _mc_cruise_speed{0.0f}; /**< Requested cruise speed. If not valid, default cruise speed is used. */
	WaypointType _type{WaypointType::idle}; /**< Type of current target triplet. */
	uORB::Subscription<home_position_s> *_sub_home_position{nullptr};

	State _current_state{State::none};

	float _speed_at_target = 0.0f; /**< Desired velocity at target. */

	DEFINE_PARAMETERS_CUSTOM_PARENT(FlightTask,
					(ParamFloat<px4::params::MPC_XY_CRUISE>) MPC_XY_CRUISE,
					(ParamFloat<px4::params::MPC_CRUISE_90>) MPC_CRUISE_90, // speed at corner when angle is 90 degrees move to line
					(ParamFloat<px4::params::NAV_ACC_RAD>) NAV_ACC_RAD, // acceptance radius at which waypoints are updated move to line
					(ParamInt<px4::params::MPC_YAW_MODE>) MPC_YAW_MODE // defines how heading is executed
				       );

private:
	matrix::Vector2f _lock_position_xy{NAN, NAN}; /**< if no valid triplet is received, lock positition to current position */
	bool _yaw_lock = false; /**< if within acceptance radius, lock yaw to current yaw */
	uORB::Subscription<position_setpoint_triplet_s> *_sub_triplet_setpoint{nullptr};

	matrix::Vector3f
	_triplet_target; /**< current triplet from navigator which may differ from the intenal one (_target) depending on the vehicle state. */
	matrix::Vector3f
	_triplet_prev_wp; /**< previous triplet from navigator which may differ from the intenal one (_prev_wp) depending on the vehicle state.*/
	matrix::Vector3f
	_triplet_next_wp; /**< next triplet from navigator which may differ from the intenal one (_next_wp) depending on the vehicle state.*/
	matrix::Vector2f _closest_pt; /**< closest point to the vehicle position on the line previous - target */

	map_projection_reference_s _reference_position{}; /**< Structure used to project lat/lon setpoint into local frame. */
	float _reference_altitude = NAN;  /**< Altitude relative to ground. */
	hrt_abstime _time_stamp_reference = 0; /**< time stamp when last reference update occured. */

	bool _evaluateTriplets(); /**< Checks and sets triplets. */
	bool _isFinite(const position_setpoint_s sp); /**< Checks if all waypoint triplets are finite. */
	bool _evaluateGlobalReference(); /**< Check is global reference is available. */
	float _getVelocityFromAngle(const float angle); /**< Computes the speed at target depending on angle. */
	State _getCurrentState(); /**< Computes the current vehicle state based on the vehicle position and navigator triplets. */
	void _set_heading_from_mode(); /**< @see  MPC_YAW_MODE */
};
