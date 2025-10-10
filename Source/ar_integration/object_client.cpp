#include "object_client.h"

#include "util.h"

void U_object_client::async_subscribe_objects()
{
	if (!channel ||
		(subscribe_thread && !subscribe_thread->done())) return;

	subscribe_thread = std::make_unique<stream_thread>( 
		[this](grpc::ClientContext& ctx)
		{
			if (this->subscribe_objects(ctx).error_code() == grpc::StatusCode::UNKNOWN)
				sub_add_disconnected = true;
		});
}

void U_object_client::async_subscribe_delete_objects()
{
	if (!channel ||
		(subscribe_delete_thread && !subscribe_delete_thread->done())) return;

	subscribe_delete_thread = std::make_unique<stream_thread>(
		[this](grpc::ClientContext& ctx)
		{
			if (this->subscribe_delete_objects(ctx).error_code() == grpc::StatusCode::UNKNOWN)
				sub_del_disconnected = true;
		});
}

void U_object_client::state_change_Implementation(connection_state old_state, connection_state new_state)
{
	if (new_state != connection_state::READY) return;

	if (sub_add_disconnected || sub_del_disconnected)
	{
		sub_add_disconnected = false;
		sub_del_disconnected = false;

		sync_objects();
		async_subscribe_objects();
		async_subscribe_delete_objects();
	}
}

grpc::Status U_object_client::subscribe_objects(grpc::ClientContext& ctx) const
{
	google::protobuf::Empty empty;
	auto stream =
		stub->transmit_object(&ctx, empty);
	stream->WaitForInitialMetadata();

	TF_Conv_Wrapper wrapper;
	generated::Object_Instance_TF_Meta msg;
	while (stream->Read(&msg))
		process(msg, wrapper);
	return stream->Finish();
}

grpc::Status U_object_client::subscribe_delete_objects(grpc::ClientContext& ctx) const
{
	google::protobuf::Empty empty;
	auto stream = stub->delete_object(&ctx, empty);
	stream->WaitForInitialMetadata();

	generated::Delete_Request req;
	while (stream->Read(&req))
		on_object_delete.Broadcast(convert<FString>(req.id()));
	return stream->Finish();
}

void U_object_client::sync_objects()
{
	if (!channel) return;

	google::protobuf::Empty empty;
	grpc::ClientContext ctx;
	auto stream = stub->sync_objects(&ctx, empty);
	stream->WaitForInitialMetadata();

	TF_Conv_Wrapper wrapper;
	generated::Object_Instance_TF_Meta msg;
	while (stream->Read(&msg))
		process(msg, wrapper);
	stream->Finish();
}

void U_object_client::process(const generated::Object_Instance_TF_Meta& meta_instance, TF_Conv_Wrapper& wrapper) const
{
	UE_LOG(LogTemp, Warning, TEXT("[OBJECT_CLIENT] Raw PN-ID from message: %d"), meta_instance.object_instance().pn_id());

	/**
	 * workaround for template issue with unreal reflection system
	 */
	const auto& instance = meta_instance.object_instance();
	if (instance.has_obj())
	{
		const auto data = convert_meta<F_object_instance_data>(meta_instance, wrapper);
		on_object_instance_data.Broadcast(data);
		UE_LOG(LogTemp, Warning, TEXT("[OBJECT_CLIENT] Received object with ID: %s and PN-ID: %d"), *data.id, data.pn_id);
	}
	else if (instance.has_box())
	{
		const auto data = 
			convert_meta<F_object_instance_colored_box>(meta_instance, wrapper);
		on_object_instance_colored_box.Broadcast(data);
		UE_LOG(LogTemp, Warning, TEXT("[OBJECT_CLIENT] Received box with ID: %s and PN-ID: %d"), *data.id, data.pn_id);

	}
}