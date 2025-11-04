#include "selection_client.h"

bool U_selection_client::send_selection(const FString& mesh_id, int32 pn_id, assignment_type assignment)
{
	if (!channel || !channel->channel)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SelectionClient] Channel invalid. Cannot send selection!"));
		return false;
	}

	const int32 assignment_raw = static_cast<int32>(assignment);
	UE_LOG(LogTemp, Log, TEXT("[SelectionClient] Sending selection %s (PN-ID: %d, Assignment: %d)"), *mesh_id, pn_id, assignment_raw);

	grpc::ClientContext ctx;
	generated::selection_message msg;

	msg.set_pn_id(pn_id);
	msg.set_object_id(TCHAR_TO_UTF8(*mesh_id));
	msg.set_assignment(static_cast<generated::assignment_type>(assignment_raw));

	google::protobuf::Empty reply;
	auto status = stub_->send_selection(&ctx, msg, &reply);

	if (status.ok())
	{
		UE_LOG(LogTemp, Log, TEXT("[SelectionClient] Successfully sent selection: %s (PN-ID: %d)"), *mesh_id, pn_id);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[SelectionClient] Failed to send selection. Error code: %d, message: %s"), static_cast<int>(status.error_code()), *FString(status.error_message().c_str()));
	}

	return status.ok();
}
