#include "util.h"

double ToUnixTimestampDecimal()
{
	const auto stamp = FDateTime::Now();
	return static_cast<double>(stamp.GetTicks() - FDateTime(1970, 1, 1).GetTicks()) / ETimespan::TicksPerSecond;
}

generated::Transformation_Meta generate_meta()
{
	generated::Transformation_Meta out;

	auto& right = *out.mutable_right();
	right.set_axis(generated::Y);
	right.set_direction(generated::POSITIVE);

	auto& forward = *out.mutable_forward();
	forward.set_axis(generated::X);
	forward.set_direction(generated::POSITIVE);

	auto& up = *out.mutable_up();
	up.set_axis(generated::Z);
	up.set_direction(generated::POSITIVE);

	auto& scale = *out.mutable_scale();
	scale.set_num(1);
	scale.set_denom(100);

	return out;
}

template<>
Transformation::AxisAlignment convert(const generated::Axis_Alignment& in)
{
	return {
		static_cast<Transformation::Axis>(in.axis()),
		static_cast<Transformation::AxisDirection>(in.direction())
	};
}

template<>
Transformation::Ratio convert(const generated::Ratio& in)
{
	return {
		in.num(),
		in.denom()
	};
}

template<>
Transformation::TransformationMeta convert(const generated::Transformation_Meta& in)
{
	return {
		convert<Transformation::AxisAlignment>(in.right()),
		convert<Transformation::AxisAlignment>(in.forward()),
		convert<Transformation::AxisAlignment>(in.up()),
		convert<Transformation::Ratio>(in.scale())
	};
}

template<>
FVector convert_meta(const generated::vertex_3d& in, const Transformation::TransformationConverter* cv)
{
	if (cv == nullptr)
		return FVector(in.x(), in.y(), in.z());

	return cv->convert_point_proto(in);
}

template<>
FVector convert_meta(const generated::vertex_3d_no_scale& in, const Transformation::TransformationConverter* cv)
{
	if (cv == nullptr)
		return FVector(in.x(), in.y(), in.z());

	return cv->convert_point_proto(in);
}

template<>
FVector convert_meta(const generated::index_3d& in, const Transformation::TransformationConverter* cv)
{
	if (cv == nullptr)
		return FVector(in.x(), in.y(), in.z());
	
	return cv->convert_index_proto(in);
}

template<>
FVector convert_meta(const generated::size_3d& in, const Transformation::TransformationConverter* cv)
{
	if (cv == nullptr)
		return FVector(in.x(), in.y(), in.z());

	return cv->convert_size_proto(in);
}

template<>
FQuat convert_meta(const generated::quaternion& in, const Transformation::TransformationConverter* cv)
{
	if (cv == nullptr)
		return FQuat(in.x(), in.y(), in.z(), in.w());

	return cv->convert_quaternion_proto(in);
}

template<>
FQuat convert_meta(const generated::Rotation_3d& in, const Transformation::TransformationConverter* cv)
{
	auto out = FQuat::MakeFromEuler(FVector(
		FMath::RadiansToDegrees(in.roll()),
		FMath::RadiansToDegrees(in.pitch()),
		FMath::RadiansToDegrees(in.yaw())));

	if (cv == nullptr)
		return out;

	return cv->convert_quaternion(out);
}

template<>
FColor convert(const generated::color& in)
{
	return FColor(in.r(), in.g(), in.b(), in.a());
}

template<>
TArray<int32> convert(const google::protobuf::RepeatedField<unsigned>& in)
{
	TArray<int32> out;
	out.Reserve(in.size());

	for (const auto& it : in)
		out.Add(it);

	return out;
}

template<>
FString convert(const std::string& in)
{
	return FString(in.c_str());
}

template<>
std::string convert(const FString& in)
{
	return std::string(TCHAR_TO_UTF8(*in));
}

template<>
F_mesh_data convert_meta(const generated::Mesh_Data& in, const Transformation::TransformationConverter* cv)
{
	F_mesh_data out;
	out.vertices = convert_array_meta<FVector>(in.vertices(), cv);
	out.indices = convert<TArray<int32>>(in.indices());
	out.name = convert<FString>(in.name());

	if (in.has_vertex_normals())
		out.normals = convert_array_meta<FVector>(in.vertex_normals().vertices(), cv);
	if (in.has_vertex_colors())
		out.colors = convert_array<FColor>(in.vertex_colors().colors());

	return out;
}

template<>
F_mesh_data convert_meta(const generated::Mesh_Data_TF_Meta& in, TF_Conv_Wrapper& cv)
{
	using namespace Transformation;
	if (in.has_transformation_meta())
		cv.set_source(convert<TransformationMeta>(in.transformation_meta()));

	return convert_meta<F_mesh_data>(in.mesh_data(), &cv.converter());
}

template<>
FBox convert_meta(const generated::aabb& in, const Transformation::TransformationConverter* cv)
{
	return FBox::BuildAABB( 
		convert_meta<FVector>(in.translation(), cv),
		convert_meta<FVector>(in.diagonal(), cv) / 2.f
	);
}

template<>
F_object_prototype convert_meta(const generated::Object_Prototype& in, const Transformation::TransformationConverter* cv)
{
	F_object_prototype out;
	out.name = convert<FString>(in.name());
	out.mean_color = convert<FColor>(in.mean_color());
	out.bounding_box = convert_meta<FBox>(in.bounding_box(), cv);
	out.mesh_name = convert<FString>(in.mesh_name());
	out.type = convert<FString>(in.type());

	return out;
}

template<>
F_object_prototype convert_meta(const generated::Object_Prototype_TF_Meta& in, TF_Conv_Wrapper& cv)
{
	using namespace Transformation;
	if (in.has_transformation_meta())
		cv.set_source(convert<TransformationMeta>(in.transformation_meta()));

	return convert_meta<F_object_prototype>(in.object_prototype(), &cv.converter());
}

template<>
F_obb convert_meta(const generated::Obb& in, const Transformation::TransformationConverter* cv)
{
	F_obb out;
	out.axis_box = convert_meta<FBox>(in.axis_aligned(), cv);
	out.rotation = convert_meta<FQuat>(in.rotation(), cv);

	return out;
}

template<>
FMatrix convert(const generated::Matrix& in)
{
	FMatrix out;

	if (in.rows() != 4 ||
		in.cols() != 4 ||
		in.data_size() != 16)
		return out;

	for (size_t y = 0; y < 4; ++y)
		for (size_t x = 0; x < 4; ++x)
			out.M[y][x] = in.data()[y * 4 + x];

	return out;
}

template<>
FTransform convert_meta(const generated::Matrix& in, const Transformation::TransformationConverter* cv)
{
	if (cv == nullptr)
	{
		FMatrix temp;

		if (in.rows() != 4 ||
			in.cols() != 4 ||
			in.data_size() != 16)
			return {};

		for (size_t y = 0; y < 4; ++y)
			for (size_t x = 0; x < 4; ++x)
				temp.M[y][x] = in.data()[y * 4 + x];

		return FTransform(std::move(temp));
	}
	return cv->convert_matrix_proto(in);
}

template<>
F_object_data convert_meta(const generated::Object_Data& in, const Transformation::TransformationConverter* cv)
{
	F_object_data out;
	out.prototype_name = convert<FString>(in.prototype_name());
	out.transform = convert_meta<FTransform>(in.transform(), cv);

	return out;
}

template<>
F_colored_box convert_meta(const generated::Colored_Box& in, const Transformation::TransformationConverter* cv)
{
	F_colored_box out;
	out.box = convert_meta<F_obb>(in.obbox(), cv);
	out.color = convert<FColor>(in.box_color());

	return out;
}

/*template<>
F_object_instance_data convert(const generated::Object_Instance& in)
{
	F_object_instance_data out;

	out.id = convert<FString>(in.id());
	out.data = convert<F_object_data>(in.obj());

	return out;
}*/

template<>
F_object_instance_data convert_meta(const generated::Object_Instance& in, const Transformation::TransformationConverter* cv)
{
	F_object_instance_data out;

	out.id = convert<FString>(in.id());
	out.pn_id = in.pn_id();
	out.data = convert_meta<F_object_data>(in.obj(), cv);

	return out;
}

template<>
F_object_instance_data convert_meta(const generated::Object_Instance_TF_Meta& in, TF_Conv_Wrapper& cv)
{
	using namespace Transformation;
	if (in.has_transformation_meta())
		cv.set_source(convert<TransformationMeta>(in.transformation_meta()));

	return convert_meta<F_object_instance_data>(in.object_instance(), &cv.converter());
}

template<>
F_object_instance_colored_box convert_meta(const generated::Object_Instance& in, const Transformation::TransformationConverter* cv)
{
	F_object_instance_colored_box out;

	out.id = convert<FString>(in.id());
	out.pn_id = in.pn_id();
	out.data = convert_meta<F_colored_box>(in.box(), cv);

	return out;
}

template<>
F_object_instance_colored_box convert_meta(const generated::Object_Instance_TF_Meta& in, TF_Conv_Wrapper& cv)
{
	using namespace Transformation;
	if (in.has_transformation_meta())
		cv.set_source(convert<TransformationMeta>(in.transformation_meta()));

	return convert_meta<F_object_instance_colored_box>(in.object_instance(), &cv.converter());
}

template<>
F_voxel_data convert_meta(const generated::Voxels& in, const Transformation::TransformationConverter* cv)
{
	F_voxel_data res;
	res.voxel_side_length = in.voxel_side_length();
	if (cv != nullptr)
		res.voxel_side_length = cv->convert_scale(res.voxel_side_length);

	res.robot_origin = convert_meta<FTransform>(in.robot_origin(), cv);
	res.indices = convert_array_meta<FVector>(in.voxel_indices(), cv);

	return res;		
}

template<>
F_voxel_data convert_meta(const generated::Voxel_TF_Meta& in, TF_Conv_Wrapper& cv)
{
	using namespace Transformation;
	if (in.has_transformation_meta())
		cv.set_source(convert<TransformationMeta>(in.transformation_meta()));

	return convert_meta<F_voxel_data>(in.voxels(), &cv.converter());
}

template<>
TArray<FVector> convert_meta(const generated::Tcps& in, const Transformation::TransformationConverter* cv)
{
	return convert_array_meta<FVector>(in.points(), cv);
}

template<>
TArray<FVector> convert_meta(const generated::Tcps_TF_Meta& in, TF_Conv_Wrapper& cv)
{
	using namespace Transformation;
	if (in.has_transformation_meta())
		cv.set_source(convert<TransformationMeta>(in.transformation_meta()));

	return convert_meta<TArray<FVector>>(in.tcps(), &cv.converter());
}

template<>
TOptional<FTransform> convert(const generated::ICP_Result& in)
{
	if (!in.has_data())
		return {};

	const auto& data = in.data();

	if (!data.has_transformation_meta())
		return convert_meta<FTransform>(data.matrix(), nullptr);

	using namespace Transformation;
	TransformationConverter converter(
		convert<TransformationMeta>(data.transformation_meta()), UnrealMeta
	);
	return convert_meta<FTransform>(data.matrix(), &converter);
}

template<>
FFrankaJoints convert(const generated::Joints& in)
{
	FFrankaJoints joints;
	joints.theta_0 = in.theta_1();
	joints.theta_1 = in.theta_2();
	joints.theta_2 = in.theta_3();
	joints.theta_3 = in.theta_4();
	joints.theta_4 = in.theta_5();
	joints.theta_5 = in.theta_6();
	joints.theta_6 = in.theta_7();

	return joints;
}

template<>
F_joints_synced convert(const generated::Sync_Joints& in)
{
	F_joints_synced out;
	out.joints = convert<FFrankaJoints>(in.joints());
	out.time_stamp = FDateTime(in.utc_timepoint() * ETimespan::TicksPerSecond + FDateTime(1970, 1, 1).GetTicks());

	return out;
}

template<>
std::array<float, 3> convert(const FVector& in)
{
	std::array<float, 3> out;
	out[0] = in.X;
	out[1] = in.Y;
	out[2] = in.Z;

	return out;
}




template<>
generated::quaternion convert(const FQuat& in)
{
	FQuat temp = in;
	temp.Normalize();

	generated::quaternion out;

	out.set_x(temp.X);
	out.set_y(temp.Y);
	out.set_z(temp.Z);
	out.set_w(temp.W);

	return out;
}


template<>
generated::size_3d convert(const FVector& in)
{
	generated::size_3d out;
	out.set_x(in.X);
	out.set_y(in.Y);
	out.set_z(in.Z);

	return out;
}

template<>
generated::vertex_3d convert(const FVector& in)
{
	generated::vertex_3d out;
	out.set_x(in.X);
	out.set_y(in.Y);
	out.set_z(in.Z);

	return out;
}

template<>
generated::Matrix convert(const FMatrix& in)
{
	generated::Matrix out;
	out.set_rows(4);
	out.set_cols(4);
	const auto data = out.mutable_data();
	data->Reserve(16);
	for (size_t y = 0; y < 4; ++y)
		for (size_t x = 0; x < 4; ++x)
			*data->Add() = in.M[y][x];

	return out;
}

template<>
generated::Matrix convert(const FTransform& in)
{
	generated::Matrix out;
	out.set_rows(4);
	out.set_cols(4);
	const auto data = out.mutable_data();
	data->Reserve(16);

	const auto temp = in.ToMatrixWithScale();
	//apply_mask(temp);

	for (size_t y = 0; y < 4; ++y)
		for (size_t x = 0; x < 4; ++x)
			data->Add(temp.M[y][x]);

	return out;
}

/*
template<>
std::unique_ptr<draco::PointCloud> convert(const TArray<FVector>& in)
{
	TSet<size_t> not_nan;
	for (size_t i = 0; i < in.Num(); ++i)
	{
		if (!in[i].ContainsNaN())
			not_nan.Add(i);
	}

	draco::PointCloudBuilder builder;
	builder.Start(not_nan.Num());

	const int att_id = builder.AddAttribute(draco::GeometryAttribute::POSITION, 3, draco::DataType::DT_FLOAT32);
	const auto data = convert_std_array<std::array<float, 3>, true>(in);

	for (const auto& idx : not_nan)
		builder.SetAttributeValueForPoint(att_id, draco::PointIndex(idx), data.data() + 3 * idx);

	return builder.Finalize(false);
}

template<>
generated::draco_data convert(const F_point_cloud& pcl)
{
	generated::draco_data request;

	const auto draco_cloud = convert<std::unique_ptr<draco::PointCloud>>(pcl.data);

	draco::EncoderBuffer buffer;
	draco::PointCloudKdTreeEncoder encoder;
	draco::EncoderOptions options = draco::EncoderOptions::CreateDefaultOptions();
	options.SetSpeed(4, 1);
	encoder.SetPointCloud(*draco_cloud);
	encoder.Encode(options, &buffer);

	request.set_data(buffer.data(), buffer.size());
	request.set_timestamp(pcl.abs_timestamp);

	return request;
}*/

template<>
generated::Pcl_Data convert(const F_point_cloud& pcl)
{
	generated::Pcl_Data request;
	request.mutable_vertices()->CopyFrom(
		convert_array<generated::vertex_3d, true>(pcl.data));

	request.set_timestamp(pcl.abs_timestamp);

	return request;
}

template<>
generated::Rotation_3d convert(const FQuat& in)
{
	generated::Rotation_3d out;

	FQuat cpy = in;

	//cpy.X *= -1;
	//cpy.W *= -1;

	const auto& rot = cpy.Rotator();

	out.set_roll(FMath::DegreesToRadians(rot.Roll));
	out.set_pitch(FMath::DegreesToRadians(rot.Pitch));
	out.set_yaw(FMath::DegreesToRadians(rot.Yaw));

	return out;
}

template<>
generated::aabb convert(const FBox& in)
{
	generated::aabb out;
	*out.mutable_diagonal() = convert<generated::size_3d>(in.GetExtent() * 2.f);
	*out.mutable_translation() = convert<generated::vertex_3d>(in.GetCenter());

	return out;
}

template<>
generated::Obb convert(const F_obb& in)
{
	generated::Obb out;

	*out.mutable_rotation() = convert<generated::quaternion>(in.rotation);
	*out.mutable_axis_aligned() = convert<generated::aabb>(in.axis_box);

	return out;
}

template<>
generated::Hand_Data convert(const std::pair<FXRMotionControllerData, FDateTime>& in)
{
	generated::Hand_Data out;

	const auto& hand_data = in.first;

	out.set_valid(hand_data.bValid);
	out.set_hand(static_cast<generated::hand_index>(hand_data.HandIndex));
	out.set_tracking_stat(static_cast<generated::tracking_status>(hand_data.TrackingStatus));

	if (hand_data.bValid)
	{
		*out.mutable_grip_position() = convert<generated::vertex_3d>(hand_data.GripPosition);
		*out.mutable_grip_rotation() = convert<generated::quaternion>(hand_data.GripRotation);
		*out.mutable_aim_position() = convert<generated::vertex_3d>(hand_data.AimPosition);
		*out.mutable_aim_rotation() = convert<generated::quaternion>(hand_data.AimRotation);
		*out.mutable_hand_key_positions() = convert_array<generated::vertex_3d>(hand_data.HandKeyPositions);
		*out.mutable_hand_key_rotations() = convert_array<generated::quaternion>(hand_data.HandKeyRotations);

		const auto mutable_radii = out.mutable_hand_key_radii();
		mutable_radii->Reserve(hand_data.HandKeyRadii.Num());
		for (const float& f : hand_data.HandKeyRadii)
			mutable_radii->Add(f);
	}
	out.set_is_grasped(hand_data.bIsGrasped);
	out.set_utc_timestamp(in.second.GetTicks());

	return out;
}

template<>
Sync_Joints_Data convert(const generated::Sync_Joints_Transmission& in)
{
	Sync_Joints_Data out;
	if (in.has_sync_joints_data())
		out.Emplace<TArray<F_joints_synced>>(convert_tarray<F_joints_synced>(in.sync_joints_data().sync_joints()));
	else
		out.Emplace<Visual_Change>(static_cast<Visual_Change>(in.state_update()));

	return out;
}

template<>
Tcps_Data convert_meta(const generated::Tcps_Transmission& in, TF_Conv_Wrapper& cv)
{
	Tcps_Data out;
	if (in.has_tcps_data())
		out.Emplace<TArray<FVector>>(convert_meta<TArray<FVector>>(in.tcps_data(), cv));
	else
		out.Emplace<Visual_Change>(static_cast<Visual_Change>(in.state_update()));

	return out;
}

template<>
Voxel_Data convert_meta(const generated::Voxel_Transmission& in, TF_Conv_Wrapper& cv)
{
	Voxel_Data out;
	if (in.has_voxels_data())
		out.Emplace<F_voxel_data>(convert_meta<F_voxel_data>(in.voxels_data(), cv));
	else
		out.Emplace<Visual_Change>(static_cast<Visual_Change>(in.state_update()));

	return out;
}


void TF_Conv_Wrapper::set_source(const Transformation::TransformationMeta& meta)
{
	m_converter = std::make_unique<Transformation::TransformationConverter>(meta, Transformation::UnrealMeta);
}

const Transformation::TransformationConverter& TF_Conv_Wrapper::converter() const
{
	return *m_converter;
}

bool TF_Conv_Wrapper::has_converter() const
{
	return !!m_converter;
}